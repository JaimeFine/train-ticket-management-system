// database_manager_trip.cpp - V2 班次(Trip)的增删查改
#include "database_manager.h"
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>

// 新建班次：余票初始为满座，发车/到达时间从所属车次模板复制
std::optional<int> DatabaseManager::createTrip(int trainId, const QString &travelDate,
                                                int totalSeats) {
    m_lastError.clear();
    auto train = findTrainById(trainId);
    // 查不到车次时用兜底时间，保证插入仍能成功
    QString depTime = train ? train->departureTime : QStringLiteral("00:00");
    QString arrTime = train ? train->arrivalTime : QStringLiteral("23:59");

    QSqlQuery q(QSqlDatabase::database(m_connectionName));
    q.prepare("INSERT INTO Trip(trainId,travelDate,departureTime,arrivalTime,"
              "totalSeats,remainingSeats,basePrice,enabled) "
              "VALUES(:tid,:dt,:dep,:arr,:ts,:rs,0,1)");
    q.bindValue(":tid", trainId); q.bindValue(":dt", travelDate);
    q.bindValue(":dep", depTime); q.bindValue(":arr", arrTime);
    q.bindValue(":ts", totalSeats); q.bindValue(":rs", totalSeats);
    if (!q.exec()) { m_lastError = q.lastError().text(); return std::nullopt; }
    return q.lastInsertId().toInt();
}

// 从查询结果当前行读出一条 TripRecord（列顺序须与 TRIP_COLS 一致）
static TripRecord readTrip(QSqlQuery &q) {
    TripRecord r;
    r.tripId=q.value(0).toInt(); r.trainId=q.value(1).toInt();
    r.travelDate=q.value(2).toString();
    r.departureTime=q.value(3).toString(); r.arrivalTime=q.value(4).toString();
    r.totalSeats=q.value(5).toInt(); r.remainingSeats=q.value(6).toInt();
    r.basePrice=q.value(7).toDouble(); r.enabled=q.value(8).toBool();
    return r;
}

// Trip 表查询列清单，各查询共用，顺序与 readTrip 对应
#define TRIP_COLS "tripId,trainId,travelDate,departureTime,arrivalTime,totalSeats,remainingSeats,basePrice,enabled"

// 按 tripId 查询单个班次
std::optional<TripRecord> DatabaseManager::findTripById(int tripId) const {
    m_lastError.clear();
    QSqlQuery q(QSqlDatabase::database(m_connectionName));
    q.prepare("SELECT " TRIP_COLS " FROM Trip WHERE tripId=:id");
    q.bindValue(":id", tripId);
    if (!q.exec()) { m_lastError=q.lastError().text(); return std::nullopt; }
    if (!q.next()) return std::nullopt;
    return readTrip(q);
}

// 查找"车次+日期"对应的班次，不存在则创建（购票时惰性生成班次）
// 先查后插并非原子操作；靠 Trip(trainId,travelDate) 的 UNIQUE 约束兜底，
// 并发下重复插入会失败而不会产生两条同日班次
std::optional<TripRecord> DatabaseManager::findOrCreateTrip(int trainId, const QString &travelDate,
                                                              int totalSeats) {
    m_lastError.clear();
    QSqlQuery q(QSqlDatabase::database(m_connectionName));
    q.prepare("SELECT " TRIP_COLS " FROM Trip WHERE trainId=:tid AND travelDate=:dt");
    q.bindValue(":tid", trainId); q.bindValue(":dt", travelDate);
    if (!q.exec()) { m_lastError=q.lastError().text(); return std::nullopt; }
    if (q.next()) return readTrip(q);
    auto id = createTrip(trainId, travelDate, totalSeats);
    if (!id) return std::nullopt;
    return findTripById(*id);
}

// 增减班次余票：delta<0 表示售票扣减，delta>0 表示退票返还
bool DatabaseManager::adjustTripSeats(int tripId, int delta) {
    m_lastError.clear();
    QSqlQuery q(QSqlDatabase::database(m_connectionName));
    if (delta < 0) {
        // 扣票时在 WHERE 中校验余票充足，单条 UPDATE 原子完成，防止超卖
        q.prepare("UPDATE Trip SET remainingSeats=remainingSeats+:d "
                   "WHERE tripId=:id AND remainingSeats>=:need AND enabled=1");
        q.bindValue(":d", delta); q.bindValue(":need", -delta);
    } else {
        // 加票（退票）无需校验余量
        q.prepare("UPDATE Trip SET remainingSeats=remainingSeats+:d WHERE tripId=:id AND enabled=1");
        q.bindValue(":d", delta);
    }
    q.bindValue(":id", tripId);
    if (!q.exec()) { m_lastError=q.lastError().text(); return false; }
    // 影响行数为 0 说明班次不存在、已停用或余票不足
    return q.numRowsAffected()>0;
}

// 查询某车次的全部班次，按出行日期升序
QList<TripRecord> DatabaseManager::findTripsByTrain(int trainId) const {
    m_lastError.clear();
    QSqlQuery q(QSqlDatabase::database(m_connectionName));
    q.prepare("SELECT " TRIP_COLS " FROM Trip WHERE trainId=:tid ORDER BY travelDate");
    q.bindValue(":tid", trainId);
    QList<TripRecord> rs;
    if (!q.exec()) { m_lastError=q.lastError().text(); return rs; }
    while (q.next()) rs.append(readTrip(q));
    return rs;
}
