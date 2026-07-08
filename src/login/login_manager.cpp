#include "login_manager.h"

#include "database_manager.h"

#include <QString>

namespace {
// 数据库中 role 用整数保存，这里集中转换为登录模块使用的枚举。
UserRole roleFromDatabaseValue(int role)
{
    switch (role) {
    case 2:
        return UserRole::Admin;
    case 1:
        return UserRole::Seller;
    case 0:
    default:
        return UserRole::Guest;
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

    // 账号密码认证只通过 DatabaseManager 接口，保持 UI -> LoginManager -> DatabaseManager 的分层。
    const auto userAccount = m_databaseManager->findUserByCredentials(trimmedUsername, password);

    if (!userAccount.has_value()) {
        return {false,
                UserRole::Guest,
                trimmedUsername,
                QStringLiteral("用户名或密码无效。")};
    }

    if (!userAccount->enabled) {
        return {false,
                roleFromDatabaseValue(userAccount->role),
                userAccount->username,
                QStringLiteral("账号已被禁用。")};
    }

    return {true,
            roleFromDatabaseValue(userAccount->role),
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
