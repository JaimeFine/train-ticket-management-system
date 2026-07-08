#include "database_manager.h"

#include <QCoreApplication>
#include <QDateTime>
#include <QDebug>

namespace {
QString makeUniqueStationName() {
    return QStringLiteral("smoke_station_%1")
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

    StationRecord newStation;
    newStation.stationName = makeUniqueStationName();

    if (!manager.addStation(newStation)) {
        qCritical() << "addStation() failed:" << manager.lastError();
        return 1;
    }

    const std::optional<StationRecord> byName =
        manager.findStationByName(newStation.stationName);

    if (!byName.has_value()) {
        qCritical() << "findStationByName() returned no result.";
        return 1;
    }

    if (byName->stationName != newStation.stationName) {
        qCritical() << "Station name mismatch after insert.";
        return 1;
    }

    const std::optional<StationRecord> byId =
        manager.findStationById(byName->stationId);

    if (!byId.has_value()) {
        qCritical() << "findStationById() returned no result.";
        return 1;
    }

    if (byId->stationId != byName->stationId ||
        byId->stationName != byName->stationName) {
        qCritical() << "Station lookup mismatch between id and name queries.";
        return 1;
    }

    qDebug() << "Station API smoke test passed.";
    qDebug() << "Created station id:" << byId->stationId;
    qDebug() << "Created station name:" << byId->stationName;
    return 0;
}

/*
$ Test Log:
PS C:\Users\13647\OneDrive\Desktop\train> cmake -S . -B build -DBUILD_TESTS=ON
-- Selecting Windows SDK version 10.0.26100.0 to target Windows 10.0.26200.
-- Could NOT find WrapVulkanHeaders (missing: Vulkan_INCLUDE_DIR)
-- Using Qt 6.9
-- Configuring done (0.3s)
-- Generating done (1.4s)
-- Build files have been written to: C:/Users/13647/OneDrive/Desktop/train/build
PS C:\Users\13647\OneDrive\Desktop\train> cmake --build build --config Debug
适用于 .NET Framework MSBuild 版本 18.7.8+1ac568fee

  Automatic MOC and UIC for target TrainTicketSystem
  database_manager_station.cpp
  TrainTicketSystem.vcxproj -> C:\Users\13647\OneDrive\Desktop\train\build\Debug\TrainTicketSystem.exe
  Automatic MOC and UIC for target database_init_smoke_test
  database_init_smoke_test.vcxproj -> C:\Users\13647\OneDrive\Desktop\train\build\tests\Debug\database_init_smoke_test.
  exe
  Automatic MOC and UIC for target station_api_smoke_test
  Building Custom Rule C:/Users/13647/OneDrive/Desktop/train/tests/CMakeLists.txt
  mocs_compilation_Debug.cpp
  station_api_smoke_test.cpp
  database_manager.cpp
  database_manager_station.cpp
  正在生成代码...
  station_api_smoke_test.vcxproj -> C:\Users\13647\OneDrive\Desktop\train\build\tests\Debug\station_api_smoke_test.exe
  Automatic MOC and UIC for target user_api_smoke_test
  user_api_smoke_test.cpp
  user_api_smoke_test.vcxproj -> C:\Users\13647\OneDrive\Desktop\train\build\tests\Debug\user_api_smoke_test.exe
PS C:\Users\13647\OneDrive\Desktop\train> ctest --test-dir build -C Debug -R station_api_smoke_test --output-on-failure
Test project C:/Users/13647/OneDrive/Desktop/train/build
    Start 3: station_api_smoke_test
1/1 Test #3: station_api_smoke_test ...........   Passed    0.16 sec

100% tests passed, 0 tests failed out of 1

Total Test time (real) =   0.29 sec
PS C:\Users\13647\OneDrive\Desktop\train>
*/
