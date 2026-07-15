#include "database_manager.h"

#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QString>

// 新增车次记录
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

// 按车次ID查询，未找到返回 nullopt
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

// 按车次号（如 G101）查询，未找到返回 nullopt
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

// 按ID整体更新车次信息，返回是否有行被修改
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

// 获取全部车次；onlyEnabled=true 时只返回启用（未软删除）的车次
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

// 软删除车次：仅置 enabled=0，保留数据以便订单等历史记录可追溯
bool DatabaseManager::deleteTrain(int trainId) {
    m_lastError.clear();
    QSqlQuery q(QSqlDatabase::database(m_connectionName));
    // 仅对当前启用的车次生效，重复删除视为失败
    q.prepare("UPDATE Train SET enabled=0 WHERE trainId=:id AND enabled=1");
    q.bindValue(":id", trainId);
    if (!q.exec()) { m_lastError = q.lastError().text(); return false; }
    if (q.numRowsAffected() == 0) {
        m_lastError = QStringLiteral("No enabled train found for the given trainId.");
        return false;
    }
    return true;
}

// 设置车次启用/停用状态
bool DatabaseManager::setTrainEnabled(int trainId, bool enabled) {
    m_lastError.clear();
    QSqlQuery q(QSqlDatabase::database(m_connectionName));
    q.prepare("UPDATE Train SET enabled=:en WHERE trainId=:id");
    q.bindValue(":en", enabled ? 1 : 0);
    q.bindValue(":id", trainId);
    if (!q.exec()) { m_lastError = q.lastError().text(); return false; }
    return q.numRowsAffected() > 0;
}

// 永久删除车次：物理删除 Train 及其 Trip；有订单时拒绝，防止破坏历史数据
bool DatabaseManager::deleteTrainPermanently(int trainId) {
    m_lastError.clear();
    QSqlQuery q(QSqlDatabase::database(m_connectionName));
    // V2 架构：订单挂在 Trip 下，需经 Trip 关联到车次统计订单数
    q.prepare("SELECT COUNT(*) FROM \"Order\" o JOIN Trip t ON o.tripId=t.tripId WHERE t.trainId=:id");
    q.bindValue(":id", trainId);
    if (!q.exec()) { m_lastError = q.lastError().text(); return false; }
    if (q.next() && q.value(0).toInt() > 0) {
        m_lastError = QStringLiteral("该车次已有订单记录，不能物理删除。");
        return false;
    }
    // 先删除该车次下的所有班次（外键依赖）
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

// 关键字模糊搜索启用中的车次（匹配车次号或起讫站名）
QList<TrainRecord> DatabaseManager::searchTrains(const QString &keyword) const {
    m_lastError.clear();
    QSqlQuery q(QSqlDatabase::database(m_connectionName));
    // LEFT JOIN 站点表两次以分别取出发站/到达站名称参与模糊匹配
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

// 按站点查询启用中的车次；isDeparture 决定按出发站还是到达站过滤
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
