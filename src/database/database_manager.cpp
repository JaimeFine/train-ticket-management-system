#include "database_manager.h"

// This file implements database connection
// ! Notes
// - QSqpDatabase represents the connection itself
// - QSqlQuery runs SQL statements through an open connection
// - QSqlError gives us the failure message
// - QCoreApplication / QDir / QFileInfo help us buils a safe path

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
    // This block load SQL from the schema file

    QString resolveSchemaPath() {
        QDir baseDir(QCoreApplication::applicationDirPath());

        QString fullPath = baseDir.filePath(QStringLiteral(
            "database/schema_v1.sql")
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

            // Removing comment lines:
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
    "train_ticket_connection"   // ! This is default
)) {
    // We used a named connection instead of an anonymous default connection so
    // this class stays predictable if the project later grows more windows or
    // more test.
}

DatabaseManager::~DatabaseManager() {
    closeDatabase();
}

bool DatabaseManager::initialize() {
    // This is the complete implementation of initialize()
    // ! What it does:
    // 1. opens SQLite,
    // 2. turns on foreign key checking,
    // 3. creates all required tables,
    // 4. returns true only if everything succeeded.

    m_lastError.clear();

    if (!openDatabase()) {
        return false;
    }

    return createTables();
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

    // Handling foreign keys:
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

    // ! If one CREATE TABLE fails, we roll back so
    // ! initialization does not end in a half-finished state.
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
    // * Trying to place the database file in a project-style "database" folder.
    //
    // We support three common run locations:
    // 1. executable launched from the repository root
    // 2. executable launched from the build folder
    // 3. executable launched somewhere else
    //
    // ! In case (3), we still create a local "database" folder so
    // ! initialization works instead of failing.

    QDir baseDir(QCoreApplication::applicationDirPath());

    if (baseDir.exists(QStringLiteral("database"))) {
        return baseDir.filePath(QStringLiteral("database/train_ticket.db"));
    }

    QDir parentDir = baseDir;
    if (parentDir.cdUp() && parentDir.exists(QStringLiteral("database"))) {
        return parentDir.filePath(QStringLiteral("database/train_ticket.db"));
    }

    // If neither directory exists, create one beside the executable.
    baseDir.mkpath(QStringLiteral("database"));
    return baseDir.filePath(QStringLiteral("database/train_ticket.db"));
}
