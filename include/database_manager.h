#pragma once

#include <optional>

#include <QString>

class QSqlDatabase;

// 登录模块只需要这些用户信息，不需要知道 SQL 怎么写。
struct UserAccountRecord
{
    int userId = 0;
    QString username;
    int role = 0;
    bool enabled = false;
};

class DatabaseManager
{
public:
    DatabaseManager();
    ~DatabaseManager();

    bool initialize();

    bool isOpen() const;
    bool wasCreated() const;
    QString databasePath() const;
    QString lastError() const;

    std::optional<UserAccountRecord> findUserByCredentials(const QString &username,
                                                           const QString &password) const;

private:
    bool openDatabase();
    void closeDatabase();
    bool createTables();
    bool executeStatement(const QString &sql);
    QString resolveDatabasePath() const;

    QString m_connectionName;
    QString m_databasePath;
    mutable QString m_lastError;
    bool m_wasCreated = false;
};
