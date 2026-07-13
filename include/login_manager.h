#pragma once

#include <QList>
#include <QString>

class DatabaseManager;

// 与开发规范中的 User.role 保持一致：0=游客，1=售票员，2=管理员，3=普通用户。
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
    int userId = 0;    // 游客和登录失败时没有数据库用户 ID。
    QString username;
    QString message;
};

struct AccountResult
{
    bool success = false;
    QString message;
};

// 售票员列表只需要显示用户名和账号状态，不向界面提供密码。
struct SellerAccountInfo
{
    QString username;
    bool enabled = true;
};

struct SellerAccountListResult
{
    // 列表为空不一定是报错，所以把状态和数据一起返回。
    bool success = false;
    QList<SellerAccountInfo> accounts;
    QString message;
};

class LoginManager
{
public:
    explicit LoginManager(DatabaseManager *databaseManager = nullptr);

    // 验证数据库账号；游客访问不需要数据库账号。
    LoginResult authenticate(const QString &username, const QString &password) const;
    LoginResult loginAsGuest() const;

    // 注册普通用户。
    AccountResult registerUser(const QString &username,
                               const QString &password) const;

    // 以下接口供管理员创建和维护售票员账号。
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

    // 已登录的管理员、售票员和普通用户可以修改自己的密码。
    AccountResult changeOwnPassword(const QString &username,
                                    UserRole currentRole,
                                    const QString &oldPassword,
                                    const QString &newPassword) const;

    // 管理员读取现有售票员账号和启用状态。
    SellerAccountListResult sellerAccounts(UserRole currentRole) const;

    // 基础查询允许四种身份访问；售票员功能允许售票员和管理员访问；
    // 管理员功能只允许管理员访问。
    static bool canAccessGuestFunctions(UserRole role);
    static bool canAccessSellerFunctions(UserRole role);
    static bool canAccessAdminFunctions(UserRole role);

private:
    DatabaseManager *m_databaseManager = nullptr;
};
