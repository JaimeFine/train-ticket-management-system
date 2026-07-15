#include "database_manager.h"

// 数据库连接、schema 初始化与路径解析。

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

namespace {
    QString resolveSchemaPath() {
        QDir baseDir(QCoreApplication::applicationDirPath());

        QString fullPath = baseDir.filePath(QStringLiteral(
            "database/schema_v2.sql")
        );

        return fullPath;
    }

    QStringList parseSqlStatements(const QString &sqlText) {
        QStringList statements;
        const QStringList rawParts = sqlText.split(';', Qt::SkipEmptyParts);

        for (const QString &rawPart : rawParts) {
            QString statement = rawPart.trimmed();
            if (statement.isEmpty()) {
                continue;
            }

            // 去掉 -- 开头的注释行。
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

}   // namespace

DatabaseManager::DatabaseManager() : m_connectionName(QStringLiteral(
    "train_ticket_connection"   // ! 默认连接名
)) {
    // 用命名连接而非匿名默认连接，将来多窗口/多测试并存时行为更可控。
}

DatabaseManager::~DatabaseManager() {
    closeDatabase();
}

bool DatabaseManager::initialize() {
    m_lastError.clear();

    if (!openDatabase()) {
        return false;
    }

    if (!createTables()) {
        return false;
    }

    return seedDemoData();
}

bool DatabaseManager::isOpen() const {
    if (!QSqlDatabase::contains(m_connectionName)) {
        return false;
    }

    return QSqlDatabase::database(m_connectionName).isOpen();
}

bool DatabaseManager::executeStatement(const QString &sql) {
    QSqlQuery query(QSqlDatabase::database(m_connectionName));
    if (!query.exec(sql)) {
        m_lastError = query.lastError().text();
        return false;
    }

    return true;
}

QString DatabaseManager::databasePath() const {
    return m_databasePath;
}

QString DatabaseManager::lastError() const {
    return m_lastError;
}

bool DatabaseManager::wasCreated() const {
    return m_wasCreated;
}

bool DatabaseManager::openDatabase() {
    m_databasePath = resolveDatabasePath();

    // 记录文件是否已存在，用于区分本次是新建还是复用数据库。
    const bool databaseFileAlreadyExists = QFileInfo::exists(m_databasePath);

    QSqlDatabase database;
    if (QSqlDatabase::contains(m_connectionName)) {
        database = QSqlDatabase::database(m_connectionName);
    } else {
        database = QSqlDatabase::addDatabase(
            QStringLiteral("QSQLITE"), m_connectionName
        );
    }

    database.setDatabaseName(m_databasePath);

    if (!database.open()) {
        m_lastError = database.lastError().text();
        return false;
    }

    if (!databaseFileAlreadyExists) {
        m_wasCreated = true;
    }

    // SQLite 默认关闭外键约束，必须对每个连接显式打开。
    QSqlQuery pragmaQuery(database);
    if (!pragmaQuery.exec(QStringLiteral("PRAGMA foreign_keys = ON;"))) {
        m_lastError = pragmaQuery.lastError().text();
        return false;
    }

    return true;
}

void DatabaseManager::closeDatabase() {
    if (!QSqlDatabase::contains(m_connectionName)) {
        return;
    }

    // 先释放局部 QSqlDatabase，再 removeDatabase，避免"连接仍在使用"警告。
    {
        QSqlDatabase database = QSqlDatabase::database(m_connectionName);
        if (database.isOpen()) {
            database.close();
        }
    }

    QSqlDatabase::removeDatabase(m_connectionName);
}

bool DatabaseManager::createTables() {
    if (!QSqlDatabase::contains(m_connectionName)) {
        m_lastError = QStringLiteral("Database connection does not exist.");
        return false;
    }

    QSqlDatabase database = QSqlDatabase::database(m_connectionName);
    if (!database.isOpen()) {
        m_lastError = QStringLiteral(
            "Database connection is not open."
        );
        return false;
    }

    // 任一建表语句失败时整体回滚，避免初始化停在半完成状态。
    if (!database.transaction()) {
        m_lastError = database.lastError().text();
        return false;
    }

    const QString schemaPath = resolveSchemaPath();
    QFile schemaFile(schemaPath);
    if (!schemaFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        m_lastError = QStringLiteral(
            "Failed to open schema file: %1"
        ).arg(schemaPath);
        database.rollback();
        return false;
    }

    QTextStream input(&schemaFile);
    const QString sqlText = input.readAll();
    schemaFile.close();

    const QStringList statements = parseSqlStatements(sqlText);
    if (statements.isEmpty()) {
        m_lastError = QStringLiteral(
            "Schema file is empty or could not be parsed: %1"
        ).arg(schemaPath);
        database.rollback();
        return false;
    }

    for (const QString &statement : statements) {
        if (!executeStatement(statement)) {
            m_lastError = QStringLiteral(
                "While executing schema file %1: %2"
            ).arg(schemaPath, m_lastError);
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

QString DatabaseManager::resolveDatabasePath() const {
    // 优先复用仓库或构建目录旁的 database 目录，必要时再在可执行文件旁创建。

    QDir baseDir(QCoreApplication::applicationDirPath());

    if (baseDir.exists(QStringLiteral("database"))) {
        return baseDir.filePath(QStringLiteral("database/train_ticket_v2.db"));
    }

    QDir parentDir = baseDir;
    if (parentDir.cdUp() && parentDir.exists(QStringLiteral("database"))) {
        return parentDir.filePath(QStringLiteral("database/train_ticket_v2.db"));
    }

    baseDir.mkpath(QStringLiteral("database"));
    return baseDir.filePath(QStringLiteral("database/train_ticket_v2.db"));
}
