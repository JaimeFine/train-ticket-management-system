#include "database_manager.h"

#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QString>

bool DatabaseManager::addTrain(const TrainRecord &train) {
    QSqlQuery query(QSqlDatabase::database(m_connectionName));

    query.prepare(QStringLiteral(
        "INSERT INTO Train ("
        "trainNumber, departureStationId, arrivalStationId, departureTime, "
        "arrivalTime, totalSeats, remainingSeats, enabled"
        ") "
        "VALUES ("
        ":trainNumber, :departureStationId, :arrivalStationId, :departureTime, "
        ":arrivalTime, :totalSeats, :remainingSeats, :enabled"
        ")"
    ));

    query.bindValue(":trainNumber", train.trainNumber);
    query.bindValue(":departureStationId", train.departureStationId);
    query.bindValue(":arrivalStationId", train.arrivalStationId);
    query.bindValue(":departureTime", train.departureTime);
    query.bindValue(":arrivalTime", train.arrivalTime);
    query.bindValue(":totalSeats", train.totalSeats);
    query.bindValue(":remainingSeats", train.remainingSeats);
    query.bindValue(":enabled", train.enabled);

    if (!query.exec()) {
        m_lastError = query.lastError().text();
        return false;
    }

    return true;
}

std::optional<TrainRecord> DatabaseManager::findTrainById(int trainId) const {
    m_lastError.clear();
    QSqlQuery query(QSqlDatabase::database(m_connectionName));

    query.prepare(QStringLiteral(
        "SELECT trainId, trainNumber, departureStationId, arrivalStationId, "
        "departureTime, arrivalTime, totalSeats, remainingSeats, enabled "
        "FROM Train "
        "WHERE trainId = :trainId"
    ));

    query.bindValue(":trainId", trainId);

    if (!query.exec()) {
        m_lastError = query.lastError().text();
        return std::nullopt;
    }

    if (!query.next()) {
        return std::nullopt;
    }

    TrainRecord record;
    record.trainId = query.value(0).toInt();
    record.trainNumber = query.value(1).toString();
    record.departureStationId = query.value(2).toInt();
    record.arrivalStationId = query.value(3).toInt();
    record.departureTime = query.value(4).toString();
    record.arrivalTime = query.value(5).toString();
    record.totalSeats = query.value(6).toInt();
    record.remainingSeats = query.value(7).toInt();
    record.enabled = query.value(8).toBool();

    return record;
}

std::optional<TrainRecord> DatabaseManager::findTrainByNumber(
    const QString &trainNumber
) const {
    m_lastError.clear();
    QSqlQuery query(QSqlDatabase::database(m_connectionName));

    query.prepare(QStringLiteral(
        "SELECT trainId, trainNumber, departureStationId, arrivalStationId, "
        "departureTime, arrivalTime, totalSeats, remainingSeats, enabled "
        "FROM Train "
        "WHERE trainNumber = :trainNumber"
    ));

    query.bindValue(":trainNumber", trainNumber);

    if (!query.exec()) {
        m_lastError = query.lastError().text();
        return std::nullopt;
    }

    if (!query.next()) {
        return std::nullopt;
    }

    TrainRecord record;
    record.trainId = query.value(0).toInt();
    record.trainNumber = query.value(1).toString();
    record.departureStationId = query.value(2).toInt();
    record.arrivalStationId = query.value(3).toInt();
    record.departureTime = query.value(4).toString();
    record.arrivalTime = query.value(5).toString();
    record.totalSeats = query.value(6).toInt();
    record.remainingSeats = query.value(7).toInt();
    record.enabled = query.value(8).toBool();

    return record;
}

bool DatabaseManager::updateTrain(const TrainRecord &train) {
    m_lastError.clear();
    QSqlQuery query(QSqlDatabase::database(m_connectionName));

    query.prepare(QStringLiteral(
        "UPDATE Train "
        "SET trainNumber = :trainNumber, "
        "   departureStationId = :departureStationId, "
        "   arrivalStationId = :arrivalStationId, "
        "   departureTime = :departureTime, "
        "   arrivalTime = :arrivalTime, "
        "   totalSeats = :totalSeats, "
        "   remainingSeats = :remainingSeats, "
        "   enabled = :enabled "
        "WHERE trainId = :trainId"
    ));

    query.bindValue(":trainId", train.trainId);
    query.bindValue(":trainNumber", train.trainNumber);
    query.bindValue(":departureStationId", train.departureStationId);
    query.bindValue(":arrivalStationId", train.arrivalStationId);
    query.bindValue(":departureTime", train.departureTime);
    query.bindValue(":arrivalTime", train.arrivalTime);
    query.bindValue(":totalSeats", train.totalSeats);
    query.bindValue(":remainingSeats", train.remainingSeats);
    query.bindValue(":enabled", train.enabled);

    if (!query.exec()) {
        m_lastError = query.lastError().text();
        return false;
    }

    // Check if any row was actually updated
    if (query.numRowsAffected() == 0) {
        return false;
    }

    return true;
}

QList<TrainRecord> DatabaseManager::getAllTrains(bool onlyEnabled) const {
    m_lastError.clear();
    QSqlQuery query(QSqlDatabase::database(m_connectionName));

    QString sql = QStringLiteral(
        "SELECT trainId, trainNumber, departureStationId, arrivalStationId, "
        "departureTime, arrivalTime, totalSeats, remainingSeats, enabled "
        "FROM Train"
    );

    if (onlyEnabled) {
        sql += QStringLiteral(" WHERE enabled = 1");
    }

    sql += QStringLiteral(" ORDER BY trainId");

    QList<TrainRecord> results;
    if (!query.prepare(sql)) {
        m_lastError = query.lastError().text();
        return results;
    }

    if (!query.exec()) {
        m_lastError = query.lastError().text();
        return results;
    }

    while (query.next()) {
        TrainRecord record;
        record.trainId = query.value(0).toInt();
        record.trainNumber = query.value(1).toString();
        record.departureStationId = query.value(2).toInt();
        record.arrivalStationId = query.value(3).toInt();
        record.departureTime = query.value(4).toString();
        record.arrivalTime = query.value(5).toString();
        record.totalSeats = query.value(6).toInt();
        record.remainingSeats = query.value(7).toInt();
        record.enabled = query.value(8).toBool();
        results.append(record);
    }

    return results;
}

bool DatabaseManager::deleteTrain(int trainId) {
    m_lastError.clear();
    QSqlQuery query(QSqlDatabase::database(m_connectionName));

    query.prepare(QStringLiteral(
        "UPDATE Train "
        "SET enabled = 0 "
        "WHERE trainId = :trainId AND enabled = 1"
    ));

    query.bindValue(":trainId", trainId);

    if (!query.exec()) {
        m_lastError = query.lastError().text();
        return false;
    }

    if (query.numRowsAffected() == 0) {
        m_lastError = QStringLiteral("No enabled train found for the given trainId.");
        return false;
    }

    return true;
}

bool DatabaseManager::setTrainEnabled(int trainId, bool enabled) {
    m_lastError.clear();
    QSqlQuery query(QSqlDatabase::database(m_connectionName));

    query.prepare(QStringLiteral(
        "UPDATE Train SET enabled = :enabled WHERE trainId = :trainId"
    ));
    query.bindValue(":enabled", enabled ? 1 : 0);
    query.bindValue(":trainId", trainId);

    if (!query.exec()) {
        m_lastError = query.lastError().text();
        return false;
    }

    if (query.numRowsAffected() == 0) {
        m_lastError = QStringLiteral("No train found for the given trainId.");
        return false;
    }

    return true;
}

bool DatabaseManager::deleteTrainPermanently(int trainId) {
    m_lastError.clear();
    QSqlQuery query(QSqlDatabase::database(m_connectionName));

    query.prepare(QStringLiteral(
        "SELECT COUNT(*) FROM \"Order\" WHERE trainId = :trainId"
    ));
    query.bindValue(":trainId", trainId);

    if (!query.exec()) {
        m_lastError = query.lastError().text();
        return false;
    }

    if (query.next() && query.value(0).toInt() > 0) {
        m_lastError = QStringLiteral("该车次已有订单记录，不能物理删除。");
        return false;
    }

    QSqlQuery deleteQuery(QSqlDatabase::database(m_connectionName));
    deleteQuery.prepare(QStringLiteral(
        "DELETE FROM Train WHERE trainId = :trainId"
    ));
    deleteQuery.bindValue(":trainId", trainId);

    if (!deleteQuery.exec()) {
        m_lastError = deleteQuery.lastError().text();
        return false;
    }

    if (deleteQuery.numRowsAffected() == 0) {
        m_lastError = QStringLiteral("No train found for the given trainId.");
        return false;
    }

    return true;
}

QList<TrainRecord> DatabaseManager::searchTrains(const QString &keyword) const {
    m_lastError.clear();
    QSqlQuery query(QSqlDatabase::database(m_connectionName));

    query.prepare(QStringLiteral(
        "SELECT t.trainId, t.trainNumber, t.departureStationId, "
        "t.arrivalStationId, t.departureTime, t.arrivalTime, "
        "t.totalSeats, t.remainingSeats, t.enabled "
        "FROM Train t "
        "LEFT JOIN Station s1 ON t.departureStationId = s1.stationId "
        "LEFT JOIN Station s2 ON t.arrivalStationId = s2.stationId "
        "WHERE t.enabled = 1 "
        "  AND (t.trainNumber LIKE :keyword "
        "       OR s1.stationName LIKE :keyword "
        "       OR s2.stationName LIKE :keyword) "
        "ORDER BY t.trainId"
    ));

    query.bindValue(
        ":keyword", QStringLiteral("%") + keyword + QStringLiteral("%")
    );

    QList<TrainRecord> results;
    if (!query.exec()) {
        m_lastError = query.lastError().text();
        return results;
    }

    while (query.next()) {
        TrainRecord record;
        record.trainId = query.value(0).toInt();
        record.trainNumber = query.value(1).toString();
        record.departureStationId = query.value(2).toInt();
        record.arrivalStationId = query.value(3).toInt();
        record.departureTime = query.value(4).toString();
        record.arrivalTime = query.value(5).toString();
        record.totalSeats = query.value(6).toInt();
        record.remainingSeats = query.value(7).toInt();
        record.enabled = query.value(8).toBool();
        results.append(record);
    }

    return results;
}

QList<TrainRecord> DatabaseManager::searchByStation(
    int stationId, bool isDeparture
) const {
    m_lastError.clear();
    QSqlQuery query(QSqlDatabase::database(m_connectionName));

    QString sql = QStringLiteral(
        "SELECT trainId, trainNumber, departureStationId, arrivalStationId, "
        "departureTime, arrivalTime, totalSeats, remainingSeats, enabled "
        "FROM Train WHERE "
    );

    if (isDeparture) {
        sql += QStringLiteral("departureStationId = :stationId");
    } else {
        sql += QStringLiteral("arrivalStationId = :stationId");
    }

    sql += QStringLiteral(" AND enabled = 1 ORDER BY trainId");

    if (!query.prepare(sql)) {
        m_lastError = query.lastError().text();
        return {};
    }

    query.bindValue(":stationId", stationId);

    QList<TrainRecord> results;
    if (!query.exec()) {
        m_lastError = query.lastError().text();
        return results;
    }

    while (query.next()) {
        TrainRecord record;
        record.trainId = query.value(0).toInt();
        record.trainNumber = query.value(1).toString();
        record.departureStationId = query.value(2).toInt();
        record.arrivalStationId = query.value(3).toInt();
        record.departureTime = query.value(4).toString();
        record.arrivalTime = query.value(5).toString();
        record.totalSeats = query.value(6).toInt();
        record.remainingSeats = query.value(7).toInt();
        record.enabled = query.value(8).toBool();
        results.append(record);
    }

    return results;
}
