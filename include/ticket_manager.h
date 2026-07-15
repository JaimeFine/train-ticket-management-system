#pragma once
/** @file ticket_manager.h - Issue 9: 订票 + 余票查询 + 车次搜索 */
#include <QString>
#include <QVector>
#include <QVariantMap>
class DatabaseManager;

// 票务管理器：封装购票/退票/改签/订单查询等业务逻辑，数据读写全部经 DatabaseManager
class TicketManager {
public:
    explicit TicketManager(DatabaseManager &db);
    int  bookTicket(int userId,int tripId,const QString &passengerName);
    int  remainingSeats(int tripId) const;
    QVector<QVariantMap> searchTrips(const QString &dep,const QString &arr,const QString &date="") const;
    QVector<QVariantMap> searchByTrainNumber(const QString &number) const;
    // Issue 10
    bool refundTicket(int orderId);
    bool changeTicket(int orderId,int newTripId);
    QVector<QVariantMap> queryOrdersByUser(int userId) const;
    QVector<QVariantMap> queryOrdersByPassenger(const QString &name) const;
    QVector<QVariantMap> queryOrderByOrderId(int orderId) const;
    QVector<QVariantMap> queryAllOrders() const;
    bool addOperationLog(const QString &operatorUsername,
                         const QString &action,
                         const QString &detail);
    QString lastError() const;
private:
    DatabaseManager &m_db; QString m_lastError;
};
