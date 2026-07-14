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

class DatabaseManager {
public:
    DatabaseManager();
    ~DatabaseManager();

    bool initialize();
    bool initializeV2();   // V2: uses schema_v2.sql

    bool isOpen() const;
    bool wasCreated() const;
    QString databasePath() const;
    QString lastError() const;

    // >>> User APIs
    bool addUser(const UserRecord &user);
    std::optional<UserRecord> findUserById(int userId) const;
    std::optional<UserRecord> findUserByUsername(const QString &username) const;
    QList<UserRecord> findUsersByRole(int role) const;
    bool updateUser(const UserRecord &user);
    bool setUserEnabled(int userId, bool enabled);

    // >>> Station APIs
    bool addStation(const StationRecord &station);
    std::optional<StationRecord> findStationById(int stationId) const;
    std::optional<StationRecord> findStationByName(const QString &stationName) const;
    QList<StationRecord> getAllStations() const;
    bool deleteStation(int stationId);

    // >>> Train APIs (V2: no remainingSeats)
    bool addTrain(const TrainRecord &train);
    std::optional<TrainRecord> findTrainById(int trainId) const;
    std::optional<TrainRecord> findTrainByNumber(const QString &trainNumber) const;
    bool updateTrain(const TrainRecord &train);
    QList<TrainRecord> getAllTrains(bool onlyEnabled = true) const;
    bool deleteTrain(int trainId);
    bool setTrainEnabled(int trainId, bool enabled);
    bool deleteTrainPermanently(int trainId);
    QList<TrainRecord> searchTrains(const QString &keyword) const;
    QList<TrainRecord> searchByStation(int stationId, bool isDeparture = true) const;

    // >>> Trip APIs (V2 新增)
    std::optional<int> createTrip(int trainId, const QString &travelDate, int totalSeats);
    std::optional<TripRecord> findTripById(int tripId) const;
    std::optional<TripRecord> findOrCreateTrip(int trainId, const QString &travelDate, int totalSeats);
    bool adjustTripSeats(int tripId, int delta);
    QList<TripRecord> findTripsByTrain(int trainId) const;

    // >>> Order APIs (V2: tripId)
    std::optional<int> createOrder(const OrderRecord &order);
    QList<OrderRecord> findOrdersByUser(int userId) const;
    bool updateOrderStatus(int orderId, int status);

    // ── Transaction ──────────────────────────────────────────
    bool beginTransaction();
    bool commitTransaction();
    bool rollbackTransaction();

    // ── Issue 9: search ──────────────────────────────────────
    struct TrainWithStations {
        int trainId=0,totalSeats=0;
        QString trainNumber,departureStationName,arrivalStationName;
        QString departureTime,arrivalTime;
        int tripId=0,remainingSeats=0;
        QString travelDate;
        bool enabled=true;
    };
    QList<TrainWithStations> searchTripsByStation(
        const QString &dep,const QString &arr,const QString &date) const;

    // ── Issue 10: refund / change / query ────────────────────
    std::optional<OrderRecord> findOrderById(int orderId) const;
    QList<OrderRecord> findOrdersByPassenger(const QString &name) const;

    // ── Issue 11: 订单历史 + 统计 ────────────────────────────
    struct OrderWithDetails {
        int orderId=0,userId=0,tripId=0,trainId=0,status=0;
        QString trainNumber,passengerName,purchaseTime;
        QString departureStationName,arrivalStationName,travelDate;
    };
    QList<OrderWithDetails> findAllOrdersWithDetails() const;
    int countOrdersByStatus(int status) const;
    int countAllOrders() const;
    struct RouteStat { QString dep,arr; int count=0; };
    QList<RouteStat> popularRoutes(int limit=10) const;
    struct MonthlyStat { QString month; int total=0,booked=0,refunded=0; };
    QList<MonthlyStat> monthlyPassengerFlow() const;

    // ── OperationLog ─────────────────────────────────────────
    struct OperationLogRecord {
        int logId = 0;
        QString operatorUsername;
        QString action;
        QString detail;
        QString createdAt;
    };
    bool addOperationLog(const QString &operatorUsername,
                         const QString &action, const QString &detail);
    QList<OperationLogRecord> findAllOperationLogs() const;

private:
    bool openDatabase();
    void closeDatabase();
    bool createTables();
    bool seedDemoData();
    bool executeStatement(const QString &sql);
    QString resolveDatabasePath() const;

    QString m_connectionName;
    QString m_databasePath;
    mutable QString m_lastError;
    bool m_wasCreated = false;
};
