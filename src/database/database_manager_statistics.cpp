/**
 * @file database_manager_statistics.cpp - Issue 11: 订单历史 + 统计
 * 实现 StatisticsManager 和 TicketManager 委托调用的聚合查询。
 */
#include "database_manager.h"

#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>

// ── Issue 11: findAllOrdersWithDetails ──────────────────────────────────
// 查询全部订单及关联明细（车次号、起讫站名），供订单历史页展示
QList<DatabaseManager::OrderWithDetails>
DatabaseManager::findAllOrdersWithDetails() const
{
    m_lastError.clear();
    QSqlQuery query(QSqlDatabase::database(m_connectionName));

    // 订单 -> Trip -> Train -> Station（出发/到达各连一次），把外键翻译成可读名称
    query.prepare(QStringLiteral(
        "SELECT o.orderId, o.userId, o.tripId, o.status, "
        "       t.trainId, t.trainNumber, o.passengerName, o.purchaseTime, "
        "       ds.stationName, a_s.stationName, o.travelDate "
        "FROM \"Order\" o "
        "JOIN Trip tr ON o.tripId = tr.tripId "
        "JOIN Train t ON tr.trainId = t.trainId "
        "JOIN Station ds ON t.departureStationId = ds.stationId "
        "JOIN Station a_s ON t.arrivalStationId   = a_s.stationId "
        "ORDER BY o.purchaseTime DESC"
    ));

    QList<OrderWithDetails> results;
    if (!query.exec()) {
        m_lastError = query.lastError().text();
        return results;
    }

    while (query.next()) {
        OrderWithDetails d;
        d.orderId              = query.value(0).toInt();
        d.userId               = query.value(1).toInt();
        d.tripId               = query.value(2).toInt();
        d.status               = query.value(3).toInt();
        d.trainId              = query.value(4).toInt();
        d.trainNumber          = query.value(5).toString();
        d.passengerName        = query.value(6).toString();
        d.purchaseTime         = query.value(7).toString();
        d.departureStationName = query.value(8).toString();
        d.arrivalStationName   = query.value(9).toString();
        d.travelDate           = query.value(10).toString();
        results.append(d);
    }
    return results;
}

// ── Issue 11: countAllOrders ────────────────────────────────────────────
// 统计订单总数（含已退票）；查询失败返回 -1
int DatabaseManager::countAllOrders() const
{
    m_lastError.clear();
    QSqlQuery query(QSqlDatabase::database(m_connectionName));

    if (!query.exec(QStringLiteral("SELECT COUNT(*) FROM \"Order\""))) {
        m_lastError = query.lastError().text();
        return -1;
    }
    return query.next() ? query.value(0).toInt() : 0;
}

// ── Issue 11: countOrdersByStatus ───────────────────────────────────────
// 按状态统计订单数（0=已购票，1=已退票）；查询失败返回 -1
int DatabaseManager::countOrdersByStatus(int status) const
{
    m_lastError.clear();
    QSqlQuery query(QSqlDatabase::database(m_connectionName));

    query.prepare(QStringLiteral(
        "SELECT COUNT(*) FROM \"Order\" WHERE status = :status"
    ));
    query.bindValue(":status", status);

    if (!query.exec()) {
        m_lastError = query.lastError().text();
        return -1;
    }
    return query.next() ? query.value(0).toInt() : 0;
}

// ── Issue 11: popularRoutes ─────────────────────────────────────────────
// 热门线路 Top N：按起讫站分组统计有效订单数，降序取前 limit 条
QList<DatabaseManager::RouteStat>
DatabaseManager::popularRoutes(int limit) const
{
    m_lastError.clear();
    QSqlQuery query(QSqlDatabase::database(m_connectionName));

    // 只统计 status=0（已购票）的订单，退票不计入热度；按站点ID分组避免重名站混淆
    query.prepare(QStringLiteral(
        "SELECT ds.stationName, a_s.stationName, COUNT(*) AS cnt "
        "FROM \"Order\" o "
        "JOIN Trip tr ON o.tripId = tr.tripId "
        "JOIN Train t  ON tr.trainId = t.trainId "
        "JOIN Station ds ON t.departureStationId = ds.stationId "
        "JOIN Station a_s ON t.arrivalStationId   = a_s.stationId "
        "WHERE o.status = 0 "
        "GROUP BY ds.stationId, a_s.stationId "
        "ORDER BY cnt DESC "
        "LIMIT :limit"
    ));
    query.bindValue(":limit", limit);

    QList<RouteStat> results;
    if (!query.exec()) {
        m_lastError = query.lastError().text();
        return results;
    }

    while (query.next()) {
        RouteStat r;
        r.dep   = query.value(0).toString();
        r.arr   = query.value(1).toString();
        r.count = query.value(2).toInt();
        results.append(r);
    }
    return results;
}

// ── Issue 11: monthlyPassengerFlow ──────────────────────────────────────
// 月度客流统计：按购票时间的年月分组，分别汇总总量、购票数和退票数
QList<DatabaseManager::MonthlyStat>
DatabaseManager::monthlyPassengerFlow() const
{
    m_lastError.clear();
    QSqlQuery query(QSqlDatabase::database(m_connectionName));

    // strftime 取 purchaseTime 的 'YYYY-MM'；SUM(CASE...) 按状态分列计数（0=已购，1=已退）
    if (!query.exec(QStringLiteral(
        "SELECT strftime('%Y-%m', purchaseTime) AS month, "
        "       COUNT(*) AS total, "
        "       SUM(CASE WHEN status = 0 THEN 1 ELSE 0 END) AS booked, "
        "       SUM(CASE WHEN status = 1 THEN 1 ELSE 0 END) AS refunded "
        "FROM \"Order\" "
        "GROUP BY month "
        "ORDER BY month DESC"))) {
        m_lastError = query.lastError().text();
        return {};
    }

    QList<MonthlyStat> results;
    while (query.next()) {
        MonthlyStat s;
        s.month    = query.value(0).toString();
        s.total    = query.value(1).toInt();
        s.booked   = query.value(2).toInt();
        s.refunded = query.value(3).toInt();
        results.append(s);
    }
    return results;
}
