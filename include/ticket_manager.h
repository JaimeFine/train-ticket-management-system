#pragma once
// ticket_manager.h — V2：基于 tripId 的售票业务
#include <QString>
#include <QVector>
#include <QVariantMap>
class DatabaseManager;

// 票务管理器：封装购票/退票/改签/订单查询等业务逻辑，数据读写全部经 DatabaseManager
class TicketManager {
public:
    explicit TicketManager(DatabaseManager &db);
    // Issue 9：查询与购票（V2 按 tripId 售票）
    int  bookTicket(int userId,int tripId,const QString &passengerName);   // 购票，成功返回订单ID，失败返回 -1（原因见 lastError）
    int  remainingSeats(int tripId) const;   // 查询班次余票
    QVector<QVariantMap> searchTrips(const QString &dep,const QString &arr,const QString &date="") const;   // 按出发/到达站（可选日期）查班次
    QVector<QVariantMap> searchByTrainNumber(const QString &number) const;   // 按车次号查询
    // Issue 10：退票 / 改签 / 订单查询（V2 按 tripId）
    bool refundTicket(int orderId);                  // 退票：订单置已退票并回补余票
    bool changeTicket(int orderId,int newTripId);   // 改签到新班次：原班次回补余票，新班次扣票
    QVector<QVariantMap> queryOrdersByUser(int userId) const;
    QVector<QVariantMap> queryOrdersByPassenger(const QString &name) const;
    QVector<QVariantMap> queryOrderByOrderId(int orderId) const;
    // Issue 11：全量订单历史（供管理端/统计使用）
    QVector<QVariantMap> queryAllOrders() const;
    // 记录一条操作日志
    bool addOperationLog(const QString &operatorUsername,
                         const QString &action,
                         const QString &detail);
    QString lastError() const;   // 最近一次业务失败原因
private:
    DatabaseManager &m_db; QString m_lastError;
};
