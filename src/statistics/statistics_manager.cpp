#include "statistics_manager.h"
#include "database_manager.h"

StatisticsManager::StatisticsManager(DatabaseManager &db) : m_db(db) {}

// ══════════════════════════════════════════════════════════════════════════════
// 总览 — 通过 DatabaseManager aggregate API
// ══════════════════════════════════════════════════════════════════════════════

int StatisticsManager::totalOrders() const  { return m_db.countAllOrders(); }
int StatisticsManager::totalBooked() const  { return m_db.countOrdersByStatus(0); }
int StatisticsManager::totalRefunded() const{ return m_db.countOrdersByStatus(1); }
int StatisticsManager::totalChanged() const { return m_db.countOrdersByStatus(2); }

// ══════════════════════════════════════════════════════════════════════════════
// 热门路线
// ══════════════════════════════════════════════════════════════════════════════

QVector<QVariantMap> StatisticsManager::popularRoutes(int limit) const
{
    QVector<QVariantMap> result;
    for (auto &r : m_db.popularRoutes(limit)) {
        QVariantMap m;
        m["departureStation"] = r.dep;
        m["arrivalStation"] = r.arr;
        m["orderCount"] = r.count;
        result.append(m);
    }
    return result;
}

// ══════════════════════════════════════════════════════════════════════════════
// 月度客流
// ══════════════════════════════════════════════════════════════════════════════

QVector<QVariantMap> StatisticsManager::monthlyPassengerFlow() const
{
    QVector<QVariantMap> result;
    for (auto &m : m_db.monthlyPassengerFlow()) {
        QVariantMap map;
        map["month"] = m.month;
        map["totalOrders"] = m.total;
        map["booked"] = m.booked;
        map["refunded"] = m.refunded;
        result.append(map);
    }
    return result;
}

// ══════════════════════════════════════════════════════════════════════════════
// 退票统计 + 每车次销售 — V1 简化（数据可从 monthly/popularRoutes 间接获取）
// ══════════════════════════════════════════════════════════════════════════════

QVector<QVariantMap> StatisticsManager::refundStatistics() const { return {}; }
QVector<QVariantMap> StatisticsManager::salesPerTrain() const { return {}; }

QString StatisticsManager::lastError() const { return m_lastError; }
