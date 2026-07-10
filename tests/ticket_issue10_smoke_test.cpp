/**
 * @file ticket_issue10_smoke_test.cpp
 * Smoke test for Issue 10: refund tickets and change tickets.
 */
#include "database_manager.h"
#include "ticket_manager.h"

#include <QCoreApplication>
#include <QDateTime>
#include <QDebug>

namespace {
QString makeUnique(const QString &prefix) {
    return QStringLiteral("%1_%2")
        .arg(prefix)
        .arg(QDateTime::currentMSecsSinceEpoch());
}
}

int main(int argc, char *argv[]) {
    QCoreApplication app(argc, argv);

    DatabaseManager db;
    if (!db.initialize()) {
        qCritical() << "DB init failed:" << db.lastError();
        return 1;
    }

    UserRecord user;
    user.username = makeUnique(QStringLiteral("issue10_user"));
    user.password = QStringLiteral("pw");
    user.role = 1;
    user.enabled = true;
    if (!db.addUser(user)) {
        qCritical() << "addUser failed:" << db.lastError();
        return 1;
    }

    const std::optional<UserRecord> storedUser =
        db.findUserByUsername(user.username);
    if (!storedUser.has_value()) {
        qCritical() << "Could not reload created user.";
        return 1;
    }

    StationRecord depStation;
    depStation.stationName = makeUnique(QStringLiteral("issue10_dep"));
    StationRecord arrStation;
    arrStation.stationName = makeUnique(QStringLiteral("issue10_arr"));

    if (!db.addStation(depStation) || !db.addStation(arrStation)) {
        qCritical() << "addStation failed:" << db.lastError();
        return 1;
    }

    const std::optional<StationRecord> dep =
        db.findStationByName(depStation.stationName);
    const std::optional<StationRecord> arr =
        db.findStationByName(arrStation.stationName);
    if (!dep.has_value() || !arr.has_value()) {
        qCritical() << "Could not reload created stations.";
        return 1;
    }

    TrainRecord trainA;
    trainA.trainNumber = makeUnique(QStringLiteral("ISSUE10_A"));
    trainA.departureStationId = dep->stationId;
    trainA.arrivalStationId = arr->stationId;
    trainA.departureTime = QStringLiteral("2026-07-10 08:00");
    trainA.arrivalTime = QStringLiteral("2026-07-10 12:00");
    trainA.totalSeats = 10;
    trainA.remainingSeats = 10;
    trainA.enabled = true;

    TrainRecord trainB;
    trainB.trainNumber = makeUnique(QStringLiteral("ISSUE10_B"));
    trainB.departureStationId = dep->stationId;
    trainB.arrivalStationId = arr->stationId;
    trainB.departureTime = QStringLiteral("2026-07-10 13:00");
    trainB.arrivalTime = QStringLiteral("2026-07-10 17:00");
    trainB.totalSeats = 8;
    trainB.remainingSeats = 8;
    trainB.enabled = true;

    if (!db.addTrain(trainA) || !db.addTrain(trainB)) {
        qCritical() << "addTrain failed:" << db.lastError();
        return 1;
    }

    const std::optional<TrainRecord> storedTrainA =
        db.findTrainByNumber(trainA.trainNumber);
    const std::optional<TrainRecord> storedTrainB =
        db.findTrainByNumber(trainB.trainNumber);
    if (!storedTrainA.has_value() || !storedTrainB.has_value()) {
        qCritical() << "Could not reload created trains.";
        return 1;
    }

    TicketManager tm(db);

    // Refund flow
    const int refundOrderId = tm.bookTicket(
        storedUser->userId, storedTrainA->trainId, QStringLiteral("Refund Passenger")
    );
    if (refundOrderId < 0) {
        qCritical() << "bookTicket for refund flow failed:" << tm.lastError();
        return 1;
    }

    if (tm.remainingSeats(storedTrainA->trainId) != 9) {
        qCritical() << "Seat count did not decrease after booking refund flow order.";
        return 1;
    }

    if (!tm.refundTicket(refundOrderId)) {
        qCritical() << "refundTicket failed:" << tm.lastError();
        return 1;
    }

    const std::optional<OrderRecord> refundedOrder = db.findOrderById(refundOrderId);
    if (!refundedOrder.has_value() || refundedOrder->status != 1) {
        qCritical() << "Refunded order status was not updated to 1.";
        return 1;
    }

    if (tm.remainingSeats(storedTrainA->trainId) != 10) {
        qCritical() << "Seat count was not restored after refund.";
        return 1;
    }

    qDebug() << "PASS: refundTicket restored seat and updated order status.";

    // Change-ticket flow
    const int changeOrderId = tm.bookTicket(
        storedUser->userId, storedTrainA->trainId, QStringLiteral("Change Passenger")
    );
    if (changeOrderId < 0) {
        qCritical() << "bookTicket for change flow failed:" << tm.lastError();
        return 1;
    }

    if (tm.remainingSeats(storedTrainA->trainId) != 9) {
        qCritical() << "Seat count did not decrease after booking change flow order.";
        return 1;
    }

    if (tm.remainingSeats(storedTrainB->trainId) != 8) {
        qCritical() << "Unexpected initial seat count for target train.";
        return 1;
    }

    if (!tm.changeTicket(changeOrderId, storedTrainB->trainId)) {
        qCritical() << "changeTicket failed:" << tm.lastError();
        return 1;
    }

    const std::optional<OrderRecord> changedOldOrder = db.findOrderById(changeOrderId);
    if (!changedOldOrder.has_value() || changedOldOrder->status != 2) {
        qCritical() << "Original order status was not updated to 2 after change.";
        return 1;
    }

    if (tm.remainingSeats(storedTrainA->trainId) != 10) {
        qCritical() << "Old train seat was not restored after change ticket.";
        return 1;
    }

    if (tm.remainingSeats(storedTrainB->trainId) != 7) {
        qCritical() << "New train seat was not deducted after change ticket.";
        return 1;
    }

    const QList<OrderRecord> orders = db.findOrdersByUser(storedUser->userId);
    bool foundReplacementOrder = false;
    for (const OrderRecord &order : orders) {
        if (order.orderId != changeOrderId &&
            order.trainId == storedTrainB->trainId &&
            order.passengerName == QStringLiteral("Change Passenger") &&
            order.status == 0) {
            foundReplacementOrder = true;
            break;
        }
    }

    if (!foundReplacementOrder) {
        qCritical() << "Could not find the replacement order created by changeTicket.";
        return 1;
    }

    qDebug() << "PASS: changeTicket updated old order, created new order, and moved seat inventory.";
    qDebug() << "Issue 10 smoke test passed.";
    return 0;
}
