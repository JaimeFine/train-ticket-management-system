-- Schema V2: 基于出行日期的 DailyTrain / Trip 数据模型

-- 用户表：登录账号与角色
CREATE TABLE IF NOT EXISTS User (
    userId INTEGER PRIMARY KEY AUTOINCREMENT,
    username TEXT NOT NULL UNIQUE,        -- 登录名，全局唯一
    password TEXT NOT NULL,
    role INTEGER NOT NULL,                -- 角色：0=Guest 1=Seller 2=Admin
    enabled INTEGER NOT NULL DEFAULT 1    -- 1=启用 0=禁用（软删除）
);

-- 车站表：站名字典
CREATE TABLE IF NOT EXISTS Station (
    stationId INTEGER PRIMARY KEY AUTOINCREMENT,
    stationName TEXT NOT NULL UNIQUE      -- 站名唯一
);

-- V2: Train 作为车次模板，去掉 remainingSeats（余票在 Trip 按日维护）
CREATE TABLE IF NOT EXISTS Train (
    trainId INTEGER PRIMARY KEY AUTOINCREMENT,
    trainNumber TEXT NOT NULL UNIQUE,     -- 车次号，全局唯一
    departureStationId INTEGER NOT NULL,
    arrivalStationId INTEGER NOT NULL,
    departureTime TEXT NOT NULL,          -- 默认发车时间
    arrivalTime TEXT NOT NULL,            -- 默认到达时间
    totalSeats INTEGER NOT NULL CHECK(totalSeats >= 0),  -- 默认总座位数
    enabled INTEGER NOT NULL DEFAULT 1,   -- 1=启用 0=停用（软删除）
    FOREIGN KEY (departureStationId) REFERENCES Station(stationId),
    FOREIGN KEY (arrivalStationId) REFERENCES Station(stationId)
);

-- V2 新增: 每日班次 = 车次 + 出行日期，可独立设置时间和座位
CREATE TABLE IF NOT EXISTS Trip (
    tripId INTEGER PRIMARY KEY AUTOINCREMENT,
    trainId INTEGER NOT NULL,             -- 所属车次模板
    travelDate TEXT NOT NULL,             -- 出行日期
    departureTime TEXT NOT NULL,          -- 当日发车时间
    arrivalTime TEXT NOT NULL,            -- 当日到达时间
    totalSeats INTEGER NOT NULL CHECK(totalSeats >= 0),
    remainingSeats INTEGER NOT NULL CHECK(remainingSeats >= 0),  -- 余票不允许为负，防止超卖
    basePrice REAL NOT NULL DEFAULT 0,       -- 当日基础票价
    enabled INTEGER NOT NULL DEFAULT 1,   -- 1=在售 0=停运
    FOREIGN KEY (trainId) REFERENCES Train(trainId),
    UNIQUE(trainId, travelDate)           -- 同一车次每天最多一个班次
);

-- V2: Order 通过 tripId 关联 Trip，直接存 travelDate 便于查询；一行订单 = 一张票
CREATE TABLE IF NOT EXISTS "Order" (
    orderId INTEGER PRIMARY KEY AUTOINCREMENT,
    userId INTEGER NOT NULL,              -- 下单用户
    tripId INTEGER NOT NULL,              -- 所购班次
    passengerName TEXT NOT NULL,          -- 乘车人姓名
    travelDate TEXT NOT NULL,             -- 冗余列，避免 JOIN
    purchaseTime TEXT NOT NULL,           -- 购票时间
    price REAL NOT NULL DEFAULT 0,         -- 实际成交票价
    status INTEGER NOT NULL,              -- 0=已预订 1=已退票 2=已改签 3=已过期
    FOREIGN KEY (userId) REFERENCES User(userId),
    FOREIGN KEY (tripId) REFERENCES Trip(tripId)
);

-- 操作日志表：记录管理/售票端关键操作，用于审计
CREATE TABLE IF NOT EXISTS OperationLog (
    logId INTEGER PRIMARY KEY AUTOINCREMENT,
    operatorUsername TEXT NOT NULL,       -- 操作人用户名
    action TEXT NOT NULL,                 -- 操作类型
    detail TEXT NOT NULL,                 -- 操作详情
    createdAt TEXT NOT NULL               -- 记录时间
);
