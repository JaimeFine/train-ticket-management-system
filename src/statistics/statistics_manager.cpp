#include "statistics_manager.h"
#include "database_manager.h"

StatisticsManager::StatisticsManager(DatabaseManager &db) : m_db(db) {}

int StatisticsManager::totalOrders() const  { return m_db.countAllOrders(); }
int StatisticsManager::totalBooked() const  { return m_db.countOrdersByStatus(0); }
int StatisticsManager::totalRefunded() const{ return m_db.countOrdersByStatus(1); }
int StatisticsManager::totalChanged() const { return m_db.countOrdersByStatus(2); }

QVector<QVariantMap> StatisticsManager::popularRoutes(int limit) const
{
    QVector<QVariantMap> result;
    auto routes = m_db.popularRoutes(limit);
    for (const auto &route : routes) {
        QVariantMap map;
        map["departureStation"] = route.dep;
        map["arrivalStation"]   = route.arr;
        map["orderCount"]       = route.count;
        result.append(map);
    }
    return result;
}

QVector<QVariantMap> StatisticsManager::monthlyPassengerFlow() const
{
    QVector<QVariantMap> result;
    auto stats = m_db.monthlyPassengerFlow();
    for (const auto &stat : stats) {
        QVariantMap map;
        map["month"]       = stat.month;
        map["totalOrders"] = stat.total;
        map["booked"]      = stat.booked;
        map["refunded"]    = stat.refunded;
        result.append(map);
    }
    return result;
}
