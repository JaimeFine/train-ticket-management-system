#include "statistics_manager.h"
#include "database_manager.h"

StatisticsManager::StatisticsManager(DatabaseManager &db) : m_db(db) {}

int StatisticsManager::totalOrders()  const { return m_db.countAllOrders(); }
int StatisticsManager::totalBooked()  const { return m_db.countOrdersByStatus(0); }
int StatisticsManager::totalRefunded()const { return m_db.countOrdersByStatus(1); }
int StatisticsManager::totalChanged() const { return m_db.countOrdersByStatus(2); }
QVector<QVariantMap> StatisticsManager::popularRoutes(int limit) const {
    QVector<QVariantMap> r;
    for(auto &x:m_db.popularRoutes(limit)){
        QVariantMap m;m["departureStation"]=x.dep;m["arrivalStation"]=x.arr;
        m["orderCount"]=x.count;r.append(m);
    }
    return r;
}
QVector<QVariantMap> StatisticsManager::monthlyPassengerFlow() const {
    QVector<QVariantMap> r;
    for(auto &x:m_db.monthlyPassengerFlow()){
        QVariantMap m;m["month"]=x.month;m["totalOrders"]=x.total;
        m["booked"]=x.booked;m["refunded"]=x.refunded;r.append(m);
    }
    return r;
}
QString StatisticsManager::lastError() const { return m_lastError; }
