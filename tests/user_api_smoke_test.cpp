#include "database_manager.h"

// This is a learning-oriented smoke test for the User API.
//
// What it checks:
// 1. Database initialization still works.
// 2. We can insert a user.
// 3. We can find the user by username.
// 4. We can find the same user by id.
// 5. We can update the user.
// 6. We can disable the user with setUserEnabled().
//
// It is not a full unit test suite. It is a practical end-to-end check that
// the new User CRUD methods are wired correctly.

#include <QCoreApplication>
#include <QDateTime>
#include <QDebug>

namespace {
    QString makeUniqueUsername() {
        // We generate a unique username so the smoke test can be run multiple times
        // without failing on the UNIQUE constraint of the username column.
        return QStringLiteral("smoke_user_%1")
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

    UserRecord newUser;
    newUser.username = makeUniqueUsername();
    newUser.password = QStringLiteral("initial_password");
    newUser.role = 1;
    newUser.enabled = true;

    if (!manager.addUser(newUser)) {
        qCritical() << "addUser() failed:" << manager.lastError();
        return 1;
    }

    const std::optional<UserRecord> byUsername =
        manager.findUserByUsername(newUser.username);

    if (!byUsername.has_value()) {
        qCritical() << "findUserByUsername() returned no result.";
        return 1;
    }

    if (byUsername->username != newUser.username) {
        qCritical() << "Username mismatch after insert.";
        return 1;
    }

    const std::optional<UserRecord> byId =
        manager.findUserById(byUsername->userId);

    if (!byId.has_value()) {
        qCritical() << "findUserById() returned no result.";
        return 1;
    }

    if (byId->userId != byUsername->userId) {
        qCritical() << "User id mismatch between lookup methods.";
        return 1;
    }

    UserRecord updatedUser = *byId;
    updatedUser.password = QStringLiteral("updated_password");
    updatedUser.role = 2;
    updatedUser.enabled = true;

    if (!manager.updateUser(updatedUser)) {
        qCritical() << "updateUser() failed:" << manager.lastError();
        return 1;
    }

    const std::optional<UserRecord> afterUpdate =
        manager.findUserById(updatedUser.userId);

    if (!afterUpdate.has_value()) {
        qCritical() << "User disappeared after update.";
        return 1;
    }

    if (afterUpdate->password != updatedUser.password ||
        afterUpdate->role != updatedUser.role) {
        qCritical() << "Updated fields were not saved correctly.";
        return 1;
    }

    if (!manager.setUserEnabled(updatedUser.userId, false)) {
        qCritical() << "setUserEnabled() failed:" << manager.lastError();
        return 1;
    }

    const std::optional<UserRecord> afterDisable =
        manager.findUserById(updatedUser.userId);

    if (!afterDisable.has_value()) {
        qCritical() << "User disappeared after disable call.";
        return 1;
    }

    if (afterDisable->enabled) {
        qCritical() << "User should be disabled, but enabled is still true.";
        return 1;
    }

    qDebug() << "User API smoke test passed.";
    qDebug() << "Created user id:" << afterDisable->userId;
    qDebug() << "Created username:" << afterDisable->username;
    return 0;
}

/*
$ Test Log:
PS C:\Users\13647\OneDrive\Desktop\train> cmake -S . -B build -DBUILD_TESTS=ON
-- Selecting Windows SDK version 10.0.26100.0 to target Windows 10.0.26200.
-- Could NOT find WrapVulkanHeaders (missing: Vulkan_INCLUDE_DIR)
-- Using Qt 6.9
-- Configuring done (6.6s)
-- Generating done (2.9s)
-- Build files have been written to: C:/Users/13647/OneDrive/Desktop/train/build
PS C:\Users\13647\OneDrive\Desktop\train> cmake --build build --config Debug
适用于 .NET Framework MSBuild 版本 18.7.8+1ac568fee

  Automatic MOC and UIC for target TrainTicketSystem
  database_manager_user.cpp
  TrainTicketSystem.vcxproj -> C:\Users\13647\OneDrive\Desktop\train\build\Debug\TrainTicketSystem.exe
  Automatic MOC and UIC for target database_init_smoke_test
  database_init_smoke_test.vcxproj -> C:\Users\13647\OneDrive\Desktop\train\build\tests\Debug\database_init_smoke_test.
  exe
  Automatic MOC and UIC for target user_api_smoke_test
  database_manager_user.cpp
  user_api_smoke_test.vcxproj -> C:\Users\13647\OneDrive\Desktop\train\build\tests\Debug\user_api_smoke_test.exe
PS C:\Users\13647\OneDrive\Desktop\train> ctest --test-dir build -C Debug -R user_api_smoke_test --output-on-failure
Test project C:/Users/13647/OneDrive/Desktop/train/build
    Start 2: user_api_smoke_test
1/1 Test #2: user_api_smoke_test ..............   Passed    0.23 sec

100% tests passed, 0 tests failed out of 1

Total Test time (real) =   0.39 sec
PS C:\Users\13647\OneDrive\Desktop\train>
*/
