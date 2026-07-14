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
        "arrivalTime, totalSeats, enabled"
        ") "
        "VALUES ("
        ":trainNumber, :departureStationId, :arrivalStationId, :departureTime, "
        ":arrivalTime, :totalSeats, :enabled"
        ")"
    ));

    query.bindValue(":trainNumber", train.trainNumber);
    query.bindValue(":departureStationId", train.departureStationId);
    query.bindValue(":arrivalStationId", train.arrivalStationId);
    query.bindValue(":departureTime", train.departureTime);
    query.bindValue(":arrivalTime", train.arrivalTime);
    query.bindValue(":totalSeats", train.totalSeats);
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
        "departureTime, arrivalTime, totalSeats, enabled "
        "FROM Train WHERE trainId = :trainId"
    ));
    query.bindValue(":trainId", trainId);

    if (!query.exec()) { m_lastError = query.lastError().text(); return std::nullopt; }
    if (!query.next()) return std::nullopt;

    TrainRecord record;
    record.trainId = query.value(0).toInt();
    record.trainNumber = query.value(1).toString();
    record.departureStationId = query.value(2).toInt();
    record.arrivalStationId = query.value(3).toInt();
    record.departureTime = query.value(4).toString();
    record.arrivalTime = query.value(5).toString();
    record.totalSeats = query.value(6).toInt();
    record.enabled = query.value(7).toBool();
    return record;
}

std::optional<TrainRecord> DatabaseManager::findTrainByNumber(
    const QString &trainNumber) const {
    m_lastError.clear();
    QSqlQuery query(QSqlDatabase::database(m_connectionName));

    query.prepare(QStringLiteral(
        "SELECT trainId, trainNumber, departureStationId, arrivalStationId, "
        "departureTime, arrivalTime, totalSeats, enabled "
        "FROM Train WHERE trainNumber = :trainNumber"
    ));
    query.bindValue(":trainNumber", trainNumber);

    if (!query.exec()) { m_lastError = query.lastError().text(); return std::nullopt; }
    if (!query.next()) return std::nullopt;

    TrainRecord record;
    record.trainId = query.value(0).toInt();
    record.trainNumber = query.value(1).toString();
    record.departureStationId = query.value(2).toInt();
    record.arrivalStationId = query.value(3).toInt();
    record.departureTime = query.value(4).toString();
    record.arrivalTime = query.value(5).toString();
    record.totalSeats = query.value(6).toInt();
    record.enabled = query.value(7).toBool();
    return record;
}

bool DatabaseManager::updateTrain(const TrainRecord &train) {
    m_lastError.clear();
    QSqlQuery query(QSqlDatabase::database(m_connectionName));

    query.prepare(QStringLiteral(
        "UPDATE Train SET trainNumber=:trainNumber, "
        "departureStationId=:departureStationId, arrivalStationId=:arrivalStationId, "
        "departureTime=:departureTime, arrivalTime=:arrivalTime, "
        "totalSeats=:totalSeats, enabled=:enabled "
        "WHERE trainId=:trainId"
    ));
    query.bindValue(":trainId", train.trainId);
    query.bindValue(":trainNumber", train.trainNumber);
    query.bindValue(":departureStationId", train.departureStationId);
    query.bindValue(":arrivalStationId", train.arrivalStationId);
    query.bindValue(":departureTime", train.departureTime);
    query.bindValue(":arrivalTime", train.arrivalTime);
    query.bindValue(":totalSeats", train.totalSeats);
    query.bindValue(":enabled", train.enabled);

    if (!query.exec()) { m_lastError = query.lastError().text(); return false; }
    return query.numRowsAffected() > 0;
}

QList<TrainRecord> DatabaseManager::getAllTrains(bool onlyEnabled) const {
    m_lastError.clear();
    QSqlQuery query(QSqlDatabase::database(m_connectionName));

    QString sql = QStringLiteral(
        "SELECT trainId,trainNumber,departureStationId,arrivalStationId,"
        "departureTime,arrivalTime,totalSeats,enabled FROM Train");
    if (onlyEnabled) sql += QStringLiteral(" WHERE enabled=1");
    sql += QStringLiteral(" ORDER BY trainId");

    QList<TrainRecord> results;
    if (!query.exec(sql)) { m_lastError = query.lastError().text(); return results; }

    while (query.next()) {
        TrainRecord r;
        r.trainId=query.value(0).toInt(); r.trainNumber=query.value(1).toString();
        r.departureStationId=query.value(2).toInt(); r.arrivalStationId=query.value(3).toInt();
        r.departureTime=query.value(4).toString(); r.arrivalTime=query.value(5).toString();
        r.totalSeats=query.value(6).toInt(); r.enabled=query.value(7).toBool();
        results.append(r);
    }
    return results;
}

bool DatabaseManager::deleteTrain(int trainId) {
    m_lastError.clear();
    QSqlQuery q(QSqlDatabase::database(m_connectionName));
    q.prepare("UPDATE Train SET enabled=0 WHERE trainId=:id AND enabled=1");
    q.bindValue(":id", trainId);
    if (!q.exec()) { m_lastError = q.lastError().text(); return false; }
    if (q.numRowsAffected() == 0) {
        m_lastError = QStringLiteral("No enabled train found for the given trainId.");
        return false;
    }
    return true;
}

bool DatabaseManager::setTrainEnabled(int trainId, bool enabled) {
    m_lastError.clear();
    QSqlQuery q(QSqlDatabase::database(m_connectionName));
    q.prepare("UPDATE Train SET enabled=:en WHERE trainId=:id");
    q.bindValue(":en", enabled ? 1 : 0);
    q.bindValue(":id", trainId);
    if (!q.exec()) { m_lastError = q.lastError().text(); return false; }
    return q.numRowsAffected() > 0;
}

bool DatabaseManager::deleteTrainPermanently(int trainId) {
    m_lastError.clear();
    QSqlQuery q(QSqlDatabase::database(m_connectionName));
    // V2: check through Trip
    q.prepare("SELECT COUNT(*) FROM \"Order\" o JOIN Trip t ON o.tripId=t.tripId WHERE t.trainId=:id");
    q.bindValue(":id", trainId);
    if (!q.exec()) { m_lastError = q.lastError().text(); return false; }
    if (q.next() && q.value(0).toInt() > 0) {
        m_lastError = QStringLiteral("该车次已有订单记录，不能物理删除。");
        return false;
    }
    // Delete trips first
    QSqlQuery dq(QSqlDatabase::database(m_connectionName));
    dq.prepare("DELETE FROM Trip WHERE trainId=:id");
    dq.bindValue(":id", trainId);
    dq.exec();

    QSqlQuery dq2(QSqlDatabase::database(m_connectionName));
    dq2.prepare("DELETE FROM Train WHERE trainId=:id");
    dq2.bindValue(":id", trainId);
    if (!dq2.exec()) { m_lastError = dq2.lastError().text(); return false; }
    return dq2.numRowsAffected() > 0;
}

QList<TrainRecord> DatabaseManager::searchTrains(const QString &keyword) const {
    m_lastError.clear();
    QSqlQuery q(QSqlDatabase::database(m_connectionName));
    q.prepare(QStringLiteral(
        "SELECT t.trainId,t.trainNumber,t.departureStationId,t.arrivalStationId,"
        "t.departureTime,t.arrivalTime,t.totalSeats,t.enabled "
        "FROM Train t "
        "LEFT JOIN Station s1 ON t.departureStationId=s1.stationId "
        "LEFT JOIN Station s2 ON t.arrivalStationId=s2.stationId "
        "WHERE t.enabled=1 AND (t.trainNumber LIKE :kw "
        "OR s1.stationName LIKE :kw OR s2.stationName LIKE :kw) ORDER BY t.trainId"));
    q.bindValue(":kw", "%" + keyword + "%");

    QList<TrainRecord> results;
    if (!q.exec()) { m_lastError = q.lastError().text(); return results; }
    while (q.next()) {
        TrainRecord r;
        r.trainId=q.value(0).toInt(); r.trainNumber=q.value(1).toString();
        r.departureStationId=q.value(2).toInt(); r.arrivalStationId=q.value(3).toInt();
        r.departureTime=q.value(4).toString(); r.arrivalTime=q.value(5).toString();
        r.totalSeats=q.value(6).toInt(); r.enabled=q.value(7).toBool();
        results.append(r);
    }
    return results;
}

QList<TrainRecord> DatabaseManager::searchByStation(int stationId, bool isDeparture) const {
    m_lastError.clear();
    QSqlQuery q(QSqlDatabase::database(m_connectionName));
    QString sql = QStringLiteral(
        "SELECT trainId,trainNumber,departureStationId,arrivalStationId,"
        "departureTime,arrivalTime,totalSeats,enabled FROM Train WHERE ");
    sql += isDeparture ? QStringLiteral("departureStationId=:sid")
                       : QStringLiteral("arrivalStationId=:sid");
    sql += QStringLiteral(" AND enabled=1 ORDER BY trainId");
    q.prepare(sql);
    q.bindValue(":sid", stationId);

    QList<TrainRecord> results;
    if (!q.exec()) { m_lastError = q.lastError().text(); return results; }
    while (q.next()) {
        TrainRecord r;
        r.trainId=q.value(0).toInt(); r.trainNumber=q.value(1).toString();
        r.departureStationId=q.value(2).toInt(); r.arrivalStationId=q.value(3).toInt();
        r.departureTime=q.value(4).toString(); r.arrivalTime=q.value(5).toString();
        r.totalSeats=q.value(6).toInt(); r.enabled=q.value(7).toBool();
        results.append(r);
    }
    return results;
}
