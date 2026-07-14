#include "database_manager.h"
#include "ticket_manager.h"

#include <QCoreApplication>
#include <QDateTime>
#include <QDebug>

namespace {
QString makeUniqueName(const QString &prefix)
{
    return QStringLiteral("%1_%2")
        .arg(prefix)
        .arg(QDateTime::currentMSecsSinceEpoch());
}
}

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);

    DatabaseManager manager;
    if (!manager.initialize()) {
        qCritical() << "Initialization failed:" << manager.lastError();
        return 1;
    }

    TicketManager ticketManager(manager);

    UserRecord user;
    user.username = makeUniqueName(QStringLiteral("issue9_user"));
    user.password = QStringLiteral("issue9_pass");
    user.role = 1;
    user.enabled = true;

    if (!manager.addUser(user)) {
        qCritical() << "addUser() failed:" << manager.lastError();
        return 1;
    }

    const auto storedUser = manager.findUserByUsername(user.username);
    if (!storedUser.has_value()) {
        qCritical() << "Could not reload created user.";
        return 1;
    }

    StationRecord departureStation;
    departureStation.stationName = makeUniqueName(QStringLiteral("issue9_dep"));
    if (!manager.addStation(departureStation)) {
        qCritical() << "addStation() for departure failed:" << manager.lastError();
        return 1;
    }

    StationRecord arrivalStation;
    arrivalStation.stationName = makeUniqueName(QStringLiteral("issue9_arr"));
    if (!manager.addStation(arrivalStation)) {
        qCritical() << "addStation() for arrival failed:" << manager.lastError();
        return 1;
    }

    const auto departureRecord =
        manager.findStationByName(departureStation.stationName);
    const auto arrivalRecord =
        manager.findStationByName(arrivalStation.stationName);

    if (!departureRecord.has_value() || !arrivalRecord.has_value()) {
        qCritical() << "Could not reload station ids.";
        return 1;
    }

    TrainRecord train;
    train.trainNumber = makeUniqueName(QStringLiteral("ISSUE9_TRAIN"));
    train.departureStationId = departureRecord->stationId;
    train.arrivalStationId = arrivalRecord->stationId;
    train.departureTime = QStringLiteral("2026-07-10 09:00:00");
    train.arrivalTime = QStringLiteral("2026-07-10 12:00:00");
    train.totalSeats = 80;
    train.enabled = true;

    if (!manager.addTrain(train)) {
        qCritical() << "addTrain() failed:" << manager.lastError();
        return 1;
    }

    const auto storedTrain = manager.findTrainByNumber(train.trainNumber);
    if (!storedTrain.has_value()) {
        qCritical() << "Could not reload created train.";
        return 1;
    }

    const QString travelDate = QStringLiteral("2026-07-10");
    const auto tripId =
        manager.createTrip(storedTrain->trainId, travelDate, storedTrain->totalSeats);
    if (!tripId.has_value()) {
        qCritical() << "createTrip() failed:" << manager.lastError();
        return 1;
    }

    const int initialSeats = ticketManager.remainingSeats(*tripId);
    if (initialSeats != 80) {
        qCritical() << "Unexpected initial remaining seats:" << initialSeats;
        return 1;
    }

    const auto searchByStations = ticketManager.searchTrips(
        departureStation.stationName,
        arrivalStation.stationName,
        travelDate
    );
    if (searchByStations.isEmpty()) {
        qCritical() << "searchTrains() returned no result.";
        return 1;
    }

    const auto searchByNumber =
        ticketManager.searchByTrainNumber(train.trainNumber);
    if (searchByNumber.size() != 1) {
        qCritical() << "searchByTrainNumber() returned unexpected result count:"
                    << searchByNumber.size();
        return 1;
    }

    const int createdOrderId = ticketManager.bookTicket(
        storedUser->userId,
        *tripId,
        QStringLiteral("Issue9 Passenger")
    );
    if (createdOrderId <= 0) {
        qCritical() << "bookTicket() failed:" << ticketManager.lastError();
        return 1;
    }

    const int seatsAfterBooking =
        ticketManager.remainingSeats(*tripId);
    if (seatsAfterBooking != 79) {
        qCritical() << "Remaining seats did not decrease correctly:"
                    << seatsAfterBooking;
        return 1;
    }

    const auto storedOrders = manager.findOrdersByUser(storedUser->userId);
    if (storedOrders.isEmpty()) {
        qCritical() << "No order found after booking.";
        return 1;
    }

    bool orderFound = false;
    for (const auto &order : storedOrders) {
        if (order.orderId == createdOrderId &&
            order.tripId == *tripId &&
            order.passengerName == QStringLiteral("Issue9 Passenger") &&
            order.status == 0) {
            orderFound = true;
            break;
        }
    }

    if (!orderFound) {
        qCritical() << "Created order id was not found in stored orders.";
        return 1;
    }

    qDebug() << "Issue 9 smoke test passed.";
    qDebug() << "Created order id:" << createdOrderId;
    qDebug() << "Remaining seats after booking:" << seatsAfterBooking;
    return 0;
}

/*
$ Test Log:
PS C:\Users\13647\OneDrive\Desktop\train> cmake --build build --config Debug
适用于 .NET Framework MSBuild 版本 18.7.8+1ac568fee

  Automatic MOC and UIC for target TrainTicketSystem
  ticket_manager.cpp
  database_manager_station.cpp
  database_manager_order.cpp
  正在生成代码...
  TrainTicketSystem.vcxproj -> C:\Users\13647\OneDrive\Desktop\train\build\Debug\TrainTicketSystem.exe
  Automatic MOC and UIC for target database_init_smoke_test
  database_init_smoke_test.vcxproj -> C:\Users\13647\OneDrive\Desktop\train\build\tests\Debug\database_init_smoke_test.
  exe
  Automatic MOC and UIC for target login_role_permission_test
  login_role_permission_test.vcxproj -> C:\Users\13647\OneDrive\Desktop\train\build\tests\Debug\login_role_permission_t
  est.exe
  Automatic MOC and UIC for target order_api_smoke_test
  order_api_smoke_test.cpp
  database_manager.cpp
  database_manager_user.cpp
  database_manager_station.cpp
  database_manager_train.cpp
  database_manager_order.cpp
  正在生成代码...
  order_api_smoke_test.vcxproj -> C:\Users\13647\OneDrive\Desktop\train\build\tests\Debug\order_api_smoke_test.exe
  Automatic MOC and UIC for target station_api_smoke_test
  database_manager.cpp
  database_manager_station.cpp
  station_api_smoke_test.cpp
  正在生成代码...
  station_api_smoke_test.vcxproj -> C:\Users\13647\OneDrive\Desktop\train\build\tests\Debug\station_api_smoke_test.exe
  Automatic MOC and UIC for target ticket_issue9_smoke_test
  Building Custom Rule C:/Users/13647/OneDrive/Desktop/train/tests/CMakeLists.txt
  mocs_compilation_Debug.cpp
  ticket_issue9_smoke_test.cpp
  ticket_manager.cpp
  database_manager.cpp
  database_manager_user.cpp
  database_manager_station.cpp
  database_manager_train.cpp
  database_manager_order.cpp
  database_manager_ticket.cpp
  正在生成代码...
  ticket_issue9_smoke_test.vcxproj -> C:\Users\13647\OneDrive\Desktop\train\build\tests\Debug\ticket_issue9_smoke_test.
  exe
  Automatic MOC and UIC for target train_api_smoke_test
  database_manager.cpp
  database_manager_station.cpp
  database_manager_train.cpp
  train_api_smoke_test.cpp
  正在生成代码...
  train_api_smoke_test.vcxproj -> C:\Users\13647\OneDrive\Desktop\train\build\tests\Debug\train_api_smoke_test.exe
  Automatic MOC and UIC for target user_api_smoke_test
  database_manager.cpp
  database_manager_user.cpp
  user_api_smoke_test.cpp
  正在生成代码...
  user_api_smoke_test.vcxproj -> C:\Users\13647\OneDrive\Desktop\train\build\tests\Debug\user_api_smoke_test.exe
PS C:\Users\13647\OneDrive\Desktop\train> ctest --test-dir build -C Debug -R ticket_issue9_smoke_test --output-on-failure
Test project C:/Users/13647/OneDrive/Desktop/train/build
    Start 4: ticket_issue9_smoke_test
1/1 Test #4: ticket_issue9_smoke_test .........   Passed    0.42 sec

100% tests passed, 0 tests failed out of 1

Total Test time (real) =   0.76 sec
PS C:\Users\13647\OneDrive\Desktop\train>
*/
