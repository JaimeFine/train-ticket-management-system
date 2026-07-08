#include "database_manager.h"

// This is a tiny smoke test program, not a full automated unit test.
//
// Its job is simple:
// - create a DatabaseManager
// - call initialize()
// - print whether initialization worked
//
// Why this matters:
// Even before booking or login exists, we want one quick way to verify that the
// database file can be opened and the schema can be created successfully.

#include <QCoreApplication>
#include <QDebug>

int main(int argc, char *argv[]) {
    QCoreApplication app(argc, argv);

    DatabaseManager manager;

    if (!manager.initialize()) {
        qCritical() << "Database initialization failed.";
        qCritical() << "Path:" << manager.databasePath();
        qCritical() << "Error:" << manager.lastError();
        return 1;
    }

    qDebug() << "Database initialization succeeded.";
    qDebug() << "Path:" << manager.databasePath();
    qDebug() << "Open:" << manager.isOpen();
    return 0;
}

/*
$ Test Log:
PS C:\Users\13647\OneDrive\Desktop\train> cmake --build build --config Debug
CMake is re-running because C:/Users/13647/OneDrive/Desktop/train/build/CMakeFiles/generate.stamp is out-of-date.
  the file 'C:/Users/13647/OneDrive/Desktop/train/CMakeLists.txt'
  is newer than 'C:/Users/13647/OneDrive/Desktop/train/build/CMakeFiles/generate.stamp.depend'
  result='-1'
-- Selecting Windows SDK version 10.0.26100.0 to target Windows 10.0.26200.
-- Could NOT find WrapVulkanHeaders (missing: Vulkan_INCLUDE_DIR)
-- Using Qt 6.9
-- Configuring done (0.2s)
-- Generating done (1.5s)
-- Build files have been written to: C:/Users/13647/OneDrive/Desktop/train/build
适用于 .NET Framework MSBuild 版本 18.7.8+1ac568fee

  Automatic MOC and UIC for target TrainTicketSystem
  database_manager.cpp
  TrainTicketSystem.vcxproj -> C:\Users\13647\OneDrive\Desktop\train\build\Debug\TrainTicketSystem.exe
  Automatic MOC and UIC for target database_init_smoke_test
  database_manager.cpp
  database_init_smoke_test.vcxproj -> C:\Users\13647\OneDrive\Desktop\train\build\tests\Debug\database_init_smoke_test.
  exe
  Building Custom Rule C:/Users/13647/OneDrive/Desktop/train/CMakeLists.txt
PS C:\Users\13647\OneDrive\Desktop\train> ctest --test-dir build -C Debug --output-on-failure
Test project C:/Users/13647/OneDrive/Desktop/train/build
    Start 1: database_init_smoke_test
1/1 Test #1: database_init_smoke_test .........   Passed    1.07 sec

100% tests passed, 0 tests failed out of 1

Total Test time (real) =   1.19 sec
*/
