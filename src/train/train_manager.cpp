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