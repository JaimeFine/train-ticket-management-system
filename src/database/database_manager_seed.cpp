#include "database_manager.h"

#include <QDateTime>
#include <QMap>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QStringList>
#include <QVariant>

namespace {
    bool execPrepared(QSqlDatabase &database,
                      QString *lastError,
                      const QString &sql,
                      const QList<QVariant> &values = {}) {
        QSqlQuery query(database);
        query.prepare(sql);

        for (const QVariant &value : values) {
            query.addBindValue(value);
        }

        if (!query.exec()) {
            if (lastError != nullptr) {
                *lastError = query.lastError().text();
            }
            return false;
        }

        return true;
    }

    bool tableHasRows(QSqlDatabase &database, const QString &tableName) {
        QSqlQuery query(database);
        if (!query.exec(QStringLiteral("SELECT 1 FROM %1 LIMIT 1").arg(tableName))) {
            return false;
        }

        return query.next();
    }
}   // namespace

bool DatabaseManager::seedDemoData() {
    if (!QSqlDatabase::contains(m_connectionName)) {
        m_lastError = QStringLiteral("Database connection does not exist.");
        return false;
    }

    QSqlDatabase database = QSqlDatabase::database(m_connectionName);
    if (!database.isOpen()) {
        m_lastError = QStringLiteral("Database connection is not open.");
        return false;
    }

    // 已有完整演示数据时直接跳过，避免每次启动都重复跑整套种子写入。
    if (tableHasRows(database, QStringLiteral("User"))
        && tableHasRows(database, QStringLiteral("Station"))
        && tableHasRows(database, QStringLiteral("Train"))
        && tableHasRows(database, QStringLiteral("Trip"))) {
        return true;
    }

    if (!database.transaction()) {
        m_lastError = database.lastError().text();
        return false;
    }

    const struct DemoUserSeed {
        QString username;
        QString password;
        int role;
        int enabled;
    } demoUsers[] = {
        {QStringLiteral("admin"), QStringLiteral("admin123"), 2, 1},
        {QStringLiteral("seller"), QStringLiteral("seller123"), 1, 1},
        {QStringLiteral("user01"), QStringLiteral("user123"), 3, 1},
        {QStringLiteral("user02"), QStringLiteral("user456"), 3, 1}
    };

    for (const DemoUserSeed &user : demoUsers) {
        if (!execPrepared(database,
                          &m_lastError,
                          QStringLiteral(
                              "INSERT OR IGNORE INTO User "
                              "(username, password, role, enabled) "
                              "VALUES (?, ?, ?, ?)"
                              ),
                          {user.username, user.password, user.role, user.enabled})) {
            database.rollback();
            return false;
        }
    }

    const QStringList demoStations = {
        QStringLiteral("北京南"),
        QStringLiteral("南京南"),
        QStringLiteral("上海虹桥"),
        QStringLiteral("杭州东")
    };

    for (const QString &stationName : demoStations) {
        if (!execPrepared(database,
                          &m_lastError,
                          QStringLiteral(
                              "INSERT OR IGNORE INTO Station (stationName) VALUES (?)"
                              ),
                          {stationName})) {
            database.rollback();
            return false;
        }
    }

    const struct DemoTrainSeed {
        QString trainNumber;
        QString departureStationName;
        QString arrivalStationName;
        QString departureTime;
        QString arrivalTime;
        int totalSeats;
        int enabled;
    } demoTrains[] = {
        {QStringLiteral("G1001"), QStringLiteral("北京南"), QStringLiteral("上海虹桥"),
         QStringLiteral("08:00"), QStringLiteral("12:38"), 600, 1},
        {QStringLiteral("G1002"), QStringLiteral("上海虹桥"), QStringLiteral("北京南"),
         QStringLiteral("14:00"), QStringLiteral("18:40"), 600, 1},
        {QStringLiteral("G2001"), QStringLiteral("南京南"), QStringLiteral("杭州东"),
         QStringLiteral("09:15"), QStringLiteral("11:10"), 480, 1},
        {QStringLiteral("G2002"), QStringLiteral("杭州东"), QStringLiteral("南京南"),
         QStringLiteral("15:20"), QStringLiteral("17:18"), 480, 1}
    };

    for (const DemoTrainSeed &train : demoTrains) {
        if (!execPrepared(
                database,
                &m_lastError,
                QStringLiteral(
                    "INSERT OR IGNORE INTO Train ("
                    "trainNumber, departureStationId, arrivalStationId, "
                    "departureTime, arrivalTime, totalSeats, enabled"
                    ") "
                    "SELECT ?, dep.stationId, arr.stationId, ?, ?, ?, ? "
                    "FROM Station dep, Station arr "
                    "WHERE dep.stationName = ? AND arr.stationName = ?"
                    ),
                {
                    train.trainNumber,
                    train.departureTime,
                    train.arrivalTime,
                    train.totalSeats,
                    train.enabled,
                    train.departureStationName,
                    train.arrivalStationName
                })) {
            database.rollback();
            return false;
        }
    }

    const QString today =
        QDateTime::currentDateTime().toString(QStringLiteral("yyyy-MM-dd"));
    const QString tomorrow =
        QDateTime::currentDateTime().addDays(1).toString(QStringLiteral("yyyy-MM-dd"));

    const QMap<QString, double> trainPrices = {
        {QStringLiteral("G1001"), 280.0},
        {QStringLiteral("G1002"), 280.0},
        {QStringLiteral("G2001"), 180.0},
        {QStringLiteral("G2002"), 180.0}
    };

    for (const QString &travelDate : {today, tomorrow}) {
        for (const DemoTrainSeed &train : demoTrains) {
            const double basePrice = trainPrices.value(train.trainNumber, 100.0);

            if (!execPrepared(
                    database,
                    &m_lastError,
                    QStringLiteral(
                        "INSERT OR IGNORE INTO Trip ("
                        "trainId, travelDate, departureTime, arrivalTime, "
                        "totalSeats, remainingSeats, basePrice, enabled"
                        ") "
                        "SELECT trainId, ?, departureTime, arrivalTime, totalSeats, totalSeats, ?, 1 "
                        "FROM Train WHERE trainNumber = ?"
                        ),
                    {travelDate, basePrice, train.trainNumber})) {
                database.rollback();
                return false;
            }
        }
    }

    if (!database.commit()) {
        m_lastError = database.lastError().text();
        database.rollback();
        return false;
    }

    return true;
}
