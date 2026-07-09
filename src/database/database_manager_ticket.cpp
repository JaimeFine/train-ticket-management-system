/**
 * @file database_manager_ticket.cpp - Issue 9-11 DatabaseManager extensions
 */
#include "database_manager.h"
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>

// ============================================ Issue 9 ============================================

bool DatabaseManager::adjustTrainSeats(int trainId, int delta)
{
    m_lastError.clear();
    QSqlQuery query(QSqlDatabase::database(m_connectionName));

    if (delta < 0) {
        query.prepare(
            "UPDATE Train SET remainingSeats = remainingSeats + :delta "
            "WHERE trainId = :id AND remainingSeats >= :need");
        query.bindValue(":delta", delta);
        query.bindValue(":need", -delta);
    } else {
        query.prepare(
            "UPDATE Train SET remainingSeats = remainingSeats + :delta "
            "WHERE trainId = :id");
        query.bindValue(":delta", delta);
    }
    query.bindValue(":id", trainId);

    if (!query.exec()) { m_lastError = query.lastError().text(); return false; }
    return query.numRowsAffected() > 0;
}

bool DatabaseManager::beginTransaction()
{
    QSqlDatabase db = QSqlDatabase::database(m_connectionName);
    return db.isOpen() && db.transaction();
}

bool DatabaseManager::commitTransaction()
{
    QSqlDatabase db = QSqlDatabase::database(m_connectionName);
    return db.isOpen() && db.commit();
}

bool DatabaseManager::rollbackTransaction()
{
    QSqlDatabase db = QSqlDatabase::database(m_connectionName);
    return db.isOpen() && db.rollback();
}

QList<DatabaseManager::TrainWithStations> DatabaseManager::searchTrainsByStation(
    const QString &departure, const QString &arrival, const QString &date) const
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

    if (!departure.isEmpty()) sql += " AND ds.stationName LIKE :dep";
    if (!arrival.isEmpty())   sql += " AND as2.stationName LIKE :arr";
    if (!date.isEmpty())      sql += " AND t.departureTime LIKE :date";
    sql += " ORDER BY t.departureTime ASC";

    query.prepare(sql);
    if (!departure.isEmpty()) query.bindValue(":dep", "%" + departure + "%");
    if (!arrival.isEmpty())   query.bindValue(":arr", "%" + arrival + "%");
    if (!date.isEmpty())      query.bindValue(":date", date + "%");

    QList<TrainWithStations> results;
    if (!query.exec()) { m_lastError = query.lastError().text(); return results; }

    while (query.next()) {
        TrainWithStations train;
        train.trainId              = query.value(0).toInt();
        train.trainNumber          = query.value(1).toString();
        train.totalSeats           = query.value(2).toInt();
        train.remainingSeats       = query.value(3).toInt();
        train.departureTime        = query.value(4).toString();
        train.arrivalTime          = query.value(5).toString();
        train.enabled              = query.value(6).toBool();
        train.departureStationName = query.value(7).toString();
        train.arrivalStationName   = query.value(8).toString();
        results.append(train);
    }
    return results;
}

// ============================================ Issue 10 ============================================

std::optional<OrderRecord> DatabaseManager::findOrderById(int orderId) const
{
    m_lastError.clear();
    QSqlQuery query(QSqlDatabase::database(m_connectionName));

    query.prepare(
        "SELECT orderId, userId, trainId, passengerName, purchaseTime, status "
        "FROM \"Order\" WHERE orderId = :id");
    query.bindValue(":id", orderId);

    if (!query.exec()) { m_lastError = query.lastError().text(); return std::nullopt; }
    if (!query.next()) return std::nullopt;

    OrderRecord record;
    record.orderId       = query.value(0).toInt();
    record.userId        = query.value(1).toInt();
    record.trainId       = query.value(2).toInt();
    record.passengerName = query.value(3).toString();
    record.purchaseTime  = query.value(4).toString();
    record.status        = query.value(5).toInt();
    return record;
}

QList<OrderRecord> DatabaseManager::findOrdersByPassenger(const QString &name) const
{
    m_lastError.clear();
    QSqlQuery query(QSqlDatabase::database(m_connectionName));

    query.prepare(
        "SELECT orderId, userId, trainId, passengerName, purchaseTime, status "
        "FROM \"Order\" WHERE passengerName LIKE :name "
        "ORDER BY purchaseTime DESC");
    query.bindValue(":name", "%" + name + "%");

    QList<OrderRecord> results;
    if (!query.exec()) { m_lastError = query.lastError().text(); return results; }

    while (query.next()) {
        OrderRecord record;
        record.orderId       = query.value(0).toInt();
        record.userId        = query.value(1).toInt();
        record.trainId       = query.value(2).toInt();
        record.passengerName = query.value(3).toString();
        record.purchaseTime  = query.value(4).toString();
        record.status        = query.value(5).toInt();
        results.append(record);
    }
    return results;
}

// ============================================ Issue 11 ============================================

QList<DatabaseManager::OrderWithDetails> DatabaseManager::findAllOrdersWithDetails() const
{
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

    QList<OrderWithDetails> results;
    if (!query.exec()) { m_lastError = query.lastError().text(); return results; }

    while (query.next()) {
        OrderWithDetails detail;
        detail.orderId              = query.value(0).toInt();
        detail.userId               = query.value(1).toInt();
        detail.trainId              = query.value(2).toInt();
        detail.status               = query.value(3).toInt();
        detail.trainNumber          = query.value(4).toString();
        detail.passengerName        = query.value(5).toString();
        detail.purchaseTime         = query.value(6).toString();
        detail.departureStationName = query.value(7).toString();
        detail.arrivalStationName   = query.value(8).toString();
        results.append(detail);
    }
    return results;
}

int DatabaseManager::countOrdersByStatus(int status) const
{
    m_lastError.clear();
    QSqlQuery query(QSqlDatabase::database(m_connectionName));

    query.prepare("SELECT COUNT(*) FROM \"Order\" WHERE status = :status");
    query.bindValue(":status", status);

    if (!query.exec()) return 0;
    return query.next() ? query.value(0).toInt() : 0;
}

int DatabaseManager::countAllOrders() const
{
    m_lastError.clear();
    QSqlQuery query(QSqlDatabase::database(m_connectionName));

    query.prepare("SELECT COUNT(*) FROM \"Order\"");

    if (!query.exec()) return 0;
    return query.next() ? query.value(0).toInt() : 0;
}

QList<DatabaseManager::RouteStat> DatabaseManager::popularRoutes(int limit) const
{
    m_lastError.clear();
    QSqlQuery query(QSqlDatabase::database(m_connectionName));

    query.prepare(QString(
        "SELECT ds.stationName, as2.stationName, COUNT(o.orderId) "
        "FROM Train t "
        "JOIN \"Order\" o ON t.trainId = o.trainId "
        "JOIN Station ds ON t.departureStationId = ds.stationId "
        "JOIN Station as2 ON t.arrivalStationId = as2.stationId "
        "WHERE o.status = 0 "
        "GROUP BY ds.stationName, as2.stationName "
        "ORDER BY COUNT(o.orderId) DESC "
        "LIMIT %1").arg(limit));

    QList<RouteStat> results;
    if (!query.exec()) { m_lastError = query.lastError().text(); return results; }

    while (query.next()) {
        RouteStat route;
        route.dep   = query.value(0).toString();
        route.arr   = query.value(1).toString();
        route.count = query.value(2).toInt();
        results.append(route);
    }
    return results;
}

QList<DatabaseManager::MonthlyStat> DatabaseManager::monthlyPassengerFlow() const
{
    m_lastError.clear();
    QSqlQuery query(QSqlDatabase::database(m_connectionName));

    query.prepare(
        "SELECT SUBSTR(purchaseTime, 1, 7), COUNT(*), "
        "  SUM(CASE WHEN status = 0 THEN 1 ELSE 0 END), "
        "  SUM(CASE WHEN status = 1 THEN 1 ELSE 0 END) "
        "FROM \"Order\" "
        "GROUP BY month "
        "ORDER BY month DESC");

    QList<MonthlyStat> results;
    if (!query.exec()) { m_lastError = query.lastError().text(); return results; }

    while (query.next()) {
        MonthlyStat stat;
        stat.month    = query.value(0).toString();
        stat.total    = query.value(1).toInt();
        stat.booked   = query.value(2).toInt();
        stat.refunded = query.value(3).toInt();
        results.append(stat);
    }
    return results;
}
