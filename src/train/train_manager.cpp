// src/train/train_manager.cpp
#include "train_manager.h"
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>

// ---------- 辅助：获取数据库连接 ----------
static QSqlDatabase getDB() {
    return QSqlDatabase::database("train_ticket_connection");
}

// ---------- 返回最后一次操作的状态信息 ----------
QString TrainManager::statusMessage() const {
    return m_lastStatus;
}

// ---------- 获取所有车次 ----------
QVector<Train> TrainManager::getAllTrains(bool onlyEnabled) {
    QVector<Train> result;
    QSqlDatabase db = getDB();
    if (!db.isOpen()) {
        setStatus("数据库未连接");
        return result;
    }

    QString sql = "SELECT trainId, trainNumber, departureStationId, arrivalStationId, "
                  "departureTime, arrivalTime, totalSeats, remainingSeats, enabled "
                  "FROM Train";
    if (onlyEnabled) {
        sql += " WHERE enabled = 1";
    }

    QSqlQuery query(db);
    if (!query.exec(sql)) {
        setStatus("查询车次失败: " + query.lastError().text());
        return result;
    }

    while (query.next()) {
        Train t;
        t.trainId = query.value("trainId").toInt();
        t.trainNumber = query.value("trainNumber").toString();
        t.departureStationId = query.value("departureStationId").toInt();
        t.arrivalStationId = query.value("arrivalStationId").toInt();
        t.departureTime = query.value("departureTime").toString();
        t.arrivalTime = query.value("arrivalTime").toString();
        t.totalSeats = query.value("totalSeats").toInt();
        t.remainingSeats = query.value("remainingSeats").toInt();
        t.enabled = query.value("enabled").toInt() == 1;
        result.append(t);
    }

    setStatus("查询成功，共 " + QString::number(result.size()) + " 条记录");
    return result;
}

// ---------- 添加车次 ----------
bool TrainManager::addTrain(const Train& train) {
    QSqlDatabase db = getDB();
    if (!db.isOpen()) {
        setStatus("数据库未连接");
        return false;
    }

    // 1. 检查车次号是否已存在
    QSqlQuery checkQuery(db);
    checkQuery.prepare("SELECT trainId FROM Train WHERE trainNumber = ?");
    checkQuery.addBindValue(train.trainNumber);
    if (!checkQuery.exec()) {
        setStatus("检查车次号失败: " + checkQuery.lastError().text());
        return false;
    }
    if (checkQuery.next()) {
        setStatus("车次号 " + train.trainNumber + " 已存在");
        return false;
    }

    // 2. 插入新记录
    QSqlQuery query(db);
    query.prepare(
        "INSERT INTO Train (trainNumber, departureStationId, arrivalStationId, "
        "departureTime, arrivalTime, totalSeats, remainingSeats, enabled) "
        "VALUES (?, ?, ?, ?, ?, ?, ?, ?)"
        );
    query.addBindValue(train.trainNumber);
    query.addBindValue(train.departureStationId);
    query.addBindValue(train.arrivalStationId);
    query.addBindValue(train.departureTime);
    query.addBindValue(train.arrivalTime);
    query.addBindValue(train.totalSeats);
    query.addBindValue(train.remainingSeats);
    query.addBindValue(train.enabled ? 1 : 0);

    if (!query.exec()) {
        setStatus("添加车次失败: " + query.lastError().text());
        return false;
    }

    setStatus("车次 " + train.trainNumber + " 添加成功");
    return true;
}

// ---------- 更新车次 ----------
bool TrainManager::updateTrain(const Train& train) {
    QSqlDatabase db = getDB();
    if (!db.isOpen()) {
        setStatus("数据库未连接");
        return false;
    }

    // 1. 检查车次是否存在
    QSqlQuery checkQuery(db);
    checkQuery.prepare("SELECT trainId FROM Train WHERE trainId = ?");
    checkQuery.addBindValue(train.trainId);
    if (!checkQuery.exec()) {
        setStatus("检查车次失败: " + checkQuery.lastError().text());
        return false;
    }
    if (!checkQuery.next()) {
        setStatus("未找到要更新的车次 (ID: " + QString::number(train.trainId) + ")");
        return false;
    }

    // 2. 检查车次号是否与其他记录冲突（排除自身）
    QSqlQuery duplicateQuery(db);
    duplicateQuery.prepare("SELECT trainId FROM Train WHERE trainNumber = ? AND trainId != ?");
    duplicateQuery.addBindValue(train.trainNumber);
    duplicateQuery.addBindValue(train.trainId);
    if (!duplicateQuery.exec()) {
        setStatus("检查车次号失败: " + duplicateQuery.lastError().text());
        return false;
    }
    if (duplicateQuery.next()) {
        setStatus("车次号 " + train.trainNumber + " 已被其他车次使用");
        return false;
    }

    // 3. 更新记录
    QSqlQuery query(db);
    query.prepare(
        "UPDATE Train SET "
        "trainNumber = ?, departureStationId = ?, arrivalStationId = ?, "
        "departureTime = ?, arrivalTime = ?, totalSeats = ?, "
        "remainingSeats = ?, enabled = ? "
        "WHERE trainId = ?"
        );
    query.addBindValue(train.trainNumber);
    query.addBindValue(train.departureStationId);
    query.addBindValue(train.arrivalStationId);
    query.addBindValue(train.departureTime);
    query.addBindValue(train.arrivalTime);
    query.addBindValue(train.totalSeats);
    query.addBindValue(train.remainingSeats);
    query.addBindValue(train.enabled ? 1 : 0);
    query.addBindValue(train.trainId);

    if (!query.exec()) {
        setStatus("更新车次失败: " + query.lastError().text());
        return false;
    }

    if (query.numRowsAffected() == 0) {
        setStatus("更新车次失败，未找到匹配的记录");
        return false;
    }

    setStatus("车次 " + train.trainNumber + " 更新成功");
    return true;
}

// ---------- 逻辑删除（停运）----------
bool TrainManager::deleteTrain(int trainId) {
    QSqlDatabase db = getDB();
    if (!db.isOpen()) {
        setStatus("数据库未连接");
        return false;
    }

    // 1. 检查车次是否存在
    QSqlQuery checkQuery(db);
    checkQuery.prepare("SELECT trainNumber FROM Train WHERE trainId = ?");
    checkQuery.addBindValue(trainId);
    if (!checkQuery.exec()) {
        setStatus("检查车次失败: " + checkQuery.lastError().text());
        return false;
    }
    if (!checkQuery.next()) {
        setStatus("未找到要停运的车次 (ID: " + QString::number(trainId) + ")");
        return false;
    }
    QString trainNumber = checkQuery.value("trainNumber").toString();

    // 2. 检查车次是否已经停运
    if (checkQuery.value("enabled").toInt() == 0) {
        setStatus("车次 " + trainNumber + " 已经处于停运状态");
        return false;
    }

    // 3. 执行逻辑删除（设置 enabled = 0）
    QSqlQuery query(db);
    query.prepare("UPDATE Train SET enabled = 0 WHERE trainId = ?");
    query.addBindValue(trainId);

    if (!query.exec()) {
        setStatus("停运车次失败: " + query.lastError().text());
        return false;
    }

    if (query.numRowsAffected() == 0) {
        setStatus("停运车次失败，未找到匹配的记录");
        return false;
    }

    setStatus("车次 " + trainNumber + " 已停运");
    return true;
}

// ---------- 按关键字搜索车次（车次号/站名模糊匹配）----------
QVector<Train> TrainManager::searchTrains(const QString& keyword) {
    QVector<Train> result;
    QSqlDatabase db = getDB();
    if (!db.isOpen()) {
        setStatus("数据库未连接");
        return result;
    }

    if (keyword.trimmed().isEmpty()) {
        setStatus("搜索关键字不能为空");
        return result;
    }

    // 联表查询：Train + Station（出发站 + 到达站）
    // 匹配：车次号、出发站名、到达站名
    QString sql =
        "SELECT t.trainId, t.trainNumber, t.departureStationId, t.arrivalStationId, "
        "t.departureTime, t.arrivalTime, t.totalSeats, t.remainingSeats, t.enabled "
        "FROM Train t "
        "LEFT JOIN Station s1 ON t.departureStationId = s1.stationId "
        "LEFT JOIN Station s2 ON t.arrivalStationId = s2.stationId "
        "WHERE (t.trainNumber LIKE ? "
        "OR s1.stationName LIKE ? "
        "OR s2.stationName LIKE ?) "
        "AND t.enabled = 1";  // 只搜索启用的车次

    QSqlQuery query(db);
    query.prepare(sql);
    QString pattern = "%" + keyword.trimmed() + "%";
    query.addBindValue(pattern);
    query.addBindValue(pattern);
    query.addBindValue(pattern);

    if (!query.exec()) {
        setStatus("搜索车次失败: " + query.lastError().text());
        return result;
    }

    while (query.next()) {
        Train t;
        t.trainId = query.value("trainId").toInt();
        t.trainNumber = query.value("trainNumber").toString();
        t.departureStationId = query.value("departureStationId").toInt();
        t.arrivalStationId = query.value("arrivalStationId").toInt();
        t.departureTime = query.value("departureTime").toString();
        t.arrivalTime = query.value("arrivalTime").toString();
        t.totalSeats = query.value("totalSeats").toInt();
        t.remainingSeats = query.value("remainingSeats").toInt();
        t.enabled = query.value("enabled").toInt() == 1;
        result.append(t);
    }

    setStatus("搜索完成，找到 " + QString::number(result.size()) + " 条匹配记录");
    return result;
}

// ---------- 按车站查询车次（出发站或到达站）----------
QVector<Train> TrainManager::searchByStation(int stationId, bool isDeparture) {
    QVector<Train> result;
    QSqlDatabase db = getDB();
    if (!db.isOpen()) {
        setStatus("数据库未连接");
        return result;
    }

    if (stationId <= 0) {
        setStatus("无效的车站ID");
        return result;
    }

    QString sql = "SELECT trainId, trainNumber, departureStationId, arrivalStationId, "
                  "departureTime, arrivalTime, totalSeats, remainingSeats, enabled "
                  "FROM Train WHERE ";
    sql += isDeparture ? "departureStationId = ?" : "arrivalStationId = ?";
    sql += " AND enabled = 1";  // 只查询启用的车次

    QSqlQuery query(db);
    query.prepare(sql);
    query.addBindValue(stationId);

    if (!query.exec()) {
        setStatus("按站查询失败: " + query.lastError().text());
        return result;
    }

    while (query.next()) {
        Train t;
        t.trainId = query.value("trainId").toInt();
        t.trainNumber = query.value("trainNumber").toString();
        t.departureStationId = query.value("departureStationId").toInt();
        t.arrivalStationId = query.value("arrivalStationId").toInt();
        t.departureTime = query.value("departureTime").toString();
        t.arrivalTime = query.value("arrivalTime").toString();
        t.totalSeats = query.value("totalSeats").toInt();
        t.remainingSeats = query.value("remainingSeats").toInt();
        t.enabled = query.value("enabled").toInt() == 1;
        result.append(t);
    }

    setStatus("按站查询完成，共 " + QString::number(result.size()) + " 条记录");
    return result;
}
// ---------- 更新剩余座位 ----------
bool TrainManager::updateRemainingSeats(int trainId, int delta) {
    QSqlDatabase db = getDB();
    if (!db.isOpen()) {
        setStatus("数据库未连接");
        return false;
    }

    // 1. 查询当前余票和总座位数
    QSqlQuery query(db);
    query.prepare("SELECT remainingSeats, totalSeats, trainNumber FROM Train WHERE trainId = ?");
    query.addBindValue(trainId);
    if (!query.exec()) {
        setStatus("查询车次失败: " + query.lastError().text());
        return false;
    }
    if (!query.next()) {
        setStatus("未找到车次 (ID: " + QString::number(trainId) + ")");
        return false;
    }

    int currentRemaining = query.value("remainingSeats").toInt();
    int totalSeats = query.value("totalSeats").toInt();
    QString trainNumber = query.value("trainNumber").toString();

    // 2. 计算新余票并校验
    int newRemaining = currentRemaining + delta;
    if (newRemaining < 0) {
        setStatus("余票不足，当前余票: " + QString::number(currentRemaining) +
                  ", 需求: " + QString::number(delta));
        return false;
    }
    if (newRemaining > totalSeats) {
        setStatus("超售，总座位: " + QString::number(totalSeats) +
                  ", 当前余票: " + QString::number(currentRemaining) +
                  ", 增量: " + QString::number(delta));
        return false;
    }

    // 3. 执行更新
    QSqlQuery updateQuery(db);
    updateQuery.prepare("UPDATE Train SET remainingSeats = ? WHERE trainId = ?");
    updateQuery.addBindValue(newRemaining);
    updateQuery.addBindValue(trainId);

    if (!updateQuery.exec()) {
        setStatus("更新余票失败: " + updateQuery.lastError().text());
        return false;
    }

    if (updateQuery.numRowsAffected() == 0) {
        setStatus("更新余票失败，未找到匹配的记录");
        return false;
    }

    setStatus("车次 " + trainNumber + " 余票更新成功，当前余票: " +
              QString::number(newRemaining));
    return true;
}