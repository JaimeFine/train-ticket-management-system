#pragma once

#include <QList>
#include <QString>

class DatabaseManager;

// 这里的数字和 User.role 一一对应，不能自己改顺序。
enum class UserRole
{
    Guest = 0,
    Seller = 1,
    Admin = 2,
    User = 3
};

// 登录结果统一从这里带回界面，界面不需要再去查数据库。
struct LoginResult
{
    bool success = false;
    UserRole role = UserRole::Guest;
    QString username;
    QString message;
};

struct AccountResult
{
    bool success = false;
    QString message;
};

struct SellerAccountInfo
{
    QString username;
    bool enabled = true;
};

class LoginManager
{
public:
    explicit LoginManager(DatabaseManager *databaseManager = nullptr);

    LoginResult authenticate(const QString &username, const QString &password) const;
    LoginResult loginAsGuest() const;

    AccountResult registerUser(const QString &username,
                               const QString &password) const;
    AccountResult createSellerAccount(UserRole currentRole,
                                      const QString &username,
                                      const QString &password) const;
    AccountResult resetSellerPassword(UserRole currentRole,
                                      const QString &username,
                                      const QString &newPassword) const;
    AccountResult resetSellerPasswordToDefault(UserRole currentRole,
                                               const QString &username) const;
    AccountResult setSellerEnabled(UserRole currentRole,
                                   const QString &username,
                                   bool enabled) const;
    AccountResult changeOwnPassword(const QString &username,
                                    UserRole currentRole,
                                    const QString &oldPassword,
                                    const QString &newPassword) const;
    QList<SellerAccountInfo> sellerAccounts(UserRole currentRole) const;

    // 其他模块接入主窗口后，可以先用这三个函数判断当前身份能不能进入。
    static bool canAccessGuestFunctions(UserRole role);
    static bool canAccessSellerFunctions(UserRole role);
    static bool canAccessAdminFunctions(UserRole role);

private:
    DatabaseManager *m_databaseManager = nullptr;
};
