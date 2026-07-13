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

bool addLoginUser(DatabaseManager &manager,
                  const QString &username,
                  const QString &password,
                  int role)
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

    const QString adminUsername = makeUsername(QStringLiteral("account_admin"));
    const QString userUsername = makeUsername(QStringLiteral("account_user"));
    const QString sellerUsername = makeUsername(QStringLiteral("account_seller"));
    const QString newSellerUsername = makeUsername(QStringLiteral("account_new_seller"));

    ok = checkResult(addLoginUser(manager, adminUsername, QStringLiteral("admin_old"), 2),
                     "Admin user should be created.") && ok;
    ok = checkResult(addLoginUser(manager, sellerUsername, QStringLiteral("seller_old"), 1),
                     "Seller user should be created.") && ok;

    // 第一组：验证普通用户可以公开注册、取得数据库用户 ID，重复用户名会被拒绝。
    const AccountResult registerResult =
        loginManager.registerUser(userUsername, QStringLiteral("user_old"));
    ok = checkResult(registerResult.success,
                     "Normal user should register.") && ok;
    const LoginResult userLogin =
        loginManager.authenticate(userUsername, QStringLiteral("user_old"));
    ok = checkResult(userLogin.success
                         && userLogin.role == UserRole::User
                         && userLogin.userId > 0,
                     "Registered normal user should login with user ID.") && ok;

    const LoginResult adminLogin =
        loginManager.authenticate(adminUsername, QStringLiteral("admin_old"));
    const LoginResult sellerLogin =
        loginManager.authenticate(sellerUsername, QStringLiteral("seller_old"));
    ok = checkResult(adminLogin.success && adminLogin.userId > 0,
                     "Admin login should provide user ID.") && ok;
    ok = checkResult(sellerLogin.success && sellerLogin.userId > 0,
                     "Seller login should provide user ID.") && ok;

    const AccountResult duplicateRegister =
        loginManager.registerUser(userUsername, QStringLiteral("user_other"));
    ok = checkResult(!duplicateRegister.success,
                     "Duplicate normal username should be rejected.") && ok;

    // 第二组：验证只有管理员能创建售票员，并能在售票员列表里看到新账号。
    // 列表接口还要区分“没有数据”和“读取失败”，所以同时测试权限和数据库不可用情况。
    const AccountResult createResult =
        loginManager.createSellerAccount(adminLogin.userId,
                                         newSellerUsername,
                                         QStringLiteral("new_seller_pass"));
    ok = checkResult(createResult.success,
                     "Admin should create seller account.") && ok;

    const LoginResult newSellerLogin =
        loginManager.authenticate(newSellerUsername, QStringLiteral("new_seller_pass"));
    ok = checkResult(newSellerLogin.success && newSellerLogin.role == UserRole::Seller,
                     "Created seller should login as seller.") && ok;

    const SellerAccountListResult sellerList =
        loginManager.sellerAccounts(adminLogin.userId);
    bool foundCreatedSeller = false;
    for (const SellerAccountInfo &seller : sellerList.accounts) {
        if (seller.username == newSellerUsername && seller.enabled) {
            foundCreatedSeller = true;
        }
    }
    ok = checkResult(sellerList.success && foundCreatedSeller,
                     "Admin should see created seller account.") && ok;

    const SellerAccountListResult sellerView =
        loginManager.sellerAccounts(sellerLogin.userId);
    ok = checkResult(!sellerView.success,
                     "Seller should not read seller account list.") && ok;

    const SellerAccountListResult missingOperatorView =
        loginManager.sellerAccounts(0);
    ok = checkResult(!missingOperatorView.success,
                     "Missing operator ID should not read seller account list.") && ok;

    LoginManager unavailableLoginManager;
    const SellerAccountListResult unavailableList =
        unavailableLoginManager.sellerAccounts(adminLogin.userId);
    ok = checkResult(!unavailableList.success,
                     "Unavailable database should return list failure.") && ok;

    const AccountResult duplicateCreate =
        loginManager.createSellerAccount(adminLogin.userId,
                                         newSellerUsername,
                                         QStringLiteral("other_pass"));
    ok = checkResult(!duplicateCreate.success,
                     "Duplicate seller username should be rejected.") && ok;

    const AccountResult sellerCreate =
        loginManager.createSellerAccount(sellerLogin.userId,
                                         makeUsername(QStringLiteral("bad_create")),
                                         QStringLiteral("bad_pass"));
    ok = checkResult(!sellerCreate.success,
                     "Seller should not create seller account.") && ok;

    // 第三组：重置密码后旧密码必须立即失效，新密码才能登录。
    // 默认密码入口复用同一套重置规则，同时检查售票员不能越权操作其他账号。
    const AccountResult resetResult =
        loginManager.resetSellerPassword(adminLogin.userId,
                                         newSellerUsername,
                                         QStringLiteral("reset_pass"));
    ok = checkResult(resetResult.success,
                     "Admin should reset seller password.") && ok;
    ok = checkResult(!loginManager.authenticate(newSellerUsername,
                                                QStringLiteral("new_seller_pass")).success,
                     "Old seller password should fail after reset.") && ok;
    ok = checkResult(loginManager.authenticate(newSellerUsername,
                                               QStringLiteral("reset_pass")).success,
                     "Reset seller password should login.") && ok;

    const AccountResult defaultResetResult =
        loginManager.resetSellerPasswordToDefault(adminLogin.userId, newSellerUsername);
    ok = checkResult(defaultResetResult.success,
                     "Admin should reset seller password to default.") && ok;
    ok = checkResult(loginManager.authenticate(newSellerUsername,
                                               QStringLiteral("123456")).success,
                     "Default seller password should login.") && ok;

    const AccountResult sellerReset =
        loginManager.resetSellerPassword(sellerLogin.userId,
                                         newSellerUsername,
                                         QStringLiteral("seller_try"));
    ok = checkResult(!sellerReset.success,
                     "Seller should not reset seller password.") && ok;

    // 第四组：禁用账号后即使密码正确也不能登录，重新启用后才能恢复。
    const AccountResult disableResult =
        loginManager.setSellerEnabled(adminLogin.userId, newSellerUsername, false);
    ok = checkResult(disableResult.success,
                     "Admin should disable seller.") && ok;
    ok = checkResult(!loginManager.authenticate(newSellerUsername,
                                                QStringLiteral("123456")).success,
                     "Disabled seller should not login.") && ok;

    const AccountResult enableResult =
        loginManager.setSellerEnabled(adminLogin.userId, newSellerUsername, true);
    ok = checkResult(enableResult.success,
                     "Admin should enable seller.") && ok;
    ok = checkResult(loginManager.authenticate(newSellerUsername,
                                               QStringLiteral("123456")).success,
                     "Enabled seller should login again.") && ok;

    ok = checkResult(manager.setUserEnabled(adminLogin.userId, false),
                     "Admin should be disabled for verification.") && ok;
    const SellerAccountListResult disabledAdminView =
        loginManager.sellerAccounts(adminLogin.userId);
    ok = checkResult(!disabledAdminView.success,
                     "Disabled admin should not manage seller accounts.") && ok;
    ok = checkResult(manager.setUserEnabled(adminLogin.userId, true),
                     "Admin should be re-enabled after verification.") && ok;

    // 第五组：修改自己的密码必须提供正确原密码，且新旧密码不能相同。
    // 售票员、普通用户和管理员都使用同一个接口，游客因为没有账号必须失败。
    const AccountResult wrongOldPassword =
        loginManager.changeOwnPassword(sellerUsername,
                                       UserRole::Seller,
                                       QStringLiteral("wrong_old"),
                                       QStringLiteral("seller_new"));
    ok = checkResult(!wrongOldPassword.success,
                     "Wrong old password should be rejected.") && ok;

    const AccountResult unchangedPassword =
        loginManager.changeOwnPassword(sellerUsername,
                                       UserRole::Seller,
                                       QStringLiteral("seller_old"),
                                       QStringLiteral("seller_old"));
    ok = checkResult(!unchangedPassword.success,
                     "New password should differ from old password.") && ok;

    const AccountResult sellerChange =
        loginManager.changeOwnPassword(sellerUsername,
                                       UserRole::Seller,
                                       QStringLiteral("seller_old"),
                                       QStringLiteral("seller_new"));
    ok = checkResult(sellerChange.success,
                     "Seller should change own password.") && ok;
    ok = checkResult(loginManager.authenticate(sellerUsername,
                                               QStringLiteral("seller_new")).success,
                     "Seller new password should login.") && ok;

    const AccountResult userChange =
        loginManager.changeOwnPassword(userUsername,
                                       UserRole::User,
                                       QStringLiteral("user_old"),
                                       QStringLiteral("user_new"));
    ok = checkResult(userChange.success,
                     "Normal user should change own password.") && ok;
    ok = checkResult(loginManager.authenticate(userUsername,
                                               QStringLiteral("user_new")).success,
                     "Normal user new password should login.") && ok;

    const AccountResult adminChange =
        loginManager.changeOwnPassword(adminUsername,
                                       UserRole::Admin,
                                       QStringLiteral("admin_old"),
                                       QStringLiteral("admin_new"));
    ok = checkResult(adminChange.success,
                     "Admin should change own password.") && ok;
    ok = checkResult(loginManager.authenticate(adminUsername,
                                               QStringLiteral("admin_new")).success,
                     "Admin new password should login.") && ok;

    const AccountResult guestChange =
        loginManager.changeOwnPassword(QStringLiteral("游客"),
                                       UserRole::Guest,
                                       QStringLiteral("old"),
                                       QStringLiteral("new"));
    ok = checkResult(!guestChange.success,
                     "Guest should not change password.") && ok;

    if (!ok) {
        return 1;
    }

    qDebug() << "Login account management test passed.";
    return 0;
}
