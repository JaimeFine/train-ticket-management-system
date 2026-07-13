/**
 * @file database_manager_ticket.cpp - Issue 9-11 extensions
 */
#include "database_manager.h"
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>

// ══════════════════════ Issue 9 ══════════════════════
bool DatabaseManager::adjustTrainSeats(int trainId, int delta) {
    m_lastError.clear();
    QSqlQuery q(QSqlDatabase::database(m_connectionName));
    if(delta<0){q.prepare("UPDATE Train SET remainingSeats=remainingSeats+:d "
                           "WHERE trainId=:id AND remainingSeats>=:need");
        q.bindValue(":d",delta);q.bindValue(":need",-delta);}
    else{q.prepare("UPDATE Train SET remainingSeats=remainingSeats+:d WHERE trainId=:id");
        q.bindValue(":d",delta);}
    q.bindValue(":id",trainId);
    if(!q.exec()){m_lastError=q.lastError().text();return false;}
    return q.numRowsAffected()>0;
}
bool DatabaseManager::beginTransaction() {
    QSqlDatabase db=QSqlDatabase::database(m_connectionName);
    return db.isOpen()&&db.transaction();
}
bool DatabaseManager::commitTransaction() {
    QSqlDatabase db=QSqlDatabase::database(m_connectionName);
    return db.isOpen()&&db.commit();
}
bool DatabaseManager::rollbackTransaction() {
    QSqlDatabase db=QSqlDatabase::database(m_connectionName);
    return db.isOpen()&&db.rollback();
}
QList<DatabaseManager::TrainWithStations> DatabaseManager::searchTrainsByStation(
    const QString &dep,const QString &arr,const QString &date) const {
    m_lastError.clear();
    QSqlQuery q(QSqlDatabase::database(m_connectionName));
    QString sql="SELECT t.trainId,t.trainNumber,t.totalSeats,t.remainingSeats,"
        "t.departureTime,t.arrivalTime,t.enabled,ds.stationName,as2.stationName "
        "FROM Train t JOIN Station ds ON t.departureStationId=ds.stationId "
        "JOIN Station as2 ON t.arrivalStationId=as2.stationId WHERE t.enabled=1";
    if(!dep.isEmpty())sql+=" AND ds.stationName LIKE :dep";
    if(!arr.isEmpty())sql+=" AND as2.stationName LIKE :arr";
    if(!date.isEmpty())sql+=" AND t.departureTime LIKE :date";
    sql+=" ORDER BY t.departureTime ASC";q.prepare(sql);
    if(!dep.isEmpty())q.bindValue(":dep","%"+dep+"%");
    if(!arr.isEmpty())q.bindValue(":arr","%"+arr+"%");
    if(!date.isEmpty())q.bindValue(":date",date+"%");
    QList<TrainWithStations> rs;
    if(!q.exec()){m_lastError=q.lastError().text();return rs;}
    while(q.next()){TrainWithStations t;t.trainId=q.value(0).toInt();
        t.trainNumber=q.value(1).toString();t.totalSeats=q.value(2).toInt();
        t.remainingSeats=q.value(3).toInt();t.departureTime=q.value(4).toString();
        t.arrivalTime=q.value(5).toString();t.enabled=q.value(6).toBool();
        t.departureStationName=q.value(7).toString();t.arrivalStationName=q.value(8).toString();
        rs.append(t);}
    return rs;
}

// ══════════════════════ Issue 10 ══════════════════════

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
