#pragma once

#include <QString>
#include <optional>
#include <QList>
#include "database/order_record.h"
#include "database/station_record.h"
#include "database/user_record.h"
#include "database/train_record.h"

class QSqlDatabase;

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

    // Utility methods:
    bool isOpen() const;
    bool wasCreated() const;
    QString databasePath() const;
    QString lastError() const;

    // >>> User APIs
    bool addUser(const UserRecord &user);
    std::optional<UserRecord> findUserById(int userId) const;
    std::optional<UserRecord> findUserByUsername(const QString &username) const;
    bool updateUser(const UserRecord &user);
    bool setUserEnabled(int userId, bool enabled);

    // >>> Station APIs
    bool addStation(const StationRecord &station);
    std::optional<StationRecord> findStationById(int stationId) const;
    std::optional<StationRecord> findStationByName(
        const QString &stationName
    ) const;

    // >>> Train APIs
    bool addTrain(const TrainRecord &train);
    std::optional<TrainRecord> findTrainById(int trainId) const;
    std::optional<TrainRecord> findTrainByNumber(const QString &trainNumber) const;
    bool updateTrain(const TrainRecord &train);

    // >>> Order APIs
    bool createOrder(const OrderRecord &order);
    QList<OrderRecord> findOrdersByUser(int userId) const;
    bool updateOrderStatus(int orderId, int status);

    // ── Issue 9: 订票 + 余票查询 + 车次搜索 ─────────────────
    bool adjustTrainSeats(int trainId, int delta);
    bool beginTransaction();
    bool commitTransaction();
    bool rollbackTransaction();
    struct TrainWithStations {
        int trainId=0,totalSeats=0,remainingSeats=0;
        QString trainNumber,departureStationName,arrivalStationName;
        QString departureTime,arrivalTime; bool enabled=true;
    };
    QList<TrainWithStations> searchTrainsByStation(
        const QString &dep,const QString &arr,const QString &date) const;

private:
    // * This block connect helpers
    // openDatabase() only worries about creating & opening the SQLite file.
    // createTables() only worries about running CREATE TABLE statements.
    
    bool openDatabase();
    void closeDatabase();
    bool createTables();
    bool executeStatement(const QString &sql);
    QString resolveDatabasePath() const;

    // * This block
    // we store these as class state so initialization can be called cleanly and
    // later modules can ask what happened without doing SQL themselves.
    QString m_connectionName;
    QString m_databasePath;
    mutable QString m_lastError;
    bool m_wasCreated = false;
};