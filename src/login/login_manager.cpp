#include "login_manager.h"

#include "database_manager.h"

#include <optional>
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
        return std::nullopt;    // 数据库里出现非法角色时，不能静默当成游客。
    }
}

bool databaseReady(const DatabaseManager *databaseManager)
{
    return databaseManager != nullptr && databaseManager->isOpen();
}
}

LoginManager::LoginManager(DatabaseManager *databaseManager)
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

    if (!userAccount.has_value() || userAccount->password != password) {
        return {false,
                UserRole::Guest,
                trimmedUsername,
                QStringLiteral("用户名或密码无效。")};
    }

    const auto role = roleFromDatabaseValue(userAccount->role);

    if (!role.has_value()) {
        return {false,
                UserRole::Guest,
                userAccount->username,
                QStringLiteral("账号角色无效。")};
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

AccountResult LoginManager::createSellerAccount(UserRole currentRole,
                                                const QString &username,
                                                const QString &password) const
{
    if (!databaseReady(m_databaseManager)) {
        return {false, QStringLiteral("账号管理服务不可用。")};
    }

    if (currentRole != UserRole::Admin) {
        return {false, QStringLiteral("只有管理员可以创建售票员账号。")};
    }

    const QString trimmedUsername = username.trimmed();

    if (trimmedUsername.isEmpty()) {
        return {false, QStringLiteral("请输入用户名。")};
    }

    if (password.isEmpty()) {
        return {false, QStringLiteral("请输入密码。")};
    }

    if (m_databaseManager->findUserByUsername(trimmedUsername).has_value()) {
        return {false, QStringLiteral("用户名已存在。")};
    }

    // issue3 只允许管理员创建售票员账号，不开放普通注册。
    UserRecord user;
    user.username = trimmedUsername;
    user.password = password;
    user.role = static_cast<int>(UserRole::Seller);
    user.enabled = true;

    if (!m_databaseManager->addUser(user)) {
        return {false, QStringLiteral("创建售票员账号失败。")};
    }

    return {true, QStringLiteral("售票员账号创建成功。")};
}

AccountResult LoginManager::resetSellerPassword(UserRole currentRole,
                                                const QString &username,
                                                const QString &newPassword) const
{
    if (!databaseReady(m_databaseManager)) {
        return {false, QStringLiteral("账号管理服务不可用。")};
    }

    if (currentRole != UserRole::Admin) {
        return {false, QStringLiteral("只有管理员可以重置售票员密码。")};
    }

    const QString trimmedUsername = username.trimmed();

    if (trimmedUsername.isEmpty()) {
        return {false, QStringLiteral("请输入售票员用户名。")};
    }

    if (newPassword.isEmpty()) {
        return {false, QStringLiteral("请输入新密码。")};
    }

    auto user = m_databaseManager->findUserByUsername(trimmedUsername);

    if (!user.has_value()) {
        return {false, QStringLiteral("未找到该售票员账号。")};
    }

    // 管理员只能处理售票员账号，避免误改管理员或游客数据。
    if (user->role != static_cast<int>(UserRole::Seller)) {
        return {false, QStringLiteral("只能重置售票员账号的密码。")};
    }

    user->password = newPassword;

    if (!m_databaseManager->updateUser(*user)) {
        return {false, QStringLiteral("重置密码失败。")};
    }

    return {true, QStringLiteral("售票员密码已重置。")};
}

AccountResult LoginManager::setSellerEnabled(UserRole currentRole,
                                             const QString &username,
                                             bool enabled) const
{
    if (!databaseReady(m_databaseManager)) {
        return {false, QStringLiteral("账号管理服务不可用。")};
    }

    if (currentRole != UserRole::Admin) {
        return {false, QStringLiteral("只有管理员可以启用或禁用售票员账号。")};
    }

    const QString trimmedUsername = username.trimmed();

    if (trimmedUsername.isEmpty()) {
        return {false, QStringLiteral("请输入售票员用户名。")};
    }

    const auto user = m_databaseManager->findUserByUsername(trimmedUsername);

    if (!user.has_value()) {
        return {false, QStringLiteral("未找到该售票员账号。")};
    }

    if (user->role != static_cast<int>(UserRole::Seller)) {
        return {false, QStringLiteral("只能修改售票员账号状态。")};
    }

    if (!m_databaseManager->setUserEnabled(user->userId, enabled)) {
        return {false, QStringLiteral("账号状态修改失败。")};
    }

    if (enabled) {
        return {true, QStringLiteral("售票员账号已启用。")};
    }

    return {true, QStringLiteral("售票员账号已禁用。")};
}

AccountResult LoginManager::changeOwnPassword(const QString &username,
                                              UserRole currentRole,
                                              const QString &oldPassword,
                                              const QString &newPassword) const
{
    if (!databaseReady(m_databaseManager)) {
        return {false, QStringLiteral("账号管理服务不可用。")};
    }

    if (currentRole == UserRole::Guest) {
        return {false, QStringLiteral("游客没有数据库账号，不能修改密码。")};
    }

    const QString trimmedUsername = username.trimmed();

    if (trimmedUsername.isEmpty()) {
        return {false, QStringLiteral("当前用户名为空。")};
    }

    if (oldPassword.isEmpty()) {
        return {false, QStringLiteral("请输入原密码。")};
    }

    if (newPassword.isEmpty()) {
        return {false, QStringLiteral("请输入新密码。")};
    }

    auto user = m_databaseManager->findUserByUsername(trimmedUsername);

    if (!user.has_value()) {
        return {false, QStringLiteral("未找到当前账号。")};
    }

    // 当前登录身份要和数据库中的账号角色一致，防止拿错账号改密码。
    if (user->role != static_cast<int>(currentRole)) {
        return {false, QStringLiteral("当前身份与账号信息不一致。")};
    }

    if (user->password != oldPassword) {
        return {false, QStringLiteral("原密码不正确。")};
    }

    user->password = newPassword;

    if (!m_databaseManager->updateUser(*user)) {
        return {false, QStringLiteral("修改密码失败。")};
    }

    return {true, QStringLiteral("密码修改成功。")};
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
