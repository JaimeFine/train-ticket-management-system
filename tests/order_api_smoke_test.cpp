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
    train.departureTime = QStringLiteral("14:00");
    train.arrivalTime = QStringLiteral("18:00");
    train.totalSeats = 150;
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

    const auto storedTripId =
        manager.createTrip(storedTrain->trainId, QStringLiteral("2026-07-08"), 150);
    if (!storedTripId.has_value()) {
        qCritical() << "Could not create trip for order test:" << manager.lastError();
        return 1;
    }

    OrderRecord order;
    order.userId = storedUser->userId;
    order.trainId = storedTrain->trainId;
    order.tripId = *storedTripId;
    order.passengerName = QStringLiteral("Smoke Passenger");
    order.travelDate = QStringLiteral("2026-07-08");
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
        createdOrder.tripId != *storedTripId ||
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

/*
$ Test Log:
PS C:\Users\13647\OneDrive\Desktop\train> cmake -S . -B build -DBUILD_TESTS=ON
-- Selecting Windows SDK version 10.0.26100.0 to target Windows 10.0.26200.
-- Could NOT find WrapVulkanHeaders (missing: Vulkan_INCLUDE_DIR)
-- Using Qt 6.9
-- Configuring done (0.2s)
-- Generating done (1.3s)
-- Build files have been written to: C:/Users/13647/OneDrive/Desktop/train/build
PS C:\Users\13647\OneDrive\Desktop\train> cmake --build build --config Debug
适用于 .NET Framework MSBuild 版本 18.7.8+1ac568fee

  Automatic MOC and UIC for target TrainTicketSystem
  database_manager_order.cpp
  TrainTicketSystem.vcxproj -> C:\Users\13647\OneDrive\Desktop\train\build\Debug\TrainTicketSystem.exe
  Automatic MOC and UIC for target database_init_smoke_test
  database_init_smoke_test.vcxproj -> C:\Users\13647\OneDrive\Desktop\train\build\tests\Debug\database_init_smoke_test.
  exe
  Automatic MOC and UIC for target order_api_smoke_test
  Building Custom Rule C:/Users/13647/OneDrive/Desktop/train/tests/CMakeLists.txt
  mocs_compilation_Debug.cpp
  order_api_smoke_test.cpp
  database_manager.cpp
  database_manager_user.cpp
  database_manager_station.cpp
  database_manager_train.cpp
  database_manager_order.cpp
  正在生成代码...
  order_api_smoke_test.vcxproj -> C:\Users\13647\OneDrive\Desktop\train\build\tests\Debug\order_api_smoke_test.exe
  Automatic MOC and UIC for target station_api_smoke_test
  station_api_smoke_test.vcxproj -> C:\Users\13647\OneDrive\Desktop\train\build\tests\Debug\station_api_smoke_test.exe
  Automatic MOC and UIC for target train_api_smoke_test
  train_api_smoke_test.cpp
  train_api_smoke_test.vcxproj -> C:\Users\13647\OneDrive\Desktop\train\build\tests\Debug\train_api_smoke_test.exe
  Automatic MOC and UIC for target user_api_smoke_test
  user_api_smoke_test.vcxproj -> C:\Users\13647\OneDrive\Desktop\train\build\tests\Debug\user_api_smoke_test.exe
PS C:\Users\13647\OneDrive\Desktop\train> ctest --test-dir build -C Debug -R order_api_smoke_test --output-on-failure
Test project C:/Users/13647/OneDrive/Desktop/train/build
    Start 5: order_api_smoke_test
1/1 Test #5: order_api_smoke_test .............   Passed    0.19 sec

100% tests passed, 0 tests failed out of 1

Total Test time (real) =   0.31 sec
PS C:\Users\13647\OneDrive\Desktop\train>
*/
