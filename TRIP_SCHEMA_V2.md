# feature/trip-schema — V2 基于出行日期的数据模型

## 一、核心改动

新增 `Trip` 表，将"车次模板"和"每日班次"解耦，支持同一车次在不同日期有独立余票。

```
V1: Train(remainingSeats) ← Order(trainId)
V2: Train(无 remainingSeats) ← Trip(trainId, travelDate, basePrice, remainingSeats) ← Order(tripId, price, travelDate)
```

## 二、Schema V2 变更

### Train 表
- 删除 `remainingSeats`（移入 Trip）
- `departureTime` / `arrivalTime` / `totalSeats` 保留为默认值

### Trip 表（新增）
| 列 | 说明 |
|----|------|
| tripId | 主键 |
| trainId | FK→Train |
| travelDate | 出行日期，如 2026-07-15 |
| departureTime | 当日发车时间 |
| arrivalTime | 当日到达时间 |
| totalSeats | 当日总座位数 |
| remainingSeats | 当日剩余座位数 |
| basePrice | 基础票价（供动态票价使用） |
| enabled | 是否启用 |
| UNIQUE(trainId, travelDate) | 同一车次同一天唯一 |

### Order 表变更
- `trainId` → `tripId`（通过 Trip 关联 Train）
- 新增 `travelDate` 冗余列（避免 JOIN）
- 新增 `price` 列（实际成交票价）
- status=3 已过期

## 三、文件清单

### 新增（3 个）
| 文件 | 说明 |
|------|------|
| `database/schema_v2.sql` | V2 建表 SQL |
| `include/database/trip_record.h` | TripRecord 结构体 |
| `src/database/database_manager_trip.cpp` | Trip CRUD 实现 |

### 修改（17 个）
| 文件 | 变更内容 |
|------|---------|
| `include/database_manager.h` | 新增 Trip API 声明，更新结构体 |
| `include/ticket_manager.h` | bookTicket/changeTicket 参数 trainId→tripId |
| `include/train_manager.h` | Train 结构体去 remainingSeats |
| `include/database/train_record.h` | 去 remainingSeats |
| `include/database/order_record.h` | trainId→tripId，新增 travelDate/price |
| `src/database/database_manager.cpp` | 读 schema_v2.sql，seedDemoData 适配 V2 |
| `src/database/database_manager_train.cpp` | 去 remainingSeats SQL |
| `src/database/database_manager_order.cpp` | INSERT 改 tripId + travelDate + price |
| `src/database/database_manager_ticket.cpp` | 查询 JOIN 改 Order→Trip→Train |
| `src/database/database_manager_statistics.cpp` | 统计 JOIN 适配 Trip，修复月份乱码 |
| `src/ticket/ticket_manager.cpp` | 全链路 tripId + travelDate + price |
| `src/train/train_manager.cpp` | 去 remainingSeats |
| `src/ui/ticket_service_dialog.cpp` | searchTrains→searchTrips |
| `src/ui/train_management_dialog.cpp` | 适配 V2 字段变更 |
| `include/ticket_service_dialog.h` | 方法名适配 |
| `CMakeLists.txt` | 添加新文件 |
| `tests/statistics_smoke_test.cpp` | V2 冒烟测试（建Trip→订3退1） |

## 四、新增 API

### DatabaseManager（Trip CRUD）
```cpp
std::optional<int> createTrip(trainId, travelDate, totalSeats);
std::optional<TripRecord> findTripById(tripId);
std::optional<TripRecord> findOrCreateTrip(trainId, travelDate, totalSeats);
bool adjustTripSeats(tripId, delta);  // 防超售
QList<TripRecord> findTripsByTrain(trainId);
```

### TicketManager（接口变更）
```cpp
// V1: bookTicket(userId, trainId, passengerName)
// V2: bookTicket(userId, tripId, passengerName)  ← price 自动取自 Trip.basePrice

// V1: changeTicket(orderId, newTrainId)
// V2: changeTicket(orderId, newTripId)

// V1: searchTrains(dep, arr, date)
// V2: searchTrips(dep, arr, date)  ← 返回 tripId + travelDate
```

## 五、Bug 修复

- `monthlyPassengerFlow()` 月份显示 `%Y-%m` 而非 `2026-07`：`strftime('%%Y-%%m')` → `strftime('%Y-%m')`

## 六、编译

```bash
mkdir _build && cd _build
cmake .. -G "MinGW Makefiles" \
  -DCMAKE_CXX_COMPILER=D:/Qt6/Tools/mingw1310_64/bin/g++.exe \
  -DCMAKE_PREFIX_PATH=D:/Qt6/6.5.3/mingw_64
cmake --build .
```

首次运行前需删除旧 `database/train_ticket.db`（V1 表结构不兼容）。
