#include "database_manager.h"

#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QString>

namespace {
int loadRepresentativeRemainingSeats(const QString &connectionName,
                                     int trainId,
                                     int fallbackSeats)
{
    QSqlQuery query(QSqlDatabase::database(connectionName));
    query.prepare(QStringLiteral(
        "SELECT remainingSeats "
        "FROM Trip WHERE trainId = :trainId AND enabled = 1 "
        "ORDER BY travelDate ASC, departureTime ASC LIMIT 1"
    ));
    query.bindValue(":trainId", trainId);
    if (!query.exec() || !query.next()) {
        return fallbackSeats;
    }
    return query.value(0).toInt();
}
}

bool DatabaseManager::addTrain(const TrainRecord &train) {
    // V2: 新增 Train 只写车次模板，不直接写余票；余票在 Trip 中按日维护
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

// 按车次ID查询，未找到返回 nullopt
std::optional<TrainRecord> DatabaseManager::findTrainById(int trainId) const {
    m_lastError.clear();
    QSqlQuery query(QSqlDatabase::database(m_connectionName));

    query.prepare(QStringLiteral(
        "SELECT trainId, trainNumber, departureStationId, arrivalStationId, "
        "departureTime, arrivalTime, totalSeats, enabled "
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
    record.remainingSeats = loadRepresentativeRemainingSeats(
        m_connectionName, record.trainId, record.totalSeats);
    record.enabled = query.value(7).toBool();

    return record;
}

// 按车次号（如 G101）查询，未找到返回 nullopt
std::optional<TrainRecord> DatabaseManager::findTrainByNumber(
    const QString &trainNumber
) const {
    m_lastError.clear();
    QSqlQuery query(QSqlDatabase::database(m_connectionName));

    query.prepare(QStringLiteral(
        "SELECT trainId, trainNumber, departureStationId, arrivalStationId, "
        "departureTime, arrivalTime, totalSeats, enabled "
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
    record.remainingSeats = loadRepresentativeRemainingSeats(
        m_connectionName, record.trainId, record.totalSeats);
    record.enabled = query.value(7).toBool();

    return record;
}

// 按ID整体更新车次信息，返回是否有行被修改
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

// 获取全部车次；onlyEnabled=true 时只返回启用（未软删除）的车次
QList<TrainRecord> DatabaseManager::getAllTrains(bool onlyEnabled) const {
    m_lastError.clear();
    QSqlQuery query(QSqlDatabase::database(m_connectionName));

    QString sql = QStringLiteral(
        "SELECT trainId, trainNumber, departureStationId, arrivalStationId, "
        "departureTime, arrivalTime, totalSeats, enabled "
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
        record.remainingSeats = loadRepresentativeRemainingSeats(
            m_connectionName, record.trainId, record.totalSeats);
        record.enabled = query.value(7).toBool();
        results.append(record);
    }

    return results;
}

// 软删除车次：仅置 enabled=0，保留数据以便订单等历史记录可追溯
bool DatabaseManager::deleteTrain(int trainId) {
    m_lastError.clear();
    QSqlQuery query(QSqlDatabase::database(m_connectionName));

    // 这一步对应管理端的“停运”：不删记录，只禁止该车次继续对外可用。
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

// 设置车次启用/停用状态
bool DatabaseManager::setTrainEnabled(int trainId, bool enabled) {
    m_lastError.clear();
    QSqlQuery query(QSqlDatabase::database(m_connectionName));

    // “恢复运营”走同一字段：enabled=1；停运则是 enabled=0。
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

// 永久删除车次：物理删除 Train 及其 Trip；有订单时拒绝，防止破坏历史数据
bool DatabaseManager::deleteTrainPermanently(int trainId) {
    m_lastError.clear();
    QSqlQuery query(QSqlDatabase::database(m_connectionName));

    query.prepare(QStringLiteral(
        "SELECT COUNT(*) "
        "FROM \"Order\" o "
        "JOIN Trip tr ON o.tripId = tr.tripId "
        "WHERE tr.trainId = :trainId"
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

    QSqlQuery deleteTrips(QSqlDatabase::database(m_connectionName));
    deleteTrips.prepare(QStringLiteral(
        "DELETE FROM Trip WHERE trainId = :trainId"
    ));
    deleteTrips.bindValue(":trainId", trainId);
    if (!deleteTrips.exec()) {
        m_lastError = deleteTrips.lastError().text();
        return false;
    }

    QSqlQuery deleteTrainQuery(QSqlDatabase::database(m_connectionName));
    deleteTrainQuery.prepare(QStringLiteral(
        "DELETE FROM Train WHERE trainId = :trainId"
    ));
    deleteTrainQuery.bindValue(":trainId", trainId);

    if (!deleteTrainQuery.exec()) {
        m_lastError = deleteTrainQuery.lastError().text();
        return false;
    }

    if (deleteTrainQuery.numRowsAffected() == 0) {
        m_lastError = QStringLiteral("No train found for the given trainId.");
        return false;
    }

    return true;
}

// 关键字模糊搜索启用中的车次（匹配车次号或起讫站名）
QList<TrainRecord> DatabaseManager::searchTrains(const QString &keyword) const {
    m_lastError.clear();
    QSqlQuery query(QSqlDatabase::database(m_connectionName));

    query.prepare(QStringLiteral(
        "SELECT t.trainId, t.trainNumber, t.departureStationId, "
        "t.arrivalStationId, t.departureTime, t.arrivalTime, "
        "t.totalSeats, t.enabled "
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
        record.remainingSeats = loadRepresentativeRemainingSeats(
            m_connectionName, record.trainId, record.totalSeats);
        record.enabled = query.value(7).toBool();
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
        "departureTime, arrivalTime, totalSeats, enabled "
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
        record.remainingSeats = loadRepresentativeRemainingSeats(
            m_connectionName, record.trainId, record.totalSeats);
        record.enabled = query.value(7).toBool();
        results.append(record);
    }

    return results;
}
