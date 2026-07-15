#pragma once

#include <QString>
#include <optional>
#include <QList>
#include "database/order_record.h"
#include "database/station_record.h"
#include "database/user_record.h"
#include "database/train_record.h"
#include "database/trip_record.h"

class QSqlDatabase;

// 数据库管理器：全项目唯一直接读写 SQLite 的模块，其余 Manager 均经由它访问数据
class DatabaseManager {
public:
    // Keeping the existing class name and keep initialize() as the main entry
    // point, because that matches the current project architecture.

    // Constructor and destructor
    DatabaseManager();
    ~DatabaseManager();

    // Main functionality of initialization:
    // - open SQLite
    // - enables foreign key support
    // - creates the Version 1 tables if they do not already exist
    bool initialize();

    // 连接状态 / 数据库路径 / 最近错误信息
    bool isOpen() const;
    bool wasCreated() const;
    QString databasePath() const;
    QString lastError() const;

    // >>> User APIs：用户增删改查、按角色筛选、启用/禁用账号
    bool addUser(const UserRecord &user);
    std::optional<UserRecord> findUserById(int userId) const;
    std::optional<UserRecord> findUserByUsername(const QString &username) const;
    QList<UserRecord> findUsersByRole(int role) const;
    bool updateUser(const UserRecord &user);
    bool setUserEnabled(int userId, bool enabled);

    // >>> Station APIs：车站增删查
    bool addStation(const StationRecord &station);
    std::optional<StationRecord> findStationById(int stationId) const;
    std::optional<StationRecord> findStationByName(
        const QString &stationName
    ) const;
    QList<StationRecord> getAllStations() const;
    bool deleteStation(int stationId);

    // >>> Train APIs（V2：车次仅作模板，余票下沉到 Trip）
    bool addTrain(const TrainRecord &train);
    std::optional<TrainRecord> findTrainById(int trainId) const;
    std::optional<TrainRecord> findTrainByNumber(const QString &trainNumber) const;
    bool updateTrain(const TrainRecord &train);

    // >>> Trip APIs（V2 新增：每日班次 = 车次+日期，余票在此维护）
    std::optional<int> createTrip(int trainId, const QString &travelDate, int totalSeats);
    std::optional<TripRecord> findTripById(int tripId) const;
    std::optional<TripRecord> findOrCreateTrip(int trainId,
                                               const QString &travelDate,
                                               int totalSeats);   // 不存在则按车次模板惰性创建
    bool adjustTripSeats(int tripId, int delta);   // 增减余票（购票-1/退票+1），扣票时校验余票充足
    QList<TripRecord> findTripsByTrain(int trainId) const;
    bool updateTrip(const TripRecord &trip);

    // >>> Order APIs（V2：订单经 tripId 关联班次）
    std::optional<int> createOrder(const OrderRecord &order);   // 成功返回新订单ID
    QList<OrderRecord> findOrdersByUser(int userId) const;
    bool updateOrderStatus(int orderId, int status);

    // ── Issue 9：车票查询 ────────────────────────────────────
    // 联查结果：车次 + 当日班次 + 站名（一行即一条可展示的查询结果）
    bool adjustTrainSeats(int trainId, int delta);
    bool beginTransaction();
    bool commitTransaction();
    bool rollbackTransaction();
    struct TrainWithStations {
        int trainId=0,totalSeats=0,remainingSeats=0;
        double basePrice = 0.0;
        QString trainNumber,departureStationName,arrivalStationName;
        QString departureTime,arrivalTime,travelDate;
        int tripId = 0;
        bool enabled=true;
    };
    // 按出发站/到达站/日期联查可售班次
    QList<TrainWithStations> searchTripsByStation(
        const QString &dep,const QString &arr,const QString &date) const;
    QList<TrainWithStations> searchTrainsByStation(
        const QString &dep,const QString &arr,const QString &date) const;

    // ── Issue 10：退票 / 改签 / 订单查询 ─────────────────────
    std::optional<OrderRecord> findOrderById(int orderId) const;
    QList<OrderRecord> findOrdersByPassenger(const QString &name) const;   // 按乘车人姓名查订单

    // >>> Jaime added for CharlesSmartWang, issue 7 & 8:
    QList<TrainRecord> getAllTrains(bool onlyEnabled = true) const;
    bool deleteTrain(int trainId);              // 软删除：置 enabled=0
    bool setTrainEnabled(int trainId, bool enabled);
    bool deleteTrainPermanently(int trainId);   // 物理删除（有关联订单时拒绝）
    QList<TrainRecord> searchTrains(const QString &keyword) const;   // 按车次号关键字模糊搜索
    QList<TrainRecord> searchByStation(int stationId, bool isDeparture = true) const;   // 按出发/到达站查车次

    // ── Issue 11: 订单历史 + 统计 ──────────────────────────
    struct OrderWithDetails {
        int orderId=0,userId=0,tripId=0,trainId=0,status=0;
        QString trainNumber,passengerName,purchaseTime;
        QString departureStationName,arrivalStationName,travelDate;
    };
    QList<OrderWithDetails> findAllOrdersWithDetails() const;
    int countOrdersByStatus(int status) const;   // 按订单状态计数
    int countAllOrders() const;
    struct RouteStat { QString dep,arr; int count=0; };   // 线路统计：出发/到达站及订单数
    QList<RouteStat> popularRoutes(int limit=10) const;   // 热门线路 Top N
    struct MonthlyStat { QString month; int total=0,booked=0,refunded=0; };   // 月度客流：总量/预订/退票
    QList<MonthlyStat> monthlyPassengerFlow() const;

    // 单条操作日志（操作人、动作、详情、时间）
    struct OperationLogRecord {
        int logId = 0;
        QString operatorUsername;
        QString action;
        QString detail;
        QString createdAt;
    };
    bool addOperationLog(const QString &operatorUsername,
                         const QString &action,
                         const QString &detail);
    QList<OperationLogRecord> findAllOperationLogs() const;

private:
    // * This block connect helpers
    // openDatabase() only worries about creating & opening the SQLite file.
    // createTables() only worries about running CREATE TABLE statements.
    
    bool openDatabase();       // 建立 SQLite 连接
    void closeDatabase();
    bool createTables();       // V1 建表
    bool seedDemoData();       // 写入演示数据
    bool executeStatement(const QString &sql);   // 执行单条 SQL 语句
    QString resolveDatabasePath() const;         // 解析数据库文件存放路径

    // * This block
    // we store these as class state so initialization can be called cleanly and
    // later modules can ask what happened without doing SQL themselves.
    QString m_connectionName;      // Qt 数据库连接名
    QString m_databasePath;        // 数据库文件路径
    mutable QString m_lastError;   // 最近一次错误信息
    bool m_wasCreated = false;     // 本次运行是否新建了数据库文件
};
