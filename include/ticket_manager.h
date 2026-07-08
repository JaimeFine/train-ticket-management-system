#pragma once

#include <QString>
#include <QVector>
#include <QVariantMap>
#include <QDateTime>

class DatabaseManager;

/**
 * @brief 票务管理器 — 负责订票/退票/改签/订单查询的业务逻辑
 *
 * 所有权：TicketManager 拥有 Order 表和 Train.remainingSeats 的业务逻辑。
 * 所有 SQL 通过 DatabaseManager 执行。
 */
class TicketManager
{
public:
    explicit TicketManager(DatabaseManager &db);

    // ── 订票 ────────────────────────────────────────────────────
    /// @return orderId > 0 成功，-1 失败
    int  bookTicket(int userId, int trainId, const QString &passengerName);

    // ── 退票 ────────────────────────────────────────────────────
    bool refundTicket(int orderId);

    // ── 改签 ────────────────────────────────────────────────────
    bool changeTicket(int orderId, int newTrainId);

    // ── 余票查询 ─────────────────────────────────────────────────
    int  remainingSeats(int trainId) const;

    // ── 订单查询 ─────────────────────────────────────────────────
    QVector<QVariantMap> queryOrdersByUser(int userId) const;
    QVector<QVariantMap> queryOrdersByPassenger(const QString &name) const;
    QVector<QVariantMap> queryOrderByOrderId(int orderId) const;
    QVector<QVariantMap> queryAllOrders() const;

    // ── 车次搜索（供售票 UI 使用）────────────────────────────────
    /// 按出发站/到达站/日期搜索可售票车次
    QVector<QVariantMap> searchTrains(const QString &departureStation,
                                      const QString &arrivalStation,
                                      const QString &date = "") const;
    /// 按车次号搜索
    QVector<QVariantMap> searchByTrainNumber(const QString &trainNumber) const;

    // ── 错误信息 ─────────────────────────────────────────────────
    QString lastError() const;

private:
    DatabaseManager &m_db;
    QString          m_lastError;
};
