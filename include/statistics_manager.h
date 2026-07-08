#pragma once

#include <QString>
#include <QVariantMap>
#include <QVector>

class DatabaseManager;

/**
 * @brief 统计管理器 — 只读统计查询
 *
 * 职责：销售统计、热门路线排名、月度客流、退票统计
 * 所有 SQL 通过 DatabaseManager 执行。
 */
class StatisticsManager
{
public:
    explicit StatisticsManager(DatabaseManager &db);

    // ── 销售统计（每车次）────────────────────────────────────────
    QVector<QVariantMap> salesPerTrain() const;

    // ── 热门路线排名（按订单数）──────────────────────────────────
    QVector<QVariantMap> popularRoutes(int limit = 10) const;

    // ── 月度客流汇总 ─────────────────────────────────────────────
    QVector<QVariantMap> monthlyPassengerFlow() const;

    // ── 退票统计 ─────────────────────────────────────────────────
    QVector<QVariantMap> refundStatistics() const;

    // ── 总览数据 ─────────────────────────────────────────────────
    int totalOrders() const;
    int totalBooked() const;
    int totalRefunded() const;
    int totalChanged() const;

    QString lastError() const;

private:
    DatabaseManager &m_db;
    QString          m_lastError;
};
