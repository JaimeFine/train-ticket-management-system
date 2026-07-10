#pragma once
/** @file statistics_manager.h - Issue 11 */
#include <QString>
#include <QVariantMap>
#include <QVector>
class DatabaseManager;

class StatisticsManager {
public:
    explicit StatisticsManager(DatabaseManager &db);
    int totalOrders() const;
    int totalBooked() const;
    int totalRefunded() const;
    int totalChanged() const;
    QVector<QVariantMap> popularRoutes(int limit=10) const;
    QVector<QVariantMap> monthlyPassengerFlow() const;
    QString lastError() const;
private:
    DatabaseManager &m_db; QString m_lastError;
};
