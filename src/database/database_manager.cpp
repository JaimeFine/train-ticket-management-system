#include "database_manager.h"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QString>
#include <QStringList>
#include <QTextStream>
#include <QVariant>

namespace {
QString resolveSchemaPath()
{
    QDir baseDir(QCoreApplication::applicationDirPath());
    const QString localSchemaPath = baseDir.filePath(QStringLiteral("database/schema_v1.sql"));
    if (QFileInfo::exists(localSchemaPath)) {
        return localSchemaPath;
    }

    QDir parentDir = baseDir;
    if (parentDir.cdUp()) {
        const QString parentSchemaPath = parentDir.filePath(QStringLiteral("database/schema_v1.sql"));
        if (QFileInfo::exists(parentSchemaPath)) {
            return parentSchemaPath;
        }
    }

    return localSchemaPath;
}

QStringList parseSqlStatements(const QString &sqlText)
{
    QStringList statements;
    const QStringList rawParts = sqlText.split(';', Qt::SkipEmptyParts);

    for (const QString &rawPart : rawParts) {
        QString statement = rawPart.trimmed();
        if (statement.isEmpty()) {
            continue;
        }

        QStringList filteredLines;
        const QStringList lines = statement.split('\n');
        for (const QString &line : lines) {
            const QString trimmedLine = line.trimmed();
            if (trimmedLine.startsWith(QStringLiteral("--"))) {
                continue;
            }
            filteredLines.append(line);
        }

        statement = filteredLines.join('\n').trimmed();
        if (!statement.isEmpty()) {
            statements.append(statement);
        }
    }

    return statements;
}
} // namespace

DatabaseManager::DatabaseManager()
    : m_connectionName(QStringLiteral("train_ticket_connection"))
{
}

DatabaseManager::~DatabaseManager()
{
    closeDatabase();
}

bool DatabaseManager::initialize()
{
    m_lastError.clear();

    if (!openDatabase()) {
        return false;
    }

    return createTables();
}

bool DatabaseManager::isOpen() const
{
    if (!QSqlDatabase::contains(m_connectionName)) {
        return false;
    }

    return QSqlDatabase::database(m_connectionName).isOpen();
}

bool DatabaseManager::wasCreated() const
{
    return m_wasCreated;
}

QString DatabaseManager::databasePath() const
{
    return m_databasePath;
}

QString DatabaseManager::lastError() const
{
    return m_lastError;
}

std::optional<UserAccountRecord> DatabaseManager::findUserByCredentials(const QString &username,
                                                                        const QString &password) const
{
    if (!QSqlDatabase::contains(m_connectionName)) {
        m_lastError = QStringLiteral("Database connection does not exist.");
        return std::nullopt;
    }

    QSqlDatabase database = QSqlDatabase::database(m_connectionName);
    if (!database.isOpen()) {
        m_lastError = QStringLiteral("Database connection is not open.");
        return std::nullopt;
    }

    // 用户名和密码查询统一放在 DatabaseManager 里，UI 和 LoginManager 都不写 SQL。
    QSqlQuery query(database);
    query.prepare(QStringLiteral(
        "SELECT userId, username, role, enabled "
        "FROM User "
        "WHERE username = :username AND password = :password"
    ));
    query.bindValue(QStringLiteral(":username"), username);
    query.bindValue(QStringLiteral(":password"), password);

    if (!query.exec()) {
        m_lastError = query.lastError().text();
        return std::nullopt;
    }

    if (!query.next()) {
        return std::nullopt;
    }

    UserAccountRecord record;
    record.userId = query.value(0).toInt();
    record.username = query.value(1).toString();
    record.role = query.value(2).toInt();
    record.enabled = query.value(3).toInt() != 0;
    return record;
}

bool DatabaseManager::executeStatement(const QString &sql)
{
    QSqlQuery query(QSqlDatabase::database(m_connectionName));
    if (!query.exec(sql)) {
        m_lastError = query.lastError().text();
        return false;
    }

    return true;
}

bool DatabaseManager::openDatabase()
{
    m_databasePath = resolveDatabasePath();

    const bool databaseFileAlreadyExists = QFileInfo::exists(m_databasePath);

    QSqlDatabase database;
    if (QSqlDatabase::contains(m_connectionName)) {
        database = QSqlDatabase::database(m_connectionName);
    } else {
        database = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), m_connectionName);
    }

    database.setDatabaseName(m_databasePath);

    if (!database.open()) {
        m_lastError = database.lastError().text();
        return false;
    }

    if (!databaseFileAlreadyExists) {
        m_wasCreated = true;
    }

    QSqlQuery pragmaQuery(database);
    if (!pragmaQuery.exec(QStringLiteral("PRAGMA foreign_keys = ON;"))) {
        m_lastError = pragmaQuery.lastError().text();
        return false;
    }

    return true;
}

void DatabaseManager::closeDatabase()
{
    if (!QSqlDatabase::contains(m_connectionName)) {
        return;
    }

    {
        QSqlDatabase database = QSqlDatabase::database(m_connectionName);
        if (database.isOpen()) {
            database.close();
        }
    }

    QSqlDatabase::removeDatabase(m_connectionName);
}

bool DatabaseManager::createTables()
{
    if (!QSqlDatabase::contains(m_connectionName)) {
        m_lastError = QStringLiteral("Database connection does not exist.");
        return false;
    }

    QSqlDatabase database = QSqlDatabase::database(m_connectionName);
    if (!database.isOpen()) {
        m_lastError = QStringLiteral("Database connection is not open.");
        return false;
    }

    if (!database.transaction()) {
        m_lastError = database.lastError().text();
        return false;
    }

    const QString schemaPath = resolveSchemaPath();
    QFile schemaFile(schemaPath);
    if (!schemaFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        m_lastError = QStringLiteral("Failed to open schema file: %1").arg(schemaPath);
        database.rollback();
        return false;
    }

    QTextStream input(&schemaFile);
    const QString sqlText = input.readAll();
    schemaFile.close();

    const QStringList statements = parseSqlStatements(sqlText);
    if (statements.isEmpty()) {
        m_lastError = QStringLiteral("Schema file is empty or could not be parsed: %1").arg(schemaPath);
        database.rollback();
        return false;
    }

    for (const QString &statement : statements) {
        if (!executeStatement(statement)) {
            m_lastError = QStringLiteral("While executing schema file %1: %2").arg(schemaPath, m_lastError);
            database.rollback();
            return false;
        }
    }

    if (!database.commit()) {
        m_lastError = database.lastError().text();
        database.rollback();
        return false;
    }

    return true;
}

QString DatabaseManager::resolveDatabasePath() const
{
    QDir baseDir(QCoreApplication::applicationDirPath());

    if (baseDir.exists(QStringLiteral("database"))) {
        return baseDir.filePath(QStringLiteral("database/train_ticket.db"));
    }

    QDir parentDir = baseDir;
    if (parentDir.cdUp() && parentDir.exists(QStringLiteral("database"))) {
        return parentDir.filePath(QStringLiteral("database/train_ticket.db"));
    }

    baseDir.mkpath(QStringLiteral("database"));
    return baseDir.filePath(QStringLiteral("database/train_ticket.db"));
}
