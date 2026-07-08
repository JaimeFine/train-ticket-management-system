#include "database_manager.h"

#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QString>

bool DatabaseManager::addUser(const UserRecord &user) {
    QSqlQuery query(QSqlDatabase::database(m_connectionName));

    query.prepare(QStringLiteral(
        "INSERT INTO User (username, password, role, enabled) "
        "VALUES (:username, :password, :role, :enabled)"
    ));

    query.bindValue(":username", user.username);
    query.bindValue(":password", user.password);
    query.bindValue(":role", user.role);
    query.bindValue(":enabled", user.enabled);

    if (!query.exec()) {
        m_lastError = query.lastError().text();
        return false;
    }

    return true;
}

std::optional<UserRecord> DatabaseManager::findUserById(int userId) const {
    QSqlQuery query(QSqlDatabase::database(m_connectionName));

    query.prepare(QStringLiteral(
        "SELECT userId, username, password, role, enabled "
        "FROM User "
        "WHERE userId = :userId"
    ));

    query.bindValue(":userId", userId); // This bind the parameter!

    if (!query.exec()) {
        m_lastError = query.lastError().text();
        return std::nullopt;
    }

    if (!query.next()) {
        return std::nullopt;
    }

    UserRecord record;
    record.userId = query.value(0).toInt();
    record.username = query.value(1).toString();
    record.password = query.value(2).toString();
    record.role = query.value(3).toInt();
    record.enabled = query.value(4).toBool();

    return record;
}

std::optional<UserRecord> DatabaseManager::findUserByUsername(
    const QString &username
) const {
    QSqlQuery query(QSqlDatabase::database(m_connectionName));

    query.prepare(QStringLiteral(
        "SELECT userId, username, password, role, enabled "
        "FROM User "
        "WHERE username = :username"
    ));

    query.bindValue(":username", username);

    if (!query.exec()) {
        m_lastError = query.lastError().text();
        return std::nullopt;
    }

    if (!query.next()) {
        return std::nullopt;
    }

    UserRecord record;
    record.userId = query.value(0).toInt();
    record.username = query.value(1).toString();
    record.password = query.value(2).toString();
    record.role = query.value(3).toInt();
    record.enabled = query.value(4).toBool();

    return record;
}

bool DatabaseManager::updateUser(const UserRecord &user) {
    QSqlQuery query(QSqlDatabase::database(m_connectionName));

    query.prepare(QStringLiteral(
        "UPDATE User "
        "SET username = :username, "
        "   password = :password, "
        "   role = :role, "
        "   enabled = :enabled "
        "WHERE userId = :userId"
    ));

    query.bindValue(":userId", user.userId);
    query.bindValue(":username", user.username);
    query.bindValue(":password", user.password);
    query.bindValue(":role", user.role);
    query.bindValue(":enabled", user.enabled);

    if (!query.exec()) {
        m_lastError = query.lastError().text();
        return false;
    }

    // Check if any row was actually updated
    if (query.numRowsAffected() == 0) {
        return false;   // userId did not exist
    }

    return true;
}

bool DatabaseManager::setUserEnabled(int userId, bool enabled) {
    QSqlQuery query(QSqlDatabase::database(m_connectionName));

    query.prepare(QStringLiteral(
        "UPDATE User "
        "SET enabled = :enabled "
        "WHERE userId = :userId"
    ));

    query.bindValue(":userId", userId);
    query.bindValue(":enabled", enabled);

    if (!query.exec()) {
        m_lastError = query.lastError().text();
        return false;
    }

    // Check if any row was actually updated
    if (query.numRowsAffected() == 0) {
        return false;   // userId did not exist
    }

    return true;
}