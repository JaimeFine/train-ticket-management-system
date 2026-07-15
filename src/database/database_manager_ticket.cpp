/** @file database_manager_ticket.cpp - V2: search + query through Trip */
#include "database_manager.h"
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>

// ══════════════════════ 事务辅助函数 ══════════════════════
// 开启事务（购票/退票需要原子地更新余票和订单）
bool DatabaseManager::beginTransaction() {
    return QSqlDatabase::database(m_connectionName).transaction();
}
// 提交事务
bool DatabaseManager::commitTransaction() {
    return QSqlDatabase::database(m_connectionName).commit();
}
// 回滚事务
bool DatabaseManager::rollbackTransaction() {
    return QSqlDatabase::database(m_connectionName).rollback();
}

// ══════════════════════ Issue 9: 余票搜索（V2 经 Trip 查询）══════════════════════
// 按出发/到达站名和日期搜索可售班次；条件为空则不参与过滤
QList<DatabaseManager::TrainWithStations> DatabaseManager::searchTripsByStation(
    const QString &dep, const QString &arr, const QString &date) const
{
    m_lastError.clear();
    QSqlQuery q(QSqlDatabase::database(m_connectionName));
    // JOIN 两次 Station 取起讫站名，JOIN Trip 取具体日期班次；只查启用且有余票的班次
    QString sql = QStringLiteral(
        "SELECT t.trainId,t.trainNumber,t.totalSeats,"
        "t.departureTime,t.arrivalTime,t.enabled,"
        "ds.stationName,as2.stationName,"
        "tr.tripId,tr.remainingSeats,tr.travelDate,"
        "tr.departureTime,tr.arrivalTime,tr.totalSeats "
        "FROM Train t "
        "JOIN Station ds ON t.departureStationId=ds.stationId "
        "JOIN Station as2 ON t.arrivalStationId=as2.stationId "
        "JOIN Trip tr ON tr.trainId=t.trainId "
        "WHERE t.enabled=1 AND tr.enabled=1 AND tr.remainingSeats>0");
    if (!dep.isEmpty()) sql += " AND ds.stationName LIKE :dep";
    if (!arr.isEmpty()) sql += " AND as2.stationName LIKE :arr";
    if (!date.isEmpty()) sql += " AND tr.travelDate=:date";
    sql += " ORDER BY tr.departureTime ASC";
    q.prepare(sql);
    if (!dep.isEmpty()) q.bindValue(":dep", "%" + dep + "%");
    if (!arr.isEmpty()) q.bindValue(":arr", "%" + arr + "%");
    if (!date.isEmpty()) q.bindValue(":date", date);

    QList<TrainWithStations> rs;
    if (!q.exec()) { m_lastError = q.lastError().text(); return rs; }
    while (q.next()) {
        TrainWithStations t;
        t.trainId = q.value(0).toInt();
        t.trainNumber = q.value(1).toString();
        t.totalSeats = q.value(13).toInt();
        t.departureTime = q.value(11).toString();
        t.arrivalTime = q.value(12).toString();
        t.enabled = q.value(5).toBool();
        t.departureStationName = q.value(6).toString();
        t.arrivalStationName = q.value(7).toString();
        t.tripId = q.value(8).toInt();
        t.remainingSeats = q.value(9).toInt();
        t.travelDate = q.value(10).toString();
        rs.append(t);
    }
    return rs;
}

// ══════════════════════ Issue 10: 订单查询（V2：tripId + travelDate）══════════════════════
// 按订单ID查询单条订单，未找到返回 nullopt
std::optional<OrderRecord> DatabaseManager::findOrderById(int orderId) const
{
    m_lastError.clear();
    QSqlQuery q(QSqlDatabase::database(m_connectionName));
    q.prepare("SELECT orderId,userId,tripId,passengerName,travelDate,purchaseTime,status "
              "FROM \"Order\" WHERE orderId=:id");
    q.bindValue(":id", orderId);
    if (!q.exec()) { m_lastError = q.lastError().text(); return std::nullopt; }
    if (!q.next()) return std::nullopt;
    OrderRecord r;
    r.orderId=q.value(0).toInt(); r.userId=q.value(1).toInt();
    r.tripId=q.value(2).toInt(); r.passengerName=q.value(3).toString();
    r.travelDate=q.value(4).toString(); r.purchaseTime=q.value(5).toString();
    r.status=q.value(6).toInt();  // 状态码：0=已购票，1=已退票
    return r;
}

// 按乘客姓名模糊查询订单，按购票时间倒序
QList<OrderRecord> DatabaseManager::findOrdersByPassenger(const QString &name) const
{
    m_lastError.clear();
    QSqlQuery q(QSqlDatabase::database(m_connectionName));
    q.prepare("SELECT orderId,userId,tripId,passengerName,travelDate,purchaseTime,status "
              "FROM \"Order\" WHERE passengerName LIKE :name ORDER BY purchaseTime DESC");
    q.bindValue(":name", "%"+name+"%");
    QList<OrderRecord> rs;
    if (!q.exec()) { m_lastError=q.lastError().text(); return rs; }
    while (q.next()) {
        OrderRecord r;
        r.orderId=q.value(0).toInt(); r.userId=q.value(1).toInt();
        r.tripId=q.value(2).toInt(); r.passengerName=q.value(3).toString();
        r.travelDate=q.value(4).toString(); r.purchaseTime=q.value(5).toString();
        r.status=q.value(6).toInt();
        rs.append(r);
    }
    return rs;
}
