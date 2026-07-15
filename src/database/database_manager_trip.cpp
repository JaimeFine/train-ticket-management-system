#include "database_manager.h"

#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>

namespace {
// 统一把当前行读取成 TripRecord，避免多个查询函数重复抄字段顺序
TripRecord readTripRecord(QSqlQuery &query)
{
    TripRecord record;
    record.tripId = query.value(0).toInt();
    record.trainId = query.value(1).toInt();
    record.travelDate = query.value(2).toString();
    record.departureTime = query.value(3).toString();
    record.arrivalTime = query.value(4).toString();
    record.totalSeats = query.value(5).toInt();
    record.remainingSeats = query.value(6).toInt();
    record.basePrice = query.value(7).toDouble();
    record.enabled = query.value(8).toBool();
    return record;
}
}

std::optional<int> DatabaseManager::createTrip(int trainId,
                                               const QString &travelDate,
                                               int totalSeats)
{
    m_lastError.clear();

    // createTrip 基于 Train 模板生成当日班次：
    // 继承默认发到时刻，总座位与余票初始相同，基础票价先置 0
    const auto train = findTrainById(trainId);
    if (!train.has_value()) {
        m_lastError = QStringLiteral("Train does not exist.");
        return std::nullopt;
    }

    QSqlQuery query(QSqlDatabase::database(m_connectionName));
    query.prepare(QStringLiteral(
        "INSERT INTO Trip ("
        "trainId, travelDate, departureTime, arrivalTime, "
        "totalSeats, remainingSeats, basePrice, enabled"
        ") VALUES ("
        ":trainId, :travelDate, :departureTime, :arrivalTime, "
        ":totalSeats, :remainingSeats, :basePrice, 1)"
    ));
    query.bindValue(":trainId", trainId);
    query.bindValue(":travelDate", travelDate);
    query.bindValue(":departureTime", train->departureTime);
    query.bindValue(":arrivalTime", train->arrivalTime);
    query.bindValue(":totalSeats", totalSeats);
    query.bindValue(":remainingSeats", totalSeats);
    query.bindValue(":basePrice", 0.0);

    if (!query.exec()) {
        m_lastError = query.lastError().text();
        return std::nullopt;
    }

    return query.lastInsertId().toInt();
}

std::optional<TripRecord> DatabaseManager::findTripById(int tripId) const
{
    m_lastError.clear();
    QSqlQuery query(QSqlDatabase::database(m_connectionName));
    query.prepare(QStringLiteral(
        "SELECT tripId, trainId, travelDate, departureTime, arrivalTime, "
        "totalSeats, remainingSeats, basePrice, enabled "
        "FROM Trip WHERE tripId = :tripId"
    ));
    query.bindValue(":tripId", tripId);

    if (!query.exec()) {
        m_lastError = query.lastError().text();
        return std::nullopt;
    }

    if (!query.next()) {
        return std::nullopt;
    }

    return readTripRecord(query);
}

std::optional<TripRecord> DatabaseManager::findOrCreateTrip(int trainId,
                                                            const QString &travelDate,
                                                            int totalSeats)
{
    m_lastError.clear();
    QSqlQuery query(QSqlDatabase::database(m_connectionName));
    query.prepare(QStringLiteral(
        "SELECT tripId, trainId, travelDate, departureTime, arrivalTime, "
        "totalSeats, remainingSeats, basePrice, enabled "
        "FROM Trip WHERE trainId = :trainId AND travelDate = :travelDate"
    ));
    query.bindValue(":trainId", trainId);
    query.bindValue(":travelDate", travelDate);

    if (!query.exec()) {
        m_lastError = query.lastError().text();
        return std::nullopt;
    }

    if (query.next()) {
        // 已存在当天班次时直接返回，不重复创建
        return readTripRecord(query);
    }

    const auto createdTripId = createTrip(trainId, travelDate, totalSeats);
    if (!createdTripId.has_value()) {
        return std::nullopt;
    }

    return findTripById(*createdTripId);
}

bool DatabaseManager::adjustTripSeats(int tripId, int delta)
{
    m_lastError.clear();
    QSqlQuery query(QSqlDatabase::database(m_connectionName));

    // 扣票时额外校验 remainingSeats >= 需要扣减的数量，防止并发/重复请求导致超卖
    if (delta < 0) {
        query.prepare(QStringLiteral(
            "UPDATE Trip "
            "SET remainingSeats = remainingSeats + :delta "
            "WHERE tripId = :tripId AND enabled = 1 AND remainingSeats >= :required"
        ));
        query.bindValue(":required", -delta);
    } else {
        query.prepare(QStringLiteral(
            "UPDATE Trip "
            "SET remainingSeats = remainingSeats + :delta "
            "WHERE tripId = :tripId AND enabled = 1"
        ));
    }

    query.bindValue(":delta", delta);
    query.bindValue(":tripId", tripId);

    if (!query.exec()) {
        m_lastError = query.lastError().text();
        return false;
    }

    if (query.numRowsAffected() == 0) {
        m_lastError = QStringLiteral("No enabled trip was updated.");
        return false;
    }

    return true;
}

QList<TripRecord> DatabaseManager::findTripsByTrain(int trainId) const
{
    m_lastError.clear();
    QSqlQuery query(QSqlDatabase::database(m_connectionName));
    query.prepare(QStringLiteral(
        "SELECT tripId, trainId, travelDate, departureTime, arrivalTime, "
        "totalSeats, remainingSeats, basePrice, enabled "
        "FROM Trip WHERE trainId = :trainId ORDER BY travelDate, departureTime"
    ));
    query.bindValue(":trainId", trainId);

    QList<TripRecord> results;
    if (!query.exec()) {
        m_lastError = query.lastError().text();
        return results;
    }

    while (query.next()) {
        results.append(readTripRecord(query));
    }

    return results;
}

bool DatabaseManager::updateTrip(const TripRecord &trip)
{
    m_lastError.clear();
    QSqlQuery query(QSqlDatabase::database(m_connectionName));

    query.prepare(QStringLiteral(
        "UPDATE Trip SET "
        "travelDate = :travelDate, "
        "departureTime = :departureTime, "
        "arrivalTime = :arrivalTime, "
        "totalSeats = :totalSeats, "
        "remainingSeats = :remainingSeats, "
        "basePrice = :basePrice, "
        "enabled = :enabled "
        "WHERE tripId = :tripId"
        ));

    query.bindValue(":tripId", trip.tripId);
    query.bindValue(":travelDate", trip.travelDate);
    query.bindValue(":departureTime", trip.departureTime);
    query.bindValue(":arrivalTime", trip.arrivalTime);
    query.bindValue(":totalSeats", trip.totalSeats);
    query.bindValue(":remainingSeats", trip.remainingSeats);
    query.bindValue(":basePrice", trip.basePrice);
    query.bindValue(":enabled", trip.enabled ? 1 : 0);

    if (!query.exec()) {
        m_lastError = query.lastError().text();
        return false;
    }

    if (query.numRowsAffected() == 0) {
        m_lastError = QStringLiteral("No trip found for the given tripId.");
        return false;
    }

    return true;
}
