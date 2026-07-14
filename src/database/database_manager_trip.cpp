/** @file database_manager_trip.cpp - V2 Trip CRUD */
#include "database_manager.h"
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>

std::optional<int> DatabaseManager::createTrip(int trainId, const QString &travelDate,
                                                int totalSeats) {
    m_lastError.clear();
    auto train = findTrainById(trainId);
    QString depTime = train ? train->departureTime : QStringLiteral("00:00");
    QString arrTime = train ? train->arrivalTime : QStringLiteral("23:59");

    QSqlQuery q(QSqlDatabase::database(m_connectionName));
    q.prepare("INSERT INTO Trip(trainId,travelDate,departureTime,arrivalTime,"
              "totalSeats,remainingSeats,basePrice,enabled) "
              "VALUES(:tid,:dt,:dep,:arr,:ts,:rs,0,1)");
    q.bindValue(":tid", trainId); q.bindValue(":dt", travelDate);
    q.bindValue(":dep", depTime); q.bindValue(":arr", arrTime);
    q.bindValue(":ts", totalSeats); q.bindValue(":rs", totalSeats);
    if (!q.exec()) { m_lastError = q.lastError().text(); return std::nullopt; }
    return q.lastInsertId().toInt();
}

static TripRecord readTrip(QSqlQuery &q) {
    TripRecord r;
    r.tripId=q.value(0).toInt(); r.trainId=q.value(1).toInt();
    r.travelDate=q.value(2).toString();
    r.departureTime=q.value(3).toString(); r.arrivalTime=q.value(4).toString();
    r.totalSeats=q.value(5).toInt(); r.remainingSeats=q.value(6).toInt();
    r.basePrice=q.value(7).toDouble(); r.enabled=q.value(8).toBool();
    return r;
}

#define TRIP_COLS "tripId,trainId,travelDate,departureTime,arrivalTime,totalSeats,remainingSeats,basePrice,enabled"

std::optional<TripRecord> DatabaseManager::findTripById(int tripId) const {
    m_lastError.clear();
    QSqlQuery q(QSqlDatabase::database(m_connectionName));
    q.prepare("SELECT " TRIP_COLS " FROM Trip WHERE tripId=:id");
    q.bindValue(":id", tripId);
    if (!q.exec()) { m_lastError=q.lastError().text(); return std::nullopt; }
    if (!q.next()) return std::nullopt;
    return readTrip(q);
}

std::optional<TripRecord> DatabaseManager::findOrCreateTrip(int trainId, const QString &travelDate,
                                                              int totalSeats) {
    m_lastError.clear();
    QSqlQuery q(QSqlDatabase::database(m_connectionName));
    q.prepare("SELECT " TRIP_COLS " FROM Trip WHERE trainId=:tid AND travelDate=:dt");
    q.bindValue(":tid", trainId); q.bindValue(":dt", travelDate);
    if (!q.exec()) { m_lastError=q.lastError().text(); return std::nullopt; }
    if (q.next()) return readTrip(q);
    auto id = createTrip(trainId, travelDate, totalSeats);
    if (!id) return std::nullopt;
    return findTripById(*id);
}

bool DatabaseManager::adjustTripSeats(int tripId, int delta) {
    m_lastError.clear();
    QSqlQuery q(QSqlDatabase::database(m_connectionName));
    if (delta < 0) {
        q.prepare("UPDATE Trip SET remainingSeats=remainingSeats+:d "
                   "WHERE tripId=:id AND remainingSeats>=:need AND enabled=1");
        q.bindValue(":d", delta); q.bindValue(":need", -delta);
    } else {
        q.prepare("UPDATE Trip SET remainingSeats=remainingSeats+:d WHERE tripId=:id AND enabled=1");
        q.bindValue(":d", delta);
    }
    q.bindValue(":id", tripId);
    if (!q.exec()) { m_lastError=q.lastError().text(); return false; }
    return q.numRowsAffected()>0;
}

QList<TripRecord> DatabaseManager::findTripsByTrain(int trainId) const {
    m_lastError.clear();
    QSqlQuery q(QSqlDatabase::database(m_connectionName));
    q.prepare("SELECT " TRIP_COLS " FROM Trip WHERE trainId=:tid ORDER BY travelDate");
    q.bindValue(":tid", trainId);
    QList<TripRecord> rs;
    if (!q.exec()) { m_lastError=q.lastError().text(); return rs; }
    while (q.next()) rs.append(readTrip(q));
    return rs;
}
