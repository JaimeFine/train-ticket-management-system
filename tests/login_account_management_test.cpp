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

    const AccountResult registerResult =
        loginManager.registerUser(userUsername, QStringLiteral("user_old"));
    ok = checkResult(registerResult.success,
                     "Normal user should register.") && ok;
    const LoginResult userLogin =
        loginManager.authenticate(userUsername, QStringLiteral("user_old"));
    ok = checkResult(userLogin.success && userLogin.role == UserRole::User,
                     "Registered normal user should login as user.") && ok;

    const AccountResult duplicateRegister =
        loginManager.registerUser(userUsername, QStringLiteral("user_other"));
    ok = checkResult(!duplicateRegister.success,
                     "Duplicate normal username should be rejected.") && ok;

    const AccountResult createResult =
        loginManager.createSellerAccount(UserRole::Admin,
                                         newSellerUsername,
                                         QStringLiteral("new_seller_pass"));
    ok = checkResult(createResult.success,
                     "Admin should create seller account.") && ok;

    const LoginResult newSellerLogin =
        loginManager.authenticate(newSellerUsername, QStringLiteral("new_seller_pass"));
    ok = checkResult(newSellerLogin.success && newSellerLogin.role == UserRole::Seller,
                     "Created seller should login as seller.") && ok;

    const QList<SellerAccountInfo> sellerAccounts =
        loginManager.sellerAccounts(UserRole::Admin);
    bool foundCreatedSeller = false;
    for (const SellerAccountInfo &seller : sellerAccounts) {
        if (seller.username == newSellerUsername && seller.enabled) {
            foundCreatedSeller = true;
        }
    }
    ok = checkResult(foundCreatedSeller,
                     "Admin should see created seller account.") && ok;

    const AccountResult duplicateCreate =
        loginManager.createSellerAccount(UserRole::Admin,
                                         newSellerUsername,
                                         QStringLiteral("other_pass"));
    ok = checkResult(!duplicateCreate.success,
                     "Duplicate seller username should be rejected.") && ok;

    const AccountResult sellerCreate =
        loginManager.createSellerAccount(UserRole::Seller,
                                         makeUsername(QStringLiteral("bad_create")),
                                         QStringLiteral("bad_pass"));
    ok = checkResult(!sellerCreate.success,
                     "Seller should not create seller account.") && ok;

    const AccountResult resetResult =
        loginManager.resetSellerPassword(UserRole::Admin,
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
        loginManager.resetSellerPasswordToDefault(UserRole::Admin, newSellerUsername);
    ok = checkResult(defaultResetResult.success,
                     "Admin should reset seller password to default.") && ok;
    ok = checkResult(loginManager.authenticate(newSellerUsername,
                                               QStringLiteral("123456")).success,
                     "Default seller password should login.") && ok;

    const AccountResult sellerReset =
        loginManager.resetSellerPassword(UserRole::Seller,
                                         newSellerUsername,
                                         QStringLiteral("seller_try"));
    ok = checkResult(!sellerReset.success,
                     "Seller should not reset seller password.") && ok;

    const AccountResult disableResult =
        loginManager.setSellerEnabled(UserRole::Admin, newSellerUsername, false);
    ok = checkResult(disableResult.success,
                     "Admin should disable seller.") && ok;
    ok = checkResult(!loginManager.authenticate(newSellerUsername,
                                                QStringLiteral("123456")).success,
                     "Disabled seller should not login.") && ok;

    const AccountResult enableResult =
        loginManager.setSellerEnabled(UserRole::Admin, newSellerUsername, true);
    ok = checkResult(enableResult.success,
                     "Admin should enable seller.") && ok;
    ok = checkResult(loginManager.authenticate(newSellerUsername,
                                               QStringLiteral("123456")).success,
                     "Enabled seller should login again.") && ok;

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
