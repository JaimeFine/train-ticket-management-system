#include "database_manager.h"
#include "login_manager.h"

#include <QCoreApplication>
#include <QDateTime>
#include <QDebug>

namespace {
QString makeUsername(const QString &prefix)
{
    return QStringLiteral("%1_%2").arg(prefix).arg(QDateTime::currentMSecsSinceEpoch());
}

bool checkResult(bool result, const char *message)
{
    if (!result) {
        qCritical() << message;
    }

    return result;
}

bool addLoginUser(DatabaseManager &manager, const QString &username, const QString &password, int role)
{
    UserRecord user;
    user.username = username;
    user.password = password;
    user.role = role;
    user.enabled = true;

    return manager.addUser(user);
}
}

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);

    bool ok = true;

    DatabaseManager manager;
    if (!manager.initialize()) {
        qCritical() << "Database initialization failed:" << manager.lastError();
        return 1;
    }

    LoginManager loginManager(&manager);

    // 游客只能使用基础查询入口。
    ok = checkResult(LoginManager::canAccessGuestFunctions(UserRole::Guest),
                     "Guest should access guest functions.") && ok;
    ok = checkResult(!LoginManager::canAccessSellerFunctions(UserRole::Guest),
                     "Guest should not access seller functions.") && ok;
    ok = checkResult(!LoginManager::canAccessAdminFunctions(UserRole::Guest),
                     "Guest should not access admin functions.") && ok;

    // 普通用户可以查询，但不能进入售票员或管理员入口。
    ok = checkResult(LoginManager::canAccessGuestFunctions(UserRole::User),
                     "User should access guest functions.") && ok;
    ok = checkResult(!LoginManager::canAccessSellerFunctions(UserRole::User),
                     "User should not access seller functions.") && ok;
    ok = checkResult(!LoginManager::canAccessAdminFunctions(UserRole::User),
                     "User should not access admin functions.") && ok;

    // 售票员可以使用查询和票务入口。
    ok = checkResult(LoginManager::canAccessGuestFunctions(UserRole::Seller),
                     "Seller should access guest functions.") && ok;
    ok = checkResult(LoginManager::canAccessSellerFunctions(UserRole::Seller),
                     "Seller should access seller functions.") && ok;
    ok = checkResult(!LoginManager::canAccessAdminFunctions(UserRole::Seller),
                     "Seller should not access admin functions.") && ok;

    // 管理员保留最高权限。
    ok = checkResult(LoginManager::canAccessGuestFunctions(UserRole::Admin),
                     "Admin should access guest functions.") && ok;
    ok = checkResult(LoginManager::canAccessSellerFunctions(UserRole::Admin),
                     "Admin should access seller functions.") && ok;
    ok = checkResult(LoginManager::canAccessAdminFunctions(UserRole::Admin),
                     "Admin should access admin functions.") && ok;

    const QString adminUsername = makeUsername(QStringLiteral("role_admin"));
    const QString userUsername = makeUsername(QStringLiteral("role_user"));
    const QString sellerUsername = makeUsername(QStringLiteral("role_seller"));
    const QString invalidRoleUsername =
        makeUsername(QStringLiteral("role_invalid"));

    ok = checkResult(addLoginUser(manager, adminUsername, QStringLiteral("admin_pass"), 2),
                     "Admin user should be created.") && ok;
    ok = checkResult(addLoginUser(manager, userUsername, QStringLiteral("user_pass"), 3),
                     "Normal user should be created.") && ok;
    ok = checkResult(addLoginUser(manager, sellerUsername, QStringLiteral("seller_pass"), 1),
                     "Seller user should be created.") && ok;
    ok = checkResult(addLoginUser(manager,
                                  invalidRoleUsername,
                                  QStringLiteral("invalid_pass"),
                                  99),
                     "Invalid-role user should be created for verification.") && ok;

    const LoginResult adminLogin =
        loginManager.authenticate(adminUsername, QStringLiteral("admin_pass"));
    const LoginResult userLogin =
        loginManager.authenticate(userUsername, QStringLiteral("user_pass"));
    const LoginResult sellerLogin =
        loginManager.authenticate(sellerUsername, QStringLiteral("seller_pass"));
    const LoginResult guestLogin = loginManager.loginAsGuest();
    const LoginResult wrongPassword =
        loginManager.authenticate(sellerUsername, QStringLiteral("bad_pass"));
    const LoginResult invalidRoleLogin =
        loginManager.authenticate(invalidRoleUsername,
                                  QStringLiteral("invalid_pass"));

    // 管理员、普通用户和售票员都走正式数据库用户接口，再由 LoginManager 判断身份。
    ok = checkResult(adminLogin.success && adminLogin.role == UserRole::Admin,
                     "Admin login should return admin role.") && ok;
    ok = checkResult(userLogin.success && userLogin.role == UserRole::User,
                     "Normal user login should return user role.") && ok;
    ok = checkResult(sellerLogin.success && sellerLogin.role == UserRole::Seller,
                     "Seller login should return seller role.") && ok;

    // 游客不需要数据库账号，直接取得 role=0 的访问结果。
    ok = checkResult(guestLogin.success && guestLogin.role == UserRole::Guest,
                     "Guest login should return guest role.") && ok;
    ok = checkResult(!wrongPassword.success,
                     "Wrong password should not login.") && ok;

    // 非法 role 不应该被静默降级成游客权限，否则会掩盖数据库脏数据问题。
    ok = checkResult(!invalidRoleLogin.success,
                     "Invalid database role should be rejected.") && ok;

    if (!ok) {
        return 1;
    }

    qDebug() << "Login role permission test passed.";
    return 0;
}

/*
$ Test Log:
PS C:\Users\13647\OneDrive\Desktop\train> cmake -S . -B build -DBUILD_TESTS=ON
-- Selecting Windows SDK version 10.0.26100.0 to target Windows 10.0.26200.
-- Could NOT find WrapVulkanHeaders (missing: Vulkan_INCLUDE_DIR)
-- Using Qt 6.9
-- Configuring done (0.5s)
-- Generating done (2.2s)
-- Build files have been written to: C:/Users/13647/OneDrive/Desktop/train/build
PS C:\Users\13647\OneDrive\Desktop\train> cmake --build build --config Debug
适用于 .NET Framework MSBuild 版本 18.7.8+1ac568fee

  Automatic MOC and UIC for target TrainTicketSystem
  main.cpp
  login_manager.cpp
  main_window.cpp
  正在生成代码...
  TrainTicketSystem.vcxproj -> C:\Users\13647\OneDrive\Desktop\train\build\Debug\TrainTicketSystem.exe
  Automatic MOC and UIC for target database_init_smoke_test
  database_init_smoke_test.vcxproj -> C:\Users\13647\OneDrive\Desktop\train\build\tests\Debug\database_init_smoke_test.
  exe
  Automatic MOC and UIC for target login_role_permission_test
  login_manager.cpp
  database_manager.cpp
  database_manager_user.cpp
  正在生成代码...
  login_role_permission_test.vcxproj -> C:\Users\13647\OneDrive\Desktop\train\build\tests\Debug\login_role_permission_t
  est.exe
  Automatic MOC and UIC for target order_api_smoke_test
  order_api_smoke_test.vcxproj -> C:\Users\13647\OneDrive\Desktop\train\build\tests\Debug\order_api_smoke_test.exe
  Automatic MOC and UIC for target station_api_smoke_test
  station_api_smoke_test.vcxproj -> C:\Users\13647\OneDrive\Desktop\train\build\tests\Debug\station_api_smoke_test.exe
  Automatic MOC and UIC for target train_api_smoke_test
  train_api_smoke_test.cpp
  database_manager.cpp
  database_manager_station.cpp
  database_manager_train.cpp
  正在生成代码...
  train_api_smoke_test.vcxproj -> C:\Users\13647\OneDrive\Desktop\train\build\tests\Debug\train_api_smoke_test.exe
  Automatic MOC and UIC for target user_api_smoke_test
  database_manager.cpp
  database_manager_user.cpp
  user_api_smoke_test.cpp
  正在生成代码...
  user_api_smoke_test.vcxproj -> C:\Users\13647\OneDrive\Desktop\train\build\tests\Debug\user_api_smoke_test.exe
PS C:\Users\13647\OneDrive\Desktop\train> ctest --test-dir build -C Debug -R login_role_permission_test --output-on-failure
Test project C:/Users/13647/OneDrive/Desktop/train/build
    Start 3: login_role_permission_test
1/1 Test #3: login_role_permission_test .......   Passed    0.41 sec

100% tests passed, 0 tests failed out of 1

Total Test time (real) =   0.59 sec
PS C:\Users\13647\OneDrive\Desktop\train>
*/
