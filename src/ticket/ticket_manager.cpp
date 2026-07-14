#include "ticket_manager.h"
#include "database_manager.h"
#include <QDateTime>

TicketManager::TicketManager(DatabaseManager &db) : m_db(db) {}

// ══════════════════════ Issue 9: V2 tripId + travelDate ══════════════════════
int TicketManager::bookTicket(int userId, int tripId, const QString &passengerName) {
    auto trip = m_db.findTripById(tripId);
    if (!trip) { m_lastError = "班次不存在"; return -1; }
    if (!trip->enabled) { m_lastError = "该班次已停运"; return -1; }
    if (trip->remainingSeats <= 0) { m_lastError = "该班次已无余票"; return -1; }

    auto train = m_db.findTrainById(trip->trainId);
    if (!train || !train->enabled) { m_lastError = "该车次已停运"; return -1; }

    if (!m_db.beginTransaction()) { m_lastError = "无法开启事务"; return -1; }
    if (!m_db.adjustTripSeats(tripId, -1)) {
        m_db.rollbackTransaction(); m_lastError = "扣减座位失败"; return -1;
    }
    OrderRecord o;
    o.userId = userId; o.tripId = tripId; o.passengerName = passengerName;
    o.travelDate = trip->travelDate;
    o.purchaseTime = QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss");
    o.price = trip->basePrice;
    o.status = 0;
    const auto orderId = m_db.createOrder(o);
    if (!orderId.has_value()) {
        m_db.rollbackTransaction(); m_lastError = "创建订单失败"; return -1;
    }
    if (!m_db.commitTransaction()) {
        m_db.rollbackTransaction(); m_lastError = "提交事务失败"; return -1;
    }
    return *orderId;
}

int TicketManager::remainingSeats(int tripId) const {
    auto t = m_db.findTripById(tripId);
    return t ? t->remainingSeats : -1;
}

static QVariantMap trip2map(const DatabaseManager::TrainWithStations &t) {
    QVariantMap m;
    m["trainId"] = t.trainId; m["trainNumber"] = t.trainNumber;
    m["departureStation"] = t.departureStationName; m["arrivalStation"] = t.arrivalStationName;
    m["departureTime"] = t.departureTime; m["arrivalTime"] = t.arrivalTime;
    m["remainingSeats"] = t.remainingSeats; m["totalSeats"] = t.totalSeats;
    m["tripId"] = t.tripId; m["travelDate"] = t.travelDate;
    return m;
}

QVector<QVariantMap> TicketManager::searchTrips(const QString &dep, const QString &arr, const QString &date) const {
    QVector<QVariantMap> r;
    for (auto &t : m_db.searchTripsByStation(dep, arr, date)) r.append(trip2map(t));
    return r;
}

QVector<QVariantMap> TicketManager::searchByTrainNumber(const QString &num) const {
    auto t = m_db.findTrainByNumber(num);
    if (!t || !t->enabled) return {};
    auto ds = m_db.findStationById(t->departureStationId);
    auto as = m_db.findStationById(t->arrivalStationId);
    auto trips = m_db.findTripsByTrain(t->trainId);
    QVector<QVariantMap> results;
    for (const auto &tr : trips) {
        QVariantMap m;
        m["trainId"]=t->trainId; m["trainNumber"]=t->trainNumber;
        m["departureStation"]=ds?ds->stationName:""; m["arrivalStation"]=as?as->stationName:"";
        m["departureTime"]=tr.departureTime; m["arrivalTime"]=tr.arrivalTime;
        m["totalSeats"]=tr.totalSeats; m["tripId"]=tr.tripId;
        m["travelDate"]=tr.travelDate; m["remainingSeats"]=tr.remainingSeats;
        results.append(m);
    }
    return results;
}

QString TicketManager::lastError() const { return m_lastError; }

// ══════════════════════ Issue 10: V2 tripId ══════════════════════
bool TicketManager::refundTicket(int orderId)
{
    auto order = m_db.findOrderById(orderId);
    if (!order) { m_lastError="订单不存在"; return false; }
    if (order->status != 0 && order->status != 3) {
        m_lastError="该订单无法退票（已退票或已改签）"; return false;
    }
    if (!m_db.beginTransaction()) { m_lastError="无法开启事务"; return false; }
    if (!m_db.updateOrderStatus(orderId, 1)) {
        m_db.rollbackTransaction(); m_lastError="退票失败"; return false;
    }
    if (!m_db.adjustTripSeats(order->tripId, +1)) {
        m_db.rollbackTransaction(); m_lastError="恢复座位失败"; return false;
    }
    if (!m_db.commitTransaction()) {
        m_db.rollbackTransaction(); m_lastError="提交事务失败"; return false;
    }
    return true;
}

bool TicketManager::changeTicket(int orderId, int newTripId)
{
    auto oldOrder = m_db.findOrderById(orderId);
    if (!oldOrder) { m_lastError="订单不存在"; return false; }
    if (oldOrder->status != 0) { m_lastError="只能改签已预订的订单"; return false; }
    auto newTrip = m_db.findTripById(newTripId);
    if (!newTrip || !newTrip->enabled) { m_lastError="目标班次不可用"; return false; }
    if (newTrip->remainingSeats <= 0) { m_lastError="目标班次已无余票"; return false; }
    if (!m_db.beginTransaction()) { m_lastError="无法开启事务"; return false; }
    if (!m_db.updateOrderStatus(orderId, 2)) {
        m_db.rollbackTransaction(); m_lastError="更新旧订单失败"; return false;
    }
    if (!m_db.adjustTripSeats(oldOrder->tripId, +1)) {
        m_db.rollbackTransaction(); m_lastError="恢复旧座位失败"; return false;
    }
    if (!m_db.adjustTripSeats(newTripId, -1)) {
        m_db.rollbackTransaction(); m_lastError="扣减新座位失败"; return false;
    }
    OrderRecord no; no.userId=oldOrder->userId; no.tripId=newTripId;
    no.passengerName=oldOrder->passengerName; no.travelDate=newTrip->travelDate;
    no.purchaseTime=QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss");
    no.status=0;
    if (!m_db.createOrder(no)) {
        m_db.rollbackTransaction(); m_lastError="创建新订单失败"; return false;
    }
    if (!m_db.commitTransaction()) {
        m_db.rollbackTransaction(); m_lastError="提交事务失败"; return false;
    }
    return true;
}

static QVariantMap orderToMap(const OrderRecord &order)
{
    QVariantMap m;
    m["orderId"]=order.orderId; m["userId"]=order.userId;
    m["tripId"]=order.tripId; m["passengerName"]=order.passengerName;
    m["travelDate"]=order.travelDate; m["purchaseTime"]=order.purchaseTime;
    m["status"]=order.status;
    return m;
}

QVector<QVariantMap> TicketManager::queryOrdersByUser(int userId) const {
    QVector<QVariantMap> r;
    for (auto &o : m_db.findOrdersByUser(userId)) r.append(orderToMap(o));
    return r;
}
QVector<QVariantMap> TicketManager::queryOrdersByPassenger(const QString &n) const {
    QVector<QVariantMap> r;
    for (auto &o : m_db.findOrdersByPassenger(n)) r.append(orderToMap(o));
    return r;
}
QVector<QVariantMap> TicketManager::queryOrderByOrderId(int id) const {
    QVector<QVariantMap> r;
    auto o = m_db.findOrderById(id);
    if (o) r.append(orderToMap(*o));
    return r;
}

// ══════════════════════ Issue 11 ══════════════════════
QVector<QVariantMap> TicketManager::queryAllOrders() const {
    QVector<QVariantMap> r;
    for (auto &d : m_db.findAllOrdersWithDetails()) {
        QVariantMap m;
        m["orderId"]=d.orderId; m["userId"]=d.userId; m["tripId"]=d.tripId;
        m["trainId"]=d.trainId; m["status"]=d.status; m["trainNumber"]=d.trainNumber;
        m["passengerName"]=d.passengerName; m["purchaseTime"]=d.purchaseTime;
        m["departureStation"]=d.departureStationName;
        m["arrivalStation"]=d.arrivalStationName; m["travelDate"]=d.travelDate;
        r.append(m);
    }
    return r;
}

bool TicketManager::addOperationLog(const QString &op,const QString &act,const QString &det) {
    return m_db.addOperationLog(op,act,det);
}
