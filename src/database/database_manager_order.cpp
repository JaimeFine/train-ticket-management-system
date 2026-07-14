#include "database_manager.h"
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>

std::optional<int> DatabaseManager::createOrder(const OrderRecord &order) {
    QSqlQuery q(QSqlDatabase::database(m_connectionName));
    q.prepare("INSERT INTO \"Order\"(userId,tripId,passengerName,travelDate,purchaseTime,price,status) "
              "VALUES(:uid,:tid,:name,:date,:pt,:pr,:st)");
    q.bindValue(":uid", order.userId);
    q.bindValue(":tid", order.tripId);
    q.bindValue(":name", order.passengerName);
    q.bindValue(":date", order.travelDate);
    q.bindValue(":pt", order.purchaseTime);
    q.bindValue(":pr", order.price);
    q.bindValue(":st", order.status);
    if (!q.exec()) { m_lastError = q.lastError().text(); return std::nullopt; }
    return q.lastInsertId().toInt();
}

QList<OrderRecord> DatabaseManager::findOrdersByUser(int userId) const {
    m_lastError.clear();
    QSqlQuery q(QSqlDatabase::database(m_connectionName));
    q.prepare("SELECT orderId,userId,tripId,passengerName,travelDate,purchaseTime,status "
              "FROM \"Order\" WHERE userId=:uid ORDER BY purchaseTime DESC");
    q.bindValue(":uid", userId);
    QList<OrderRecord> rs;
    if (!q.exec()) { m_lastError = q.lastError().text(); return rs; }
    while (q.next()) {
        OrderRecord r;
        r.orderId = q.value(0).toInt(); r.userId = q.value(1).toInt();
        r.tripId = q.value(2).toInt(); r.passengerName = q.value(3).toString();
        r.travelDate = q.value(4).toString(); r.purchaseTime = q.value(5).toString();
        r.status = q.value(6).toInt();
        rs.append(r);
    }
    return rs;
}

bool DatabaseManager::updateOrderStatus(int orderId, int status) {
    m_lastError.clear();
    QSqlQuery q(QSqlDatabase::database(m_connectionName));
    q.prepare("UPDATE \"Order\" SET status=:st WHERE orderId=:id");
    q.bindValue(":id", orderId);
    q.bindValue(":st", status);
    if (!q.exec()) { m_lastError = q.lastError().text(); return false; }
    return q.numRowsAffected() > 0;
}
