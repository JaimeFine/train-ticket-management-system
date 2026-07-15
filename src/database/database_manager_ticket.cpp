#include "database_manager.h"

#include <QDate>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>

bool DatabaseManager::beginTransaction()
{
    return QSqlDatabase::database(m_connectionName).transaction();
}

bool DatabaseManager::commitTransaction()
{
    return QSqlDatabase::database(m_connectionName).commit();
}

bool DatabaseManager::rollbackTransaction()
{
    return QSqlDatabase::database(m_connectionName).rollback();
}

bool DatabaseManager::adjustTrainSeats(int trainId, int delta)
{
    m_lastError.clear();

    const auto train = findTrainById(trainId);
    if (!train.has_value()) {
        m_lastError = QStringLiteral("Train does not exist.");
        return false;
    }

    const QString travelDate =
        QDate::currentDate().toString(QStringLiteral("yyyy-MM-dd"));
    const auto trip = findOrCreateTrip(trainId, travelDate, train->totalSeats);
    if (!trip.has_value()) {
        return false;
    }

    return adjustTripSeats(trip->tripId, delta);
}

QList<DatabaseManager::TrainWithStations> DatabaseManager::searchTripsByStation(
    const QString &dep, const QString &arr, const QString &date) const
{
    m_lastError.clear();
    QSqlQuery query(QSqlDatabase::database(m_connectionName));

    QString sql = QStringLiteral(
        "SELECT t.trainId, t.trainNumber, tr.totalSeats, tr.remainingSeats, tr.basePrice, "
        "tr.departureTime, tr.arrivalTime, t.enabled, ds.stationName, as2.stationName, "
        "tr.travelDate, tr.tripId "
        "FROM Train t "
        "JOIN Station ds ON t.departureStationId = ds.stationId "
        "JOIN Station as2 ON t.arrivalStationId = as2.stationId "
        "JOIN Trip tr ON tr.trainId = t.trainId "
        "WHERE t.enabled = 1 AND tr.enabled = 1"
    );

    if (!dep.isEmpty()) {
        sql += QStringLiteral(" AND ds.stationName LIKE :dep");
    }
    if (!arr.isEmpty()) {
        sql += QStringLiteral(" AND as2.stationName LIKE :arr");
    }
    if (!date.isEmpty()) {
        sql += QStringLiteral(" AND tr.travelDate = :date");
    }

    sql += QStringLiteral(" ORDER BY tr.travelDate ASC, tr.departureTime ASC");
    query.prepare(sql);

    if (!dep.isEmpty()) {
        query.bindValue(":dep", QStringLiteral("%") + dep + QStringLiteral("%"));
    }
    if (!arr.isEmpty()) {
        query.bindValue(":arr", QStringLiteral("%") + arr + QStringLiteral("%"));
    }
    if (!date.isEmpty()) {
        query.bindValue(":date", date);
    }

    QList<TrainWithStations> results;
    if (!query.exec()) {
        m_lastError = query.lastError().text();
        return results;
    }

    while (query.next()) {
        TrainWithStations record;
        record.trainId = query.value(0).toInt();
        record.trainNumber = query.value(1).toString();
        record.totalSeats = query.value(2).toInt();
        record.remainingSeats = query.value(3).toInt();
        record.basePrice = query.value(4).toDouble();
        record.departureTime = query.value(5).toString();
        record.arrivalTime = query.value(6).toString();
        record.enabled = query.value(7).toBool();
        record.departureStationName = query.value(8).toString();
        record.arrivalStationName = query.value(9).toString();
        record.travelDate = query.value(10).toString();
        record.tripId = query.value(11).toInt();
        results.append(record);
    }

    return results;
}

QList<DatabaseManager::TrainWithStations> DatabaseManager::searchTrainsByStation(
    const QString &dep, const QString &arr, const QString &date) const
{
    return searchTripsByStation(dep, arr, date);
}

std::optional<OrderRecord> DatabaseManager::findOrderById(int orderId) const
{
    m_lastError.clear();
    QSqlQuery query(QSqlDatabase::database(m_connectionName));
    query.prepare(QStringLiteral(
        "SELECT o.orderId, o.userId, tr.trainId, o.tripId, o.passengerName, "
        "o.travelDate, o.purchaseTime, o.price, o.status "
        "FROM \"Order\" o "
        "JOIN Trip tr ON o.tripId = tr.tripId "
        "WHERE o.orderId = :orderId"
    ));
    query.bindValue(":orderId", orderId);

    if (!query.exec()) {
        m_lastError = query.lastError().text();
        return std::nullopt;
    }
    if (!query.next()) {
        return std::nullopt;
    }

    OrderRecord record;
    record.orderId = query.value(0).toInt();
    record.userId = query.value(1).toInt();
    record.trainId = query.value(2).toInt();
    record.tripId = query.value(3).toInt();
    record.passengerName = query.value(4).toString();
    record.travelDate = query.value(5).toString();
    record.purchaseTime = query.value(6).toString();
    record.price = query.value(7).toDouble();
    record.status = query.value(8).toInt();
    return record;
}

// 按乘客姓名模糊查询订单，按购票时间倒序
QList<OrderRecord> DatabaseManager::findOrdersByPassenger(const QString &name) const
{
    m_lastError.clear();
    QSqlQuery query(QSqlDatabase::database(m_connectionName));
    query.prepare(QStringLiteral(
        "SELECT o.orderId, o.userId, tr.trainId, o.tripId, o.passengerName, "
        "o.travelDate, o.purchaseTime, o.price, o.status "
        "FROM \"Order\" o "
        "JOIN Trip tr ON o.tripId = tr.tripId "
        "WHERE o.passengerName LIKE :name "
        "ORDER BY o.purchaseTime DESC"
    ));
    query.bindValue(":name", QStringLiteral("%") + name + QStringLiteral("%"));

    QList<OrderRecord> results;
    if (!query.exec()) {
        m_lastError = query.lastError().text();
        return results;
    }

    while (query.next()) {
        OrderRecord record;
        record.orderId = query.value(0).toInt();
        record.userId = query.value(1).toInt();
        record.trainId = query.value(2).toInt();
        record.tripId = query.value(3).toInt();
        record.passengerName = query.value(4).toString();
        record.travelDate = query.value(5).toString();
        record.purchaseTime = query.value(6).toString();
        record.price = query.value(7).toDouble();
        record.status = query.value(8).toInt();
        results.append(record);
    }

    return results;
}
