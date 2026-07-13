#include "database_manager.h"

#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QString>

bool DatabaseManager::addStation(const StationRecord &station) {
    QSqlQuery query(QSqlDatabase::database(m_connectionName));

    query.prepare(QStringLiteral(
        "INSERT INTO Station (stationName) "
        "VALUES (:stationName)"
    ));

    query.bindValue(":stationName", station.stationName);

    if (!query.exec()) {
        m_lastError = query.lastError().text();
        return false;
    }

    return true;
}

std::optional<StationRecord> DatabaseManager::findStationById(
    int stationId
) const {
    m_lastError.clear();
    QSqlQuery query(QSqlDatabase::database(m_connectionName));

    query.prepare(QStringLiteral(
        "SELECT stationId, stationName "
        "FROM Station "
        "WHERE stationId = :stationId"
    ));

    query.bindValue(":stationId", stationId);

    if (!query.exec()) {
        m_lastError = query.lastError().text();
        return std::nullopt;
    }

    if (!query.next()) {
        return std::nullopt;
    }

    StationRecord record;
    record.stationId = query.value(0).toInt();
    record.stationName = query.value(1).toString();

    return record;
}

std::optional<StationRecord> DatabaseManager::findStationByName(
    const QString &stationName
) const {
    m_lastError.clear();
    QSqlQuery query(QSqlDatabase::database(m_connectionName));

    query.prepare(QStringLiteral(
        "SELECT stationId, stationName "
        "FROM Station "
        "WHERE stationName = :stationName"
    ));

    query.bindValue(":stationName", stationName);

    if (!query.exec()) {
        m_lastError = query.lastError().text();
        return std::nullopt;
    }

    if (!query.next()) {
        return std::nullopt;
    }

    StationRecord record;
    record.stationId = query.value(0).toInt();
    record.stationName = query.value(1).toString();

    return record;
}

QList<StationRecord> DatabaseManager::getAllStations() const {
    m_lastError.clear();
    QSqlQuery query(QSqlDatabase::database(m_connectionName));

    query.prepare(QStringLiteral(
        "SELECT stationId, stationName "
        "FROM Station "
        "ORDER BY stationName"
    ));

    QList<StationRecord> results;
    if (!query.exec()) {
        m_lastError = query.lastError().text();
        return results;
    }

    while (query.next()) {
        StationRecord record;
        record.stationId = query.value(0).toInt();
        record.stationName = query.value(1).toString();
        results.append(record);
    }

    return results;
}

bool DatabaseManager::deleteStation(int stationId) {
    m_lastError.clear();
    QSqlQuery query(QSqlDatabase::database(m_connectionName));

    query.prepare(QStringLiteral(
        "SELECT trainId "
        "FROM Train "
        "WHERE departureStationId = :stationId OR arrivalStationId = :stationId"
    ));

    query.bindValue(":stationId", stationId);

    if (!query.exec()) {
        m_lastError = query.lastError().text();
        return false;
    }

    if (query.next()) {
        m_lastError = QStringLiteral("该站点被车次引用，不能删除");
        return false;
    }

    query.prepare(QStringLiteral(
        "DELETE FROM Station "
        "WHERE stationId = :stationId"
    ));

    query.bindValue(":stationId", stationId);

    if (!query.exec()) {
        m_lastError = query.lastError().text();
        return false;
    }

    if (query.numRowsAffected() == 0) {
        m_lastError = QStringLiteral("未找到要删除的站点");
        return false;
    }

    return true;
}
