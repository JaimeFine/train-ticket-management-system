/** @file database_manager_ticket.cpp - V2: search + query through Trip */
#include "database_manager.h"
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>

// ══════════════════════ Transaction helpers ══════════════════════
bool DatabaseManager::beginTransaction() {
    return QSqlDatabase::database(m_connectionName).transaction();
}
bool DatabaseManager::commitTransaction() {
    return QSqlDatabase::database(m_connectionName).commit();
}
bool DatabaseManager::rollbackTransaction() {
    return QSqlDatabase::database(m_connectionName).rollback();
}

// ══════════════════════ Issue 9: search (V2: through Trip) ══════════════════════
QList<DatabaseManager::TrainWithStations> DatabaseManager::searchTripsByStation(
    const QString &dep, const QString &arr, const QString &date) const
{
    m_lastError.clear();
    QSqlQuery q(QSqlDatabase::database(m_connectionName));
    QString sql = QStringLiteral(
        "SELECT t.trainId,t.trainNumber,t.totalSeats,"
        "t.departureTime,t.arrivalTime,t.enabled,"
        "ds.stationName,as2.stationName,"
        "tr.tripId,tr.remainingSeats,tr.travelDate,"
        "tr.departureTime,tr.arrivalTime,tr.totalSeats "
        "FROM Train t "
        "JOIN Station ds ON t.departureStationId=ds.stationId "
        "JOIN Station as2 ON t.arrivalStationId=as2.stationId "
        "JOIN Trip tr ON tr.trainId=t.trainId "
        "WHERE t.enabled=1 AND tr.enabled=1 AND tr.remainingSeats>0");
    if (!dep.isEmpty()) sql += " AND ds.stationName LIKE :dep";
    if (!arr.isEmpty()) sql += " AND as2.stationName LIKE :arr";
    if (!date.isEmpty()) sql += " AND tr.travelDate=:date";
    sql += " ORDER BY tr.departureTime ASC";
    q.prepare(sql);
    if (!dep.isEmpty()) q.bindValue(":dep", "%" + dep + "%");
    if (!arr.isEmpty()) q.bindValue(":arr", "%" + arr + "%");
    if (!date.isEmpty()) q.bindValue(":date", date);

    QList<TrainWithStations> rs;
    if (!q.exec()) { m_lastError = q.lastError().text(); return rs; }
    while (q.next()) {
        TrainWithStations t;
        t.trainId = q.value(0).toInt();
        t.trainNumber = q.value(1).toString();
        t.totalSeats = q.value(13).toInt();
        t.departureTime = q.value(11).toString();
        t.arrivalTime = q.value(12).toString();
        t.enabled = q.value(5).toBool();
        t.departureStationName = q.value(6).toString();
        t.arrivalStationName = q.value(7).toString();
        t.tripId = q.value(8).toInt();
        t.remainingSeats = q.value(9).toInt();
        t.travelDate = q.value(10).toString();
        rs.append(t);
    }
    return rs;
}

// ══════════════════════ Issue 10: query (V2: tripId + travelDate) ══════════════════════
std::optional<OrderRecord> DatabaseManager::findOrderById(int orderId) const
{
    m_lastError.clear();
    QSqlQuery q(QSqlDatabase::database(m_connectionName));
    q.prepare("SELECT orderId,userId,tripId,passengerName,travelDate,purchaseTime,status "
              "FROM \"Order\" WHERE orderId=:id");
    q.bindValue(":id", orderId);
    if (!q.exec()) { m_lastError = q.lastError().text(); return std::nullopt; }
    if (!q.next()) return std::nullopt;
    OrderRecord r;
    r.orderId=q.value(0).toInt(); r.userId=q.value(1).toInt();
    r.tripId=q.value(2).toInt(); r.passengerName=q.value(3).toString();
    r.travelDate=q.value(4).toString(); r.purchaseTime=q.value(5).toString();
    r.status=q.value(6).toInt();
    return r;
}

QList<OrderRecord> DatabaseManager::findOrdersByPassenger(const QString &name) const
{
    m_lastError.clear();
    QSqlQuery q(QSqlDatabase::database(m_connectionName));
    q.prepare("SELECT orderId,userId,tripId,passengerName,travelDate,purchaseTime,status "
              "FROM \"Order\" WHERE passengerName LIKE :name ORDER BY purchaseTime DESC");
    q.bindValue(":name", "%"+name+"%");
    QList<OrderRecord> rs;
    if (!q.exec()) { m_lastError=q.lastError().text(); return rs; }
    while (q.next()) {
        OrderRecord r;
        r.orderId=q.value(0).toInt(); r.userId=q.value(1).toInt();
        r.tripId=q.value(2).toInt(); r.passengerName=q.value(3).toString();
        r.travelDate=q.value(4).toString(); r.purchaseTime=q.value(5).toString();
        r.status=q.value(6).toInt();
        rs.append(r);
    }
    return rs;
}
