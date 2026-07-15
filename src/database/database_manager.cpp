#include "database_manager.h"

// 本文件实现数据库连接管理（打开/建表/种子数据）
// ! 说明
// - QSqlDatabase 表示连接本身
// - QSqlQuery 通过已打开的连接执行 SQL
// - QSqlError 提供失败信息
// - QCoreApplication / QDir / QFileInfo 用于构造安全路径

#include <QCoreApplication>
#include <QDateTime>
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
    // 本匿名命名空间：从 schema 文件加载 SQL 的辅助函数

    // 解析 schema 文件路径（可执行文件目录下的 database/schema_v2.sql）
    QString resolveSchemaPath() {
        QDir baseDir(QCoreApplication::applicationDirPath());

        QString fullPath = baseDir.filePath(QStringLiteral(
            "database/schema_v2.sql")
        );

        return fullPath;
    }

    // 把整段 SQL 文本按分号拆成独立语句，并剔除 -- 注释行
    QStringList parseSqlStatements(const QString &sqlText) {
        QStringList statements;
        const QStringList rawParts = sqlText.split(';', Qt::SkipEmptyParts);

        for (const QString &rawPart : rawParts) {
            QString statement = rawPart.trimmed();
            if (statement.isEmpty()) {
                continue;
            }

            // 去掉 -- 开头的注释行：
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

    // 执行带参数绑定的预编译 SQL，失败时把错误写入 lastError
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

// 构造函数：设定固定的命名连接名
DatabaseManager::DatabaseManager() : m_connectionName(QStringLiteral(
    "train_ticket_connection"   // ! 默认连接名
)) {
    // 用命名连接而非匿名默认连接，将来多窗口/多测试并存时行为更可控
}

// 析构函数：关闭并移除数据库连接
DatabaseManager::~DatabaseManager() {
    closeDatabase();
}

// 初始化数据库：打开连接、建表、写入演示数据
bool DatabaseManager::initialize() {
    // ! 步骤：
    // 1. 打开 SQLite，
    // 2. 启用外键检查，
    // 3. 创建所有必需的表，
    // 4. 全部成功才返回 true。

    m_lastError.clear();

    if (!openDatabase()) {
        return false;
    }

    if (!createTables()) {
        return false;
    }

    return seedDemoData();
}

// 判断命名连接是否存在且已打开
bool DatabaseManager::isOpen() const {
    if (!QSqlDatabase::contains(m_connectionName)) {
        return false;
    }

    return QSqlDatabase::database(m_connectionName).isOpen();
}

// 直接执行一条 SQL 语句，失败时记录错误信息
bool DatabaseManager::executeStatement(const QString &sql) {
    QSqlQuery query(QSqlDatabase::database(m_connectionName));
    if (!query.exec(sql)) {
        m_lastError = query.lastError().text();
        return false;
    }

    return true;
}

// 返回数据库文件路径
QString DatabaseManager::databasePath() const {
    return m_databasePath;
}

// 返回最近一次错误信息
QString DatabaseManager::lastError() const {
    return m_lastError;
}

// 返回数据库文件是否为本次运行新建
bool DatabaseManager::wasCreated() const {
    return m_wasCreated;
}

// 打开（或复用）SQLite 连接，并启用外键约束
bool DatabaseManager::openDatabase() {
    m_databasePath = resolveDatabasePath();

    // 记录文件是否已存在，用于区分"新建"和"复用"数据库
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

    // 启用外键：SQLite 默认关闭，必须每个连接手动打开
    QSqlQuery pragmaQuery(database);
    if (!pragmaQuery.exec(QStringLiteral("PRAGMA foreign_keys = ON;"))) {
        m_lastError = pragmaQuery.lastError().text();
        return false;
    }

    return true;
}

// 关闭并移除命名连接
void DatabaseManager::closeDatabase() {
    if (!QSqlDatabase::contains(m_connectionName)) {
        return;
    }

    // 局部作用域让 QSqlDatabase 对象先析构，removeDatabase 才不会警告"连接仍在使用"
    {
        QSqlDatabase database = QSqlDatabase::database(m_connectionName);
        if (database.isOpen()) {
            database.close();
        }
    }

    QSqlDatabase::removeDatabase(m_connectionName);
}

// 读取 schema 文件并在事务中执行建表语句
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

    // ! 建表放在事务中：任一 CREATE TABLE 失败就整体回滚，
    // ! 避免初始化停在"建了一半"的状态。
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

// 写入演示数据（用户/车站/车次/班次），INSERT OR IGNORE 保证可重复执行
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

    // 整批种子数据放同一事务：要么全部写入，要么全部回滚
    if (!database.transaction()) {
        m_lastError = database.lastError().text();
        return false;
    }

    // 演示用户：管理员/售票员/普通用户
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

    // 演示车站
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

    // 演示车次：站名在插入时通过子查询转换为 stationId
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

    // V2：为演示车次生成今天和明天的班次(Trip)记录，余票初始为满座
    const QString today = QDateTime::currentDateTime().toString("yyyy-MM-dd");
    const QString tomorrow = QDateTime::currentDateTime().addDays(1).toString("yyyy-MM-dd");
    for (const QString &date : {today, tomorrow}) {
        for (const DemoTrainSeed &train : demoTrains) {
            if (!execPrepared(database, &m_lastError,
                    QStringLiteral("INSERT OR IGNORE INTO Trip "
                        "(trainId,travelDate,departureTime,arrivalTime,totalSeats,remainingSeats,enabled) "
                        "SELECT trainId,?,departureTime,arrivalTime,totalSeats,totalSeats,1 "
                        "FROM Train WHERE trainNumber=?"),
                    {date, train.trainNumber})) {
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

// 解析数据库文件存放路径
QString DatabaseManager::resolveDatabasePath() const {
    // * 尽量把数据库文件放进项目风格的 "database" 目录。
    //
    // 支持三种常见运行位置：
    // 1. 从仓库根目录启动可执行文件
    // 2. 从 build 目录启动
    // 3. 从其他位置启动
    //
    // ! 情况(3)下也会在可执行文件旁新建 "database" 目录，
    // ! 保证初始化不会因目录缺失而失败。

    QDir baseDir(QCoreApplication::applicationDirPath());

    if (baseDir.exists(QStringLiteral("database"))) {
        return baseDir.filePath(QStringLiteral("database/train_ticket.db"));
    }

    QDir parentDir = baseDir;
    if (parentDir.cdUp() && parentDir.exists(QStringLiteral("database"))) {
        return parentDir.filePath(QStringLiteral("database/train_ticket.db"));
    }

    // 两处都没有时，在可执行文件旁新建 database 目录。
    baseDir.mkpath(QStringLiteral("database"));
    return baseDir.filePath(QStringLiteral("database/train_ticket.db"));
}
