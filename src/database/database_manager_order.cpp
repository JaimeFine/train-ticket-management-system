#include "database_manager.h"

#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QString>

std::optional<int> DatabaseManager::createOrder(const OrderRecord &order) {
    QSqlQuery query(QSqlDatabase::database(m_connectionName));

    query.prepare(QStringLiteral(
        "INSERT INTO \"Order\" ("
        "userId, trainId, passengerName, purchaseTime, status"
        ") "
        "VALUES (:userId, :trainId, :passengerName, :purchaseTime, :status)"
    ));

    query.bindValue(":userId", order.userId);
    query.bindValue(":trainId", order.trainId);
    query.bindValue(":passengerName", order.passengerName);
    query.bindValue(":purchaseTime", order.purchaseTime);
    query.bindValue(":status", order.status);

    if (!query.exec()) {
        m_lastError = query.lastError().text();
        return std::nullopt;
    }

    return query.lastInsertId().toInt();
}

QList<OrderRecord> DatabaseManager::findOrdersByUser(int userId) const {
    m_lastError.clear();
    QSqlQuery query(QSqlDatabase::database(m_connectionName));

    query.prepare(QStringLiteral(
        "SELECT orderId, userId, trainId, passengerName, purchaseTime, status "
        "FROM \"Order\" "
        "WHERE userId = :userId"
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
        record.passengerName = query.value(3).toString();
        record.purchaseTime = query.value(4).toString();
        record.status = query.value(5).toInt();

        records.append(record);
    }

    return records;
}

bool DatabaseManager::updateOrderStatus(int orderId, int status) {
    m_lastError.clear();
    QSqlQuery query(QSqlDatabase::database(m_connectionName));

    query.prepare(QStringLiteral(
        "UPDATE \"Order\" "
        "SET status = :status "
        "WHERE orderId = :orderId"
    ));

    query.bindValue(":orderId", orderId);
    query.bindValue(":status", status);

    if (!query.exec()) {
        m_lastError = query.lastError().text();
        return false;
    }

    // Check if any row was actually updated
    if (query.numRowsAffected() == 0) {
        return false;
    }

    return true;
}
