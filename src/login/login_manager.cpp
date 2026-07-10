#include "login_manager.h"

#include "database_manager.h"

#include <optional>
#include <QString>

namespace {
// 数据库存的是数字，这里转成代码里用的身份。
std::optional<UserRole> roleFromDatabaseValue(int role)
{
    switch (role) {
    case 3:
        return UserRole::User;
    case 2:
        return UserRole::Admin;
    case 1:
        return UserRole::Seller;
    case 0:
        return UserRole::Guest;
    default:
        return std::nullopt;    // 角色数字不对，就让登录失败。
    }
}

bool databaseReady(const DatabaseManager *databaseManager)
{
    return databaseManager != nullptr && databaseManager->isOpen();
}

QString defaultSellerPassword()
{
    return QStringLiteral("123456");
}
}

LoginManager::LoginManager(DatabaseManager *databaseManager)
    : m_databaseManager(databaseManager)
{
}

LoginResult LoginManager::authenticate(const QString &username, const QString &password) const
{
    // 登录按固定顺序处理：先检查输入，再确认数据库可用，然后查账号、核对密码，
    // 最后检查身份和启用状态。中间任何一步不通过就马上返回失败结果，
    // 界面只负责把这个结果显示出来。
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

    // 查账号只能走 DatabaseManager，LoginManager 拿到记录后再做密码和身份判断。
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

AccountResult LoginManager::registerUser(const QString &username,
                                         const QString &password) const
{
    if (!databaseReady(m_databaseManager)) {
        return {false, QStringLiteral("注册服务不可用。")};
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

    // 普通注册固定写入 User 身份。管理员和售票员不能从公开注册入口创建，
    // 避免有人通过注册界面给自己更高权限。
    UserRecord user;
    user.username = trimmedUsername;
    user.password = password;
    user.role = static_cast<int>(UserRole::User);
    user.enabled = true;

    if (!m_databaseManager->addUser(user)) {
        return {false, QStringLiteral("用户注册失败。")};
    }

    return {true, QStringLiteral("用户注册成功，请使用新账号登录。")};
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

    // 管理员新增售票员走这里。
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

    // 先读出完整账号再确认身份，只允许改售票员，别误改管理员或普通用户。
    if (user->role != static_cast<int>(UserRole::Seller)) {
        return {false, QStringLiteral("只能重置售票员账号的密码。")};
    }

    user->password = newPassword;

    if (!m_databaseManager->updateUser(*user)) {
        return {false, QStringLiteral("重置密码失败。")};
    }

    return {true, QStringLiteral("售票员密码已重置。")};
}

AccountResult LoginManager::resetSellerPasswordToDefault(UserRole currentRole,
                                                         const QString &username) const
{
    const AccountResult result =
        resetSellerPassword(currentRole, username, defaultSellerPassword());

    if (!result.success) {
        return result;
    }

    return {true, QStringLiteral("已重置为默认密码：123456")};
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

    // 身份对得上，才允许改自己的密码。
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

QList<SellerAccountInfo> LoginManager::sellerAccounts(UserRole currentRole) const
{
    QList<SellerAccountInfo> sellers;

    if (!databaseReady(m_databaseManager) || currentRole != UserRole::Admin) {
        return sellers;
    }

    // DatabaseManager 返回完整的 UserRecord，但列表页面只需要用户名和启用状态。
    // 这里换成较小的 SellerAccountInfo，密码不会被带到界面层。
    const QList<UserRecord> users =
        m_databaseManager->findUsersByRole(static_cast<int>(UserRole::Seller));

    for (const UserRecord &user : users) {
        SellerAccountInfo info;
        info.username = user.username;
        info.enabled = user.enabled;
        sellers.append(info);
    }

    return sellers;
}

bool LoginManager::canAccessGuestFunctions(UserRole role)
{
    return role == UserRole::Guest || role == UserRole::User
           || role == UserRole::Seller || role == UserRole::Admin;
}

bool LoginManager::canAccessSellerFunctions(UserRole role)
{
    return role == UserRole::Seller || role == UserRole::Admin;
}

bool LoginManager::canAccessAdminFunctions(UserRole role)
{
    return role == UserRole::Admin;
}
