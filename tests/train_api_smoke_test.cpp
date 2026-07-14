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

    StationRecord departureStation;
    departureStation.stationName = makeUniqueName(QStringLiteral("dep_station"));
    if (!manager.addStation(departureStation)) {
        qCritical() << "Failed to add departure station:" << manager.lastError();
        return 1;
    }

    StationRecord arrivalStation;
    arrivalStation.stationName = makeUniqueName(QStringLiteral("arr_station"));
    if (!manager.addStation(arrivalStation)) {
        qCritical() << "Failed to add arrival station:" << manager.lastError();
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
    train.trainNumber = makeUniqueName(QStringLiteral("G_smoke"));
    train.departureStationId = departureRecord->stationId;
    train.arrivalStationId = arrivalRecord->stationId;
    train.departureTime = QStringLiteral("2026-07-08 09:00");
    train.arrivalTime = QStringLiteral("2026-07-08 12:30");
    train.totalSeats = 120;
    train.enabled = true;

    if (!manager.addTrain(train)) {
        qCritical() << "addTrain() failed:" << manager.lastError();
        return 1;
    }

    const std::optional<TrainRecord> byNumber =
        manager.findTrainByNumber(train.trainNumber);

    if (!byNumber.has_value()) {
        qCritical() << "findTrainByNumber() returned no result.";
        return 1;
    }

    if (byNumber->trainNumber != train.trainNumber) {
        qCritical() << "Train number mismatch after insert.";
        return 1;
    }

    const std::optional<TrainRecord> byId =
        manager.findTrainById(byNumber->trainId);

    if (!byId.has_value()) {
        qCritical() << "findTrainById() returned no result.";
        return 1;
    }

    if (byId->trainId != byNumber->trainId) {
        qCritical() << "Train id mismatch between lookup methods.";
        return 1;
    }

    TrainRecord updatedTrain = *byId;
    updatedTrain.departureTime = QStringLiteral("2026-07-08 09:30");
    updatedTrain.arrivalTime = QStringLiteral("2026-07-08 13:00");
    updatedTrain.enabled = false;

    if (!manager.updateTrain(updatedTrain)) {
        qCritical() << "updateTrain() failed:" << manager.lastError();
        return 1;
    }

    const std::optional<TrainRecord> afterUpdate =
        manager.findTrainById(updatedTrain.trainId);

    if (!afterUpdate.has_value()) {
        qCritical() << "Train disappeared after update.";
        return 1;
    }

    if (afterUpdate->departureTime != updatedTrain.departureTime ||
        afterUpdate->arrivalTime != updatedTrain.arrivalTime ||
        afterUpdate->enabled != updatedTrain.enabled) {
        qCritical() << "Updated train fields were not saved correctly.";
        return 1;
    }

    qDebug() << "Train API smoke test passed.";
    qDebug() << "Created train id:" << afterUpdate->trainId;
    qDebug() << "Created train number:" << afterUpdate->trainNumber;
    return 0;
}

/*
$ Test Log:
PS C:\Users\13647\OneDrive\Desktop\train> cmake -S . -B build -DBUILD_TESTS=ON
-- Selecting Windows SDK version 10.0.26100.0 to target Windows 10.0.26200.
-- Could NOT find WrapVulkanHeaders (missing: Vulkan_INCLUDE_DIR)
-- Using Qt 6.9
-- Configuring done (0.3s)
-- Generating done (1.2s)
-- Build files have been written to: C:/Users/13647/OneDrive/Desktop/train/build
PS C:\Users\13647\OneDrive\Desktop\train> cmake --build build --config Debug
适用于 .NET Framework MSBuild 版本 18.7.8+1ac568fee

  Automatic MOC and UIC for target TrainTicketSystem
  database_manager_train.cpp
  TrainTicketSystem.vcxproj -> C:\Users\13647\OneDrive\Desktop\train\build\Debug\TrainTicketSystem.exe
  Automatic MOC and UIC for target database_init_smoke_test
  database_init_smoke_test.cpp
  database_init_smoke_test.vcxproj -> C:\Users\13647\OneDrive\Desktop\train\build\tests\Debug\database_init_smoke_test.
  exe
  Automatic MOC and UIC for target station_api_smoke_test
  station_api_smoke_test.cpp
  station_api_smoke_test.vcxproj -> C:\Users\13647\OneDrive\Desktop\train\build\tests\Debug\station_api_smoke_test.exe
  Automatic MOC and UIC for target train_api_smoke_test
  Building Custom Rule C:/Users/13647/OneDrive/Desktop/train/tests/CMakeLists.txt
  mocs_compilation_Debug.cpp
  train_api_smoke_test.cpp
  database_manager.cpp
  database_manager_station.cpp
  database_manager_train.cpp
  正在生成代码...
  train_api_smoke_test.vcxproj -> C:\Users\13647\OneDrive\Desktop\train\build\tests\Debug\train_api_smoke_test.exe
  Automatic MOC and UIC for target user_api_smoke_test
  user_api_smoke_test.cpp
  user_api_smoke_test.vcxproj -> C:\Users\13647\OneDrive\Desktop\train\build\tests\Debug\user_api_smoke_test.exe
PS C:\Users\13647\OneDrive\Desktop\train> ctest --test-dir build -C Debug -R train_api_smoke_test --output-on-failure
Test project C:/Users/13647/OneDrive/Desktop/train/build
    Start 4: train_api_smoke_test
1/1 Test #4: train_api_smoke_test .............   Passed    0.25 sec

100% tests passed, 0 tests failed out of 1

Total Test time (real) =   0.40 sec
PS C:\Users\13647\OneDrive\Desktop\train>
*/
