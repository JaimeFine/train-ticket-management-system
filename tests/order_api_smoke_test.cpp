#include "database_manager.h"

#include <QCoreApplication>
#include <QDateTime>
#include <QDebug>

namespace {
QString makeUniqueName(const QString &prefix) {
    return QStringLiteral("%1_%2")
        .arg(prefix)
        .arg(QDateTime::currentMSecsSinceEpoch());
}
}

int main(int argc, char *argv[]) {
    QCoreApplication app(argc, argv);

    DatabaseManager manager;
    if (!manager.initialize()) {
        qCritical() << "Initialization failed:" << manager.lastError();
        return 1;
    }

    UserRecord user;
    user.username = makeUniqueName(QStringLiteral("order_user"));
    user.password = QStringLiteral("order_password");
    user.role = 1;
    user.enabled = true;

    if (!manager.addUser(user)) {
        qCritical() << "addUser() failed:" << manager.lastError();
        return 1;
    }

    const std::optional<UserRecord> storedUser =
        manager.findUserByUsername(user.username);
    if (!storedUser.has_value()) {
        qCritical() << "Could not reload created user.";
        return 1;
    }

    StationRecord departureStation;
    departureStation.stationName = makeUniqueName(QStringLiteral("order_dep"));
    if (!manager.addStation(departureStation)) {
        qCritical() << "addStation() for departure failed:" << manager.lastError();
        return 1;
    }

    StationRecord arrivalStation;
    arrivalStation.stationName = makeUniqueName(QStringLiteral("order_arr"));
    if (!manager.addStation(arrivalStation)) {
        qCritical() << "addStation() for arrival failed:" << manager.lastError();
        return 1;
    }

    const std::optional<StationRecord> departureRecord =
        manager.findStationByName(departureStation.stationName);
    const std::optional<StationRecord> arrivalRecord =
        manager.findStationByName(arrivalStation.stationName);

    if (!departureRecord.has_value() || !arrivalRecord.has_value()) {
        qCritical() << "Could not reload station ids for train creation.";
        return 1;
    }

    TrainRecord train;
    train.trainNumber = makeUniqueName(QStringLiteral("order_train"));
    train.departureStationId = departureRecord->stationId;
    train.arrivalStationId = arrivalRecord->stationId;
    train.departureTime = QStringLiteral("2026-07-08 14:00");
    train.arrivalTime = QStringLiteral("2026-07-08 18:00");
    train.totalSeats = 150;
    train.remainingSeats = 150;
    train.enabled = true;

    if (!manager.addTrain(train)) {
        qCritical() << "addTrain() failed:" << manager.lastError();
        return 1;
    }

    const std::optional<TrainRecord> storedTrain =
        manager.findTrainByNumber(train.trainNumber);
    if (!storedTrain.has_value()) {
        qCritical() << "Could not reload created train.";
        return 1;
    }

    OrderRecord order;
    order.userId = storedUser->userId;
    order.trainId = storedTrain->trainId;
    order.passengerName = QStringLiteral("Smoke Passenger");
    order.purchaseTime = QStringLiteral("2026-07-08 13:30");
    order.status = 0;

    if (!manager.createOrder(order)) {
        qCritical() << "createOrder() failed:" << manager.lastError();
        return 1;
    }

    QList<OrderRecord> orders = manager.findOrdersByUser(storedUser->userId);
    if (orders.isEmpty()) {
        qCritical() << "findOrdersByUser() returned no orders.";
        return 1;
    }

    const OrderRecord createdOrder = orders.back();
    if (createdOrder.userId != storedUser->userId ||
        createdOrder.trainId != storedTrain->trainId ||
        createdOrder.passengerName != order.passengerName) {
        qCritical() << "Order data mismatch after creation.";
        return 1;
    }

    if (!manager.updateOrderStatus(createdOrder.orderId, 2)) {
        qCritical() << "updateOrderStatus() failed:" << manager.lastError();
        return 1;
    }

    orders = manager.findOrdersByUser(storedUser->userId);
    if (orders.isEmpty()) {
        qCritical() << "Order disappeared after status update.";
        return 1;
    }

    const OrderRecord updatedOrder = orders.back();
    if (updatedOrder.orderId != createdOrder.orderId ||
        updatedOrder.status != 2) {
        qCritical() << "Order status was not updated correctly.";
        return 1;
    }

    qDebug() << "Order API smoke test passed.";
    qDebug() << "Created order id:" << updatedOrder.orderId;
    qDebug() << "Created order passenger:" << updatedOrder.passengerName;
    return 0;
}
