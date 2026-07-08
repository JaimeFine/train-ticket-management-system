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
