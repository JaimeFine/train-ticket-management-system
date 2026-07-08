#pragma once

#include <QString>

class DatabaseManager;

// 与开发规范中的 User.role 保持一致：0=游客，1=售票员，2=管理员。
enum class UserRole
{
    Guest = 0,
    Seller = 1,
    Admin = 2
};

// UI 只依赖这个结果对象展示状态，不直接访问数据库或执行认证细节。
struct LoginResult
{
    bool success = false;
    UserRole role = UserRole::Guest;
    QString username;
    QString message;
};

class LoginManager
{
public:
    explicit LoginManager(const DatabaseManager *databaseManager = nullptr);

    LoginResult authenticate(const QString &username, const QString &password) const;
    LoginResult loginAsGuest() const;

private:
    const DatabaseManager *m_databaseManager = nullptr;
};
