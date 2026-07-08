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