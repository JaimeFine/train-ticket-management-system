#include "login_manager.h"

#include "database_manager.h"

#include <QString>

namespace {
// 数据库中 role 用整数保存，这里集中转换为登录模块使用的枚举。
std::optional<UserRole> roleFromDatabaseValue(int role)
{
    switch (role) {
    case 2:
        return UserRole::Admin;
    case 1:
        return UserRole::Seller;
    case 0:
        return UserRole::Guest;
    default:
        return std::nullopt;    // >>> MANAGER FIX
    }
}
}

LoginManager::LoginManager(const DatabaseManager *databaseManager)
    : m_databaseManager(databaseManager)
{
}

LoginResult LoginManager::authenticate(const QString &username, const QString &password) const
{
    // 先做界面无关的基础输入校验，避免空值继续进入后续认证流程。
    const QString trimmedUsername = username.trimmed();

    if (trimmedUsername.isEmpty()) {
        return {false, UserRole::Guest, QString(), QStringLiteral("请输入用户名。")};
    }

    if (password.isEmpty()) {
        return {false, UserRole::Guest, trimmedUsername, QStringLiteral("请输入密码。")};
    }

    if (m_databaseManager == nullptr) {
        return {false,
                UserRole::Guest,
                trimmedUsername,
                QStringLiteral("登录服务尚未连接。")};
    }

    if (!m_databaseManager->isOpen()) {
        return {false,
                UserRole::Guest,
                trimmedUsername,
                QStringLiteral("登录服务不可用，请检查数据库。")};
    }

    // 用户查询只通过 DatabaseManager，密码和角色判断留在 LoginManager。
    const auto userAccount = m_databaseManager->findUserByUsername(trimmedUsername);
    // >>> MANAGER ADDED THIS:
    const auto role = roleFromDatabaseValue(userAccount->role);

    if (!role.has_value()) {
        return {false,
                UserRole::Guest,
                userAccount->username,
                QStringLiteral("账号角色无效。")};
    }

    if (!userAccount.has_value() || userAccount->password != password) {
        return {false,
                UserRole::Guest,
                trimmedUsername,
                QStringLiteral("用户名或密码无效。")};
    }

    if (!userAccount->enabled) {
        return {false,
                *role,
                userAccount->username,
                QStringLiteral("账号已被禁用。")};
    }

    return {true,
            *role,
            userAccount->username,
            QStringLiteral("登录成功。")};
}

LoginResult LoginManager::loginAsGuest() const
{
    return {true,
            UserRole::Guest,
            QStringLiteral("游客"),
            QStringLiteral("游客访问已开启。")};
}

bool LoginManager::canAccessGuestFunctions(UserRole role)
{
    return role == UserRole::Guest || role == UserRole::Seller || role == UserRole::Admin;
}

bool LoginManager::canAccessSellerFunctions(UserRole role)
{
    return role == UserRole::Seller || role == UserRole::Admin;
}

bool LoginManager::canAccessAdminFunctions(UserRole role)
{
    return role == UserRole::Admin;
}
