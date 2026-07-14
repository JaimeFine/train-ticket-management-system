#include "database_manager.h"

#include <QDate>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QString>

std::optional<int> DatabaseManager::createOrder(const OrderRecord &order)
{
    m_lastError.clear();

    int tripId = order.tripId;
    QString travelDate = order.travelDate;
    double price = order.price;

    if (tripId <= 0 && order.trainId > 0) {
        const auto train = findTrainById(order.trainId);
        if (!train.has_value()) {
            m_lastError = QStringLiteral("Train does not exist.");
            return std::nullopt;
        }

        if (travelDate.isEmpty()) {
            if (order.purchaseTime.size() >= 10) {
                travelDate = order.purchaseTime.left(10);
            } else {
                travelDate = QDate::currentDate().toString(QStringLiteral("yyyy-MM-dd"));
            }
        }

        const auto trip = findOrCreateTrip(order.trainId, travelDate, train->totalSeats);
        if (!trip.has_value()) {
            return std::nullopt;
        }

        tripId = trip->tripId;
        if (price <= 0.0) {
            price = trip->basePrice;
        }
    }

    if (tripId <= 0) {
        m_lastError = QStringLiteral("Trip does not exist.");
        return std::nullopt;
    }

    if (travelDate.isEmpty()) {
        const auto trip = findTripById(tripId);
        if (!trip.has_value()) {
            m_lastError = QStringLiteral("Trip does not exist.");
            return std::nullopt;
        }
        travelDate = trip->travelDate;
        if (price <= 0.0) {
            price = trip->basePrice;
        }
    }

    QSqlQuery query(QSqlDatabase::database(m_connectionName));
    query.prepare(QStringLiteral(
        "INSERT INTO \"Order\" ("
        "userId, tripId, passengerName, travelDate, purchaseTime, price, status"
        ") VALUES ("
        ":userId, :tripId, :passengerName, :travelDate, :purchaseTime, :price, :status)"
    ));
    query.bindValue(":userId", order.userId);
    query.bindValue(":tripId", tripId);
    query.bindValue(":passengerName", order.passengerName);
    query.bindValue(":travelDate", travelDate);
    query.bindValue(":purchaseTime", order.purchaseTime);
    query.bindValue(":price", price);
    query.bindValue(":status", order.status);

    if (!query.exec()) {
        m_lastError = query.lastError().text();
        return std::nullopt;
    }

    return query.lastInsertId().toInt();
}

QList<OrderRecord> DatabaseManager::findOrdersByUser(int userId) const
{
    m_lastError.clear();
    QSqlQuery query(QSqlDatabase::database(m_connectionName));
    query.prepare(QStringLiteral(
        "SELECT o.orderId, o.userId, tr.trainId, o.tripId, o.passengerName, "
        "o.travelDate, o.purchaseTime, o.price, o.status "
        "FROM \"Order\" o "
        "JOIN Trip tr ON o.tripId = tr.tripId "
        "WHERE o.userId = :userId "
        "ORDER BY o.purchaseTime DESC"
    ));
    query.bindValue(":userId", userId);

    QList<OrderRecord> records;
    if (!query.exec()) {
        m_lastError = query.lastError().text();
        return records;
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
        records.append(record);
    }

    return records;
}

bool DatabaseManager::updateOrderStatus(int orderId, int status)
{
    m_lastError.clear();
    QSqlQuery query(QSqlDatabase::database(m_connectionName));
    query.prepare(QStringLiteral(
        "UPDATE \"Order\" SET status = :status WHERE orderId = :orderId"
    ));
    query.bindValue(":orderId", orderId);
    query.bindValue(":status", status);

    if (!query.exec()) {
        m_lastError = query.lastError().text();
        return false;
    }

    if (query.numRowsAffected() == 0) {
        m_lastError = QStringLiteral("No order found for the given orderId.");
        return false;
    }

    return true;
}
