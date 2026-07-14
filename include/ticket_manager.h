#pragma once
/** @file ticket_manager.h - V2: tripId-based ticketing */
#include <QString>
#include <QVector>
#include <QVariantMap>
class DatabaseManager;

class TicketManager {
public:
    explicit TicketManager(DatabaseManager &db);

    // Issue 9 (V2: tripId)
    int  bookTicket(int userId, int tripId, const QString &passengerName);
    int  remainingSeats(int tripId) const;
    QVector<QVariantMap> searchTrips(const QString &dep,const QString &arr,const QString &date="") const;
    QVector<QVariantMap> searchByTrainNumber(const QString &number) const;

    // Issue 10 (V2: tripId)
    bool refundTicket(int orderId);
    bool changeTicket(int orderId, int newTripId);
    QVector<QVariantMap> queryOrdersByUser(int userId) const;
    QVector<QVariantMap> queryOrdersByPassenger(const QString &name) const;
    QVector<QVariantMap> queryOrderByOrderId(int orderId) const;

    // Issue 11
    QVector<QVariantMap> queryAllOrders() const;

    bool addOperationLog(const QString &operatorUsername,
                         const QString &action, const QString &detail);
    QString lastError() const;
private:
    DatabaseManager &m_db; QString m_lastError;
};
