#include "database_manager.h"

#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QString>

bool DatabaseManager::createOrder(const OrderRecord &order) {
    QSqlQuery query(QSqlDatabase::database(m_connectionName));

    query.prepare(QStringLiteral(
        "INSERT INTO \"Order\" ("
        "userId, trainId, passengerName, purchaseTime, status"
        ") "
        "VALUES (:userId, :trainId, :passengerName, :purchaseTime, :status)"
    ));

    query.bindValue(":userId", order.userId);
    query.bindValue(":trainId", order.trainId);
    query.bindValue(":passengerName", order.passengerName);
    query.bindValue(":purchaseTime", order.purchaseTime);
    query.bindValue(":status", order.status);

    if (!query.exec()) {
        m_lastError = query.lastError().text();
        return false;
    }

    return true;
}

QList<OrderRecord> DatabaseManager::findOrdersByUser(int userId) const {
    QSqlQuery query(QSqlDatabase::database(m_connectionName));

    query.prepare(QStringLiteral(
        "SELECT orderId, userId, trainId, passengerName, purchaseTime, status "
        "FROM \"Order\" "
        "WHERE userId = :userId"
    ));

    query.bindValue(":userId", userId);

    QList<OrderRecord> records;

    if (!query.exec()) {
        m_lastError = query.lastError().text();
        return records;
    }

    while (query.next()) {
        OrderRecord record;
        record.orderId = query.value(0).toInt();
        record.userId = query.value(1).toInt();
        record.trainId = query.value(2).toInt();
        record.passengerName = query.value(3).toString();
        record.purchaseTime = query.value(4).toString();
        record.status = query.value(5).toInt();

        records.append(record);
    }

    return records;
}

bool DatabaseManager::updateOrderStatus(int orderId, int status) {
    QSqlQuery query(QSqlDatabase::database(m_connectionName));

    query.prepare(QStringLiteral(
        "UPDATE \"Order\" "
        "SET status = :status "
        "WHERE orderId = :orderId"
    ));

    query.bindValue(":orderId", orderId);
    query.bindValue(":status", status);

    if (!query.exec()) {
        m_lastError = query.lastError().text();
        return false;
    }

    // Check if any row was actually updated
    if (query.numRowsAffected() == 0) {
        return false;
    }

    return true;
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
