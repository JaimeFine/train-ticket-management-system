/**
 * @file database_manager_ticket.cpp
 * @brief Issue 9 — Ticket/Statistics extensions to DatabaseManager
 *
 * 新增方法：订单搜索、车次搜索（带站名）、座位调整、统计聚合
 * 遵循 PM 的编码模式（named params, m_connectionName, std::optional）
 */
#include "database_manager.h"

#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QString>

// ══════════════════════════════════════════════════════════════════════════════
// Order search extensions
// ══════════════════════════════════════════════════════════════════════════════

std::optional<OrderRecord> DatabaseManager::findOrderById(int orderId) const {
    m_lastError.clear();
    QSqlQuery query(QSqlDatabase::database(m_connectionName));
    query.prepare("SELECT orderId, userId, trainId, passengerName, purchaseTime, status "
                  "FROM \"Order\" WHERE orderId = :orderId");
    query.bindValue(":orderId", orderId);
    if (!query.exec()) { m_lastError = query.lastError().text(); return std::nullopt; }
    if (!query.next()) return std::nullopt;
    OrderRecord r;
    r.orderId = query.value(0).toInt(); r.userId = query.value(1).toInt();
    r.trainId = query.value(2).toInt(); r.passengerName = query.value(3).toString();
    r.purchaseTime = query.value(4).toString(); r.status = query.value(5).toInt();
    return r;
}

QList<OrderRecord> DatabaseManager::findOrdersByPassenger(const QString &name) const {
    m_lastError.clear();
    QSqlQuery query(QSqlDatabase::database(m_connectionName));
    query.prepare("SELECT orderId, userId, trainId, passengerName, purchaseTime, status "
                  "FROM \"Order\" WHERE passengerName LIKE :name ORDER BY purchaseTime DESC");
    query.bindValue(":name", "%" + name + "%");
    QList<OrderRecord> records;
    if (!query.exec()) { m_lastError = query.lastError().text(); return records; }
    while (query.next()) {
        OrderRecord r;
        r.orderId = query.value(0).toInt(); r.userId = query.value(1).toInt();
        r.trainId = query.value(2).toInt(); r.passengerName = query.value(3).toString();
        r.purchaseTime = query.value(4).toString(); r.status = query.value(5).toInt();
        records.append(r);
    }
    return records;
}

QList<DatabaseManager::OrderWithDetails>
DatabaseManager::findAllOrdersWithDetails() const {
    m_lastError.clear();
    QSqlQuery query(QSqlDatabase::database(m_connectionName));
    query.prepare(
        "SELECT o.orderId, o.userId, o.trainId, o.status, "
        "  t.trainNumber, o.passengerName, o.purchaseTime, "
        "  ds.stationName, as2.stationName "
        "FROM \"Order\" o "
        "JOIN Train t ON o.trainId = t.trainId "
        "JOIN Station ds ON t.departureStationId = ds.stationId "
        "JOIN Station as2 ON t.arrivalStationId = as2.stationId "
        "ORDER BY o.purchaseTime DESC");
    QList<OrderWithDetails> records;
    if (!query.exec()) { m_lastError = query.lastError().text(); return records; }
    while (query.next()) {
        OrderWithDetails d;
        d.orderId = query.value(0).toInt(); d.userId = query.value(1).toInt();
        d.trainId = query.value(2).toInt(); d.status = query.value(3).toInt();
        d.trainNumber = query.value(4).toString();
        d.passengerName = query.value(5).toString();
        d.purchaseTime = query.value(6).toString();
        d.departureStationName = query.value(7).toString();
        d.arrivalStationName = query.value(8).toString();
        records.append(d);
    }
    return records;
}

// ══════════════════════════════════════════════════════════════════════════════
// Train search with station names
// ══════════════════════════════════════════════════════════════════════════════

QList<DatabaseManager::TrainWithStations>
DatabaseManager::searchTrainsByStation(
    const QString &depStation, const QString &arrStation, const QString &date) const
{
    m_lastError.clear();
    QSqlQuery query(QSqlDatabase::database(m_connectionName));

    QString sql =
        "SELECT t.trainId, t.trainNumber, t.totalSeats, t.remainingSeats, "
        "  t.departureTime, t.arrivalTime, t.enabled, "
        "  ds.stationName, as2.stationName "
        "FROM Train t "
        "JOIN Station ds ON t.departureStationId = ds.stationId "
        "JOIN Station as2 ON t.arrivalStationId = as2.stationId "
        "WHERE t.enabled = 1";

    if (!depStation.isEmpty()) sql += " AND ds.stationName LIKE :dep";
    if (!arrStation.isEmpty()) sql += " AND as2.stationName LIKE :arr";
    if (!date.isEmpty())       sql += " AND t.departureTime LIKE :date";
    sql += " ORDER BY t.departureTime ASC";

    query.prepare(sql);
    if (!depStation.isEmpty()) query.bindValue(":dep", "%" + depStation + "%");
    if (!arrStation.isEmpty()) query.bindValue(":arr", "%" + arrStation + "%");
    if (!date.isEmpty())       query.bindValue(":date", date + "%");

    QList<TrainWithStations> results;
    if (!query.exec()) { m_lastError = query.lastError().text(); return results; }
    while (query.next()) {
        TrainWithStations t;
        t.trainId = query.value(0).toInt();
        t.trainNumber = query.value(1).toString();
        t.totalSeats = query.value(2).toInt();
        t.remainingSeats = query.value(3).toInt();
        t.departureTime = query.value(4).toString();
        t.arrivalTime = query.value(5).toString();
        t.enabled = query.value(6).toBool();
        t.departureStationName = query.value(7).toString();
        t.arrivalStationName = query.value(8).toString();
        results.append(t);
    }
    return results;
}

// ══════════════════════════════════════════════════════════════════════════════
// Seat management
// ══════════════════════════════════════════════════════════════════════════════

bool DatabaseManager::adjustTrainSeats(int trainId, int delta) {
    m_lastError.clear();
    QSqlQuery query(QSqlDatabase::database(m_connectionName));
    // delta < 0: only update if enough seats remain
    if (delta < 0) {
        query.prepare("UPDATE Train SET remainingSeats = remainingSeats + :delta "
                      "WHERE trainId = :trainId AND remainingSeats >= :need");
        query.bindValue(":delta", delta);
        query.bindValue(":need", -delta);
    } else {
        query.prepare("UPDATE Train SET remainingSeats = remainingSeats + :delta "
                      "WHERE trainId = :trainId");
        query.bindValue(":delta", delta);
    }
    query.bindValue(":trainId", trainId);
    if (!query.exec()) { m_lastError = query.lastError().text(); return false; }
    return query.numRowsAffected() > 0;
}

// ══════════════════════════════════════════════════════════════════════════════
// Statistics aggregates
// ══════════════════════════════════════════════════════════════════════════════

int DatabaseManager::countOrdersByStatus(int status) const {
    m_lastError.clear();
    QSqlQuery query(QSqlDatabase::database(m_connectionName));
    query.prepare("SELECT COUNT(*) FROM \"Order\" WHERE status = :status");
    query.bindValue(":status", status);
    if (!query.exec()) return 0;
    return query.next() ? query.value(0).toInt() : 0;
}

int DatabaseManager::countAllOrders() const {
    m_lastError.clear();
    QSqlQuery query(QSqlDatabase::database(m_connectionName));
    query.prepare("SELECT COUNT(*) FROM \"Order\"");
    if (!query.exec()) return 0;
    return query.next() ? query.value(0).toInt() : 0;
}

QList<DatabaseManager::RouteStat>
DatabaseManager::popularRoutes(int limit) const {
    m_lastError.clear();
    QSqlQuery query(QSqlDatabase::database(m_connectionName));
    query.prepare(
        "SELECT ds.stationName, as2.stationName, COUNT(o.orderId) "
        "FROM Train t "
        "JOIN \"Order\" o ON t.trainId = o.trainId "
        "JOIN Station ds ON t.departureStationId = ds.stationId "
        "JOIN Station as2 ON t.arrivalStationId = as2.stationId "
        "WHERE o.status = 0 "
        "GROUP BY ds.stationName, as2.stationName "
        "ORDER BY COUNT(o.orderId) DESC LIMIT :limit");
    query.bindValue(":limit", limit);
    QList<RouteStat> results;
    if (!query.exec()) { m_lastError = query.lastError().text(); return results; }
    while (query.next()) {
        results.append({query.value(0).toString(), query.value(1).toString(), query.value(2).toInt()});
    }
    return results;
}

QList<DatabaseManager::MonthlyStat>
DatabaseManager::monthlyPassengerFlow() const {
    m_lastError.clear();
    QSqlQuery query(QSqlDatabase::database(m_connectionName));
    query.prepare(
        "SELECT SUBSTR(purchaseTime, 1, 7), COUNT(*), "
        "  SUM(CASE WHEN status=0 THEN 1 ELSE 0 END), "
        "  SUM(CASE WHEN status=1 THEN 1 ELSE 0 END) "
        "FROM \"Order\" GROUP BY month ORDER BY month DESC");
    QList<MonthlyStat> results;
    if (!query.exec()) { m_lastError = query.lastError().text(); return results; }
    while (query.next()) {
        results.append({query.value(0).toString(), query.value(1).toInt(),
                        query.value(2).toInt(), query.value(3).toInt()});
    }
    return results;
}
