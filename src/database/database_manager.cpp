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

    bool execPrepared(QSqlDatabase &database,
                      QString *lastError,
                      const QString &sql,
                      const QList<QVariant> &values = {}) {
        QSqlQuery query(database);
        query.prepare(sql);

        for (const QVariant &value : values) {
            query.addBindValue(value);
        }

        if (!query.exec()) {
            if (lastError != nullptr) {
                *lastError = query.lastError().text();
            }
            return false;
        }

        return true;
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

bool DatabaseManager::seedDemoData() {
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

    const struct DemoUserSeed {
        QString username;
        QString password;
        int role;
        int enabled;
    } demoUsers[] = {
        {QStringLiteral("admin"), QStringLiteral("admin123"), 2, 1},
        {QStringLiteral("seller"), QStringLiteral("seller123"), 1, 1},
        {QStringLiteral("user01"), QStringLiteral("user123"), 3, 1},
        {QStringLiteral("user02"), QStringLiteral("user456"), 3, 1}
    };

    for (const DemoUserSeed &user : demoUsers) {
        if (!execPrepared(database,
                          &m_lastError,
                          QStringLiteral(
                              "INSERT OR IGNORE INTO User "
                              "(username, password, role, enabled) "
                              "VALUES (?, ?, ?, ?)"
                          ),
                          {user.username, user.password, user.role, user.enabled})) {
            database.rollback();
            return false;
        }
    }

    const QStringList demoStations = {
        QStringLiteral("北京南"),
        QStringLiteral("南京南"),
        QStringLiteral("上海虹桥"),
        QStringLiteral("杭州东")
    };

    for (const QString &stationName : demoStations) {
        if (!execPrepared(database,
                          &m_lastError,
                          QStringLiteral(
                              "INSERT OR IGNORE INTO Station (stationName) VALUES (?)"
                          ),
                          {stationName})) {
            database.rollback();
            return false;
        }
    }

    const struct DemoTrainSeed {
        QString trainNumber;
        QString departureStationName;
        QString arrivalStationName;
        QString departureTime;
        QString arrivalTime;
        int totalSeats;
        int enabled;
    } demoTrains[] = {
        {QStringLiteral("G1001"), QStringLiteral("北京南"), QStringLiteral("上海虹桥"),
         QStringLiteral("08:00"), QStringLiteral("12:38"), 600, 1},
        {QStringLiteral("G1002"), QStringLiteral("上海虹桥"), QStringLiteral("北京南"),
         QStringLiteral("14:00"), QStringLiteral("18:40"), 600, 1},
        {QStringLiteral("G2001"), QStringLiteral("南京南"), QStringLiteral("杭州东"),
         QStringLiteral("09:15"), QStringLiteral("11:10"), 480, 1},
        {QStringLiteral("G2002"), QStringLiteral("杭州东"), QStringLiteral("南京南"),
         QStringLiteral("15:20"), QStringLiteral("17:18"), 480, 1}
    };

    for (const DemoTrainSeed &train : demoTrains) {
        if (!execPrepared(
                database,
                &m_lastError,
                QStringLiteral(
                    "INSERT OR IGNORE INTO Train ("
                    "trainNumber, departureStationId, arrivalStationId, "
                    "departureTime, arrivalTime, totalSeats, enabled"
                    ") "
                    "SELECT ?, dep.stationId, arr.stationId, ?, ?, ?, ? "
                    "FROM Station dep, Station arr "
                    "WHERE dep.stationName = ? AND arr.stationName = ?"
                ),
                {
                    train.trainNumber,
                    train.departureTime,
                    train.arrivalTime,
                    train.totalSeats,
                    train.enabled,
                    train.departureStationName,
                    train.arrivalStationName
                })) {
            database.rollback();
            return false;
        }
    }

    const QString today =
        QDateTime::currentDateTime().toString(QStringLiteral("yyyy-MM-dd"));
    const QString tomorrow =
        QDateTime::currentDateTime().addDays(1).toString(QStringLiteral("yyyy-MM-dd"));

    for (const QString &travelDate : {today, tomorrow}) {
        for (const DemoTrainSeed &train : demoTrains) {
            if (!execPrepared(
                    database,
                    &m_lastError,
                    QStringLiteral(
                        "INSERT OR IGNORE INTO Trip ("
                        "trainId, travelDate, departureTime, arrivalTime, "
                        "totalSeats, remainingSeats, basePrice, enabled"
                        ") "
                        "SELECT trainId, ?, departureTime, arrivalTime, totalSeats, totalSeats, 0, 1 "
                        "FROM Train WHERE trainNumber = ?"
                    ),
                    {travelDate, train.trainNumber})) {
                database.rollback();
                return false;
            }
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
        return baseDir.filePath(QStringLiteral("database/train_ticket_v2.db"));
    }

    QDir parentDir = baseDir;
    if (parentDir.cdUp() && parentDir.exists(QStringLiteral("database"))) {
        return parentDir.filePath(QStringLiteral("database/train_ticket_v2.db"));
    }

    // If neither directory exists, create one beside the executable.
    baseDir.mkpath(QStringLiteral("database"));
    return baseDir.filePath(QStringLiteral("database/train_ticket_v2.db"));
}
