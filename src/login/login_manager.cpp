#include "login_manager.h"

#include "database_manager.h"

#include <QDateTime>
#include <optional>
#include <QString>

namespace {
// 数据库存的是数字，这里分别转成游客、售票员、管理员和普通用户四种身份。
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
    // 账号操作都会先走这项检查，避免每个函数重复写空指针和连接状态判断。
    return databaseManager != nullptr && databaseManager->isOpen();
}

QString defaultSellerPassword()
{
    // 默认密码只在这里保存一份，重置逻辑和成功提示就不会出现两个不同的值。
    return QStringLiteral("123456");
}
}

LoginManager::LoginManager(DatabaseManager *databaseManager)
    : m_databaseManager(databaseManager)
{
}

const DatabaseManager *LoginManager::databaseManager() const
{
    return m_databaseManager;
}

LoginResult LoginManager::authenticate(const QString &username, const QString &password) const
{
    // 登录按固定顺序处理：先检查输入，再确认数据库可用，然后查账号、核对密码，
    // 最后检查身份和启用状态。中间任何一步不通过就马上返回失败结果，
    // 界面只负责把这个结果显示出来。
    const QString trimmedUsername = username.trimmed();

    if (trimmedUsername.isEmpty()) {
        return {false, UserRole::Guest, 0, QString(), QStringLiteral("请输入用户名。")};
    }

    if (password.isEmpty()) {
        return {false, UserRole::Guest, 0, trimmedUsername, QStringLiteral("请输入密码。")};
    }

    if (m_databaseManager == nullptr) {
        return {false,
                UserRole::Guest,
                0,
                trimmedUsername,
                QStringLiteral("登录服务尚未连接。")};
    }

    if (!m_databaseManager->isOpen()) {
        return {false,
                UserRole::Guest,
                0,
                trimmedUsername,
                QStringLiteral("登录服务不可用，请检查数据库。")};
    }

    // 查账号只能走 DatabaseManager，LoginManager 拿到记录后再做密码和身份判断。
    const auto userAccount = m_databaseManager->findUserByUsername(trimmedUsername);

    // 账号不存在和密码错误使用同一条提示，界面不会向外暴露某个用户名是否存在。
    if (!userAccount.has_value() || userAccount->password != password) {
        return {false,
                UserRole::Guest,
                0,
                trimmedUsername,
                QStringLiteral("用户名或密码无效。")};
    }

    const auto role = roleFromDatabaseValue(userAccount->role);

    if (!role.has_value()) {
        return {false,
                UserRole::Guest,
                0,
                userAccount->username,
                QStringLiteral("账号角色无效。")};
    }

    if (!userAccount->enabled) {
        return {false,
                *role,
                0,
                userAccount->username,
                QStringLiteral("账号已被禁用。")};
    }

    m_databaseManager->addOperationLog(userAccount->username,
                                       QStringLiteral("登录"),
                                       QStringLiteral("用户成功登录系统"));

    return {true,
            *role,
            userAccount->userId,
            userAccount->username,
            QStringLiteral("登录成功。")};
}

LoginResult LoginManager::loginAsGuest() const
{
    // 游客没有数据库账号，不查询 User 表，直接返回 role=0 的临时身份。
    // 主窗口根据这个身份只开放游客能够使用的功能。
    return {true,
            UserRole::Guest,
            0,
            QStringLiteral("游客"),
            QStringLiteral("游客访问已开启。")};
}

AccountResult LoginManager::registerUser(const QString &username,
                                         const QString &password) const
{
    // 普通用户注册：检查数据库和输入，确认用户名没有被使用，
    // 然后以 role=3、enabled=true 写入一条新的 User 记录。
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

    m_databaseManager->addOperationLog(trimmedUsername,
                                       QStringLiteral("注册"),
                                       QStringLiteral("普通用户注册新账号"));

    return {true, QStringLiteral("用户注册成功，请使用新账号登录。")};
}

AccountResult LoginManager::createSellerAccount(int operatorUserId,
                                                const QString &username,
                                                const QString &password) const
{
    // 创建售票员：先确认操作者是管理员，再检查用户名，最后写入 role=1 的账号。
    // 这个函数是员工权限管理页面创建售票员时使用的业务入口。
    if (!databaseReady(m_databaseManager)) {
        return {false, QStringLiteral("账号管理服务不可用。")};
    }

    if (!isActiveAdmin(operatorUserId)) {
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

    UserRecord user;
    user.username = trimmedUsername;
    user.password = password;
    user.role = static_cast<int>(UserRole::Seller);
    user.enabled = true;

    if (!m_databaseManager->addUser(user)) {
        return {false, QStringLiteral("创建售票员账号失败。")};
    }

    m_databaseManager->addOperationLog(QStringLiteral("admin:%1").arg(operatorUserId),
                                       QStringLiteral("创建售票员"),
                                       QStringLiteral("创建售票员账号：%1").arg(trimmedUsername));

    return {true, QStringLiteral("售票员账号创建成功。")};
}

AccountResult LoginManager::resetSellerPassword(int operatorUserId,
                                                const QString &username,
                                                const QString &newPassword) const
{
    // 管理员重置售票员密码：查出目标账号并确认它确实是售票员，
    // 再保存新密码，避免管理员在这个入口误改其他类型的账号。
    if (!databaseReady(m_databaseManager)) {
        return {false, QStringLiteral("账号管理服务不可用。")};
    }

    if (!isActiveAdmin(operatorUserId)) {
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

    m_databaseManager->addOperationLog(QStringLiteral("admin:%1").arg(operatorUserId),
                                       QStringLiteral("重置售票员密码"),
                                       QStringLiteral("重置售票员密码：%1").arg(trimmedUsername));

    return {true, QStringLiteral("售票员密码已重置。")};
}

AccountResult LoginManager::resetSellerPasswordToDefault(int operatorUserId,
                                                         const QString &username) const
{
    // 默认密码重置仍然调用普通重置函数，这样管理员权限、账号存在和角色检查
    // 只维护一套，不会因为以后修改其中一条规则而漏掉这个入口。
    const AccountResult result =
        resetSellerPassword(operatorUserId, username, defaultSellerPassword());

    if (!result.success) {
        return result;
    }

    return {true,
            QStringLiteral("已重置为默认密码：%1").arg(defaultSellerPassword())};
}

AccountResult LoginManager::setSellerEnabled(int operatorUserId,
                                             const QString &username,
                                             bool enabled) const
{
    // 管理员通过 enabled 控制售票员能否登录。禁用只改变账号状态，
    // 不删除账号和原有信息，之后重新启用仍可以继续使用。
    if (!databaseReady(m_databaseManager)) {
        return {false, QStringLiteral("账号管理服务不可用。")};
    }

    if (!isActiveAdmin(operatorUserId)) {
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

    // 状态修改只更新 enabled 字段，不重写密码和角色等无关数据。
    if (!m_databaseManager->setUserEnabled(user->userId, enabled)) {
        return {false, QStringLiteral("账号状态修改失败。")};
    }

    m_databaseManager->addOperationLog(QStringLiteral("admin:%1").arg(operatorUserId),
                                       enabled ? QStringLiteral("启用售票员") : QStringLiteral("禁用售票员"),
                                       QStringLiteral("%1：%2")
                                           .arg(enabled ? QStringLiteral("启用账号")
                                                        : QStringLiteral("禁用账号"),
                                                trimmedUsername));

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
    // 当前登录用户修改自己的密码：重新读取账号，核对登录身份和原密码，
    // 全部一致后才保存新密码。游客没有 User 记录，因此不能进入这条流程。
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

    // 用户名来自当前登录结果，但 Manager 仍要重新核对数据库角色和原密码。
    // 这样以后即使换了别的界面入口，也不能只传一个用户名就修改密码。
    if (user->role != static_cast<int>(currentRole)) {
        return {false, QStringLiteral("当前身份与账号信息不一致。")};
    }

    if (user->password != oldPassword) {
        return {false, QStringLiteral("原密码不正确。")};
    }

    if (newPassword == oldPassword) {
        return {false, QStringLiteral("新密码不能与原密码相同。")};
    }

    user->password = newPassword;

    if (!m_databaseManager->updateUser(*user)) {
        return {false, QStringLiteral("修改密码失败。")};
    }

    m_databaseManager->addOperationLog(trimmedUsername,
                                       QStringLiteral("修改密码"),
                                       QStringLiteral("用户修改了自己的密码"));

    return {true, QStringLiteral("密码修改成功。")};
}

SellerAccountListResult LoginManager::sellerAccounts(int operatorUserId) const
{
    // 员工权限管理页面用这个函数读取全部售票员账号。
    // 返回值只保留用户名和启用状态，不把密码交给 UI。
    if (!databaseReady(m_databaseManager)) {
        return {false, {}, QStringLiteral("账号列表服务不可用。")};
    }

    if (!isActiveAdmin(operatorUserId)) {
        return {false, {}, QStringLiteral("只有管理员可以查看售票员账号。")};
    }

    const QList<UserRecord> users =
        m_databaseManager->findUsersByRole(static_cast<int>(UserRole::Seller));

    // 查询结果为空可能只是还没有售票员，不能直接当成失败；
    // 只有 DatabaseManager 给出了错误信息时，才向界面返回读取失败。
    if (!m_databaseManager->lastError().isEmpty()) {
        return {false, {}, QStringLiteral("读取售票员账号失败。")};
    }

    SellerAccountListResult result;
    result.success = true;

    for (const UserRecord &user : users) {
        SellerAccountInfo info;
        info.username = user.username;
        info.enabled = user.enabled;
        result.accounts.append(info);
    }

    if (result.accounts.isEmpty()) {
        result.message = QStringLiteral("暂无售票员账号。");
    }

    return result;
}

bool LoginManager::isActiveAdmin(int userId) const
{
    if (!databaseReady(m_databaseManager) || userId <= 0) {
        return false;
    }

    // 不能只相信界面传来的角色。每次管理员操作前都按登录用户 ID 重查数据库，
    // 账号仍然存在、没有被禁用并且 role=2 时，才允许继续执行。
    const auto user = m_databaseManager->findUserById(userId);
    return user.has_value()
           && user->enabled
           && user->role == static_cast<int>(UserRole::Admin);
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
