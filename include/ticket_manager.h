#pragma once
/** @file ticket_manager.h - Issue 9: 订票 + 余票查询 + 车次搜索 */
#include <QString>
#include <QVector>
#include <QVariantMap>
class DatabaseManager;

class TicketManager {
public:
    explicit TicketManager(DatabaseManager &db);
    int  bookTicket(int userId,int trainId,const QString &passengerName);
    int  remainingSeats(int trainId) const;
    QVector<QVariantMap> searchTrains(const QString &dep,const QString &arr,const QString &date="") const;
    QVector<QVariantMap> searchByTrainNumber(const QString &number) const;
    QString lastError() const;
private:
    DatabaseManager &m_db; QString m_lastError;
};
