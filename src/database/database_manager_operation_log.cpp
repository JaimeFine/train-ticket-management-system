#include "database_manager.h"

#include <QDateTime>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>

bool DatabaseManager::addOperationLog(const QString &operatorUsername,
                                      const QString &action,
                                      const QString &detail)
{
    m_lastError.clear();
    QSqlQuery query(QSqlDatabase::database(m_connectionName));
    query.prepare(QStringLiteral(
        "INSERT INTO OperationLog (operatorUsername, action, detail, createdAt) "
        "VALUES (:operatorUsername, :action, :detail, :createdAt)"
    ));
    query.bindValue(QStringLiteral(":operatorUsername"), operatorUsername);
    query.bindValue(QStringLiteral(":action"), action);
    query.bindValue(QStringLiteral(":detail"), detail);
    query.bindValue(QStringLiteral(":createdAt"),
                    QDateTime::currentDateTime().toString(QStringLiteral("yyyy-MM-dd HH:mm:ss")));

    if (!query.exec()) {
        m_lastError = query.lastError().text();
        return false;
    }

    return true;
}

QList<DatabaseManager::OperationLogRecord> DatabaseManager::findAllOperationLogs() const
{
    m_lastError.clear();
    QSqlQuery query(QSqlDatabase::database(m_connectionName));
    query.prepare(QStringLiteral(
        "SELECT logId, operatorUsername, action, detail, createdAt "
        "FROM OperationLog ORDER BY logId DESC"
    ));

    QList<OperationLogRecord> results;
    if (!query.exec()) {
        m_lastError = query.lastError().text();
        return results;
    }

    while (query.next()) {
        OperationLogRecord record;
        record.logId = query.value(0).toInt();
        record.operatorUsername = query.value(1).toString();
        record.action = query.value(2).toString();
        record.detail = query.value(3).toString();
        record.createdAt = query.value(4).toString();
        results.append(record);
    }

    return results;
}
