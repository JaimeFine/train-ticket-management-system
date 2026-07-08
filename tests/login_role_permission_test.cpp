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
    const QString sellerUsername = makeUsername(QStringLiteral("role_seller"));

    ok = checkResult(addLoginUser(manager, adminUsername, QStringLiteral("admin_pass"), 2),
                     "Admin user should be created.") && ok;
    ok = checkResult(addLoginUser(manager, sellerUsername, QStringLiteral("seller_pass"), 1),
                     "Seller user should be created.") && ok;

    const LoginResult adminLogin =
        loginManager.authenticate(adminUsername, QStringLiteral("admin_pass"));
    const LoginResult sellerLogin =
        loginManager.authenticate(sellerUsername, QStringLiteral("seller_pass"));
    const LoginResult guestLogin = loginManager.loginAsGuest();
    const LoginResult wrongPassword =
        loginManager.authenticate(sellerUsername, QStringLiteral("bad_pass"));

    // 管理员和售票员都走正式数据库用户接口，再由 LoginManager 判断身份。
    ok = checkResult(adminLogin.success && adminLogin.role == UserRole::Admin,
                     "Admin login should return admin role.") && ok;
    ok = checkResult(sellerLogin.success && sellerLogin.role == UserRole::Seller,
                     "Seller login should return seller role.") && ok;

    // 游客不需要数据库账号，直接取得 role=0 的访问结果。
    ok = checkResult(guestLogin.success && guestLogin.role == UserRole::Guest,
                     "Guest login should return guest role.") && ok;
    ok = checkResult(!wrongPassword.success,
                     "Wrong password should not login.") && ok;

    if (!ok) {
        return 1;
    }

    qDebug() << "Login role permission test passed.";
    return 0;
}
