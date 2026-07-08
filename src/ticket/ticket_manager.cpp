#include "ticket_manager.h"
#include "database_manager.h"

#include <QDebug>

TicketManager::TicketManager(DatabaseManager &db) : m_db(db) {}

// ══════════════════════════════════════════════════════════════════════════════
// 订票
// ══════════════════════════════════════════════════════════════════════════════

int TicketManager::bookTicket(int userId, int trainId, const QString &passengerName)
{
    auto train = m_db.findTrainById(trainId);
    if (!train) { m_lastError = "车次不存在"; return -1; }
    if (!train->enabled) { m_lastError = "该车次已停运"; return -1; }
    if (train->remainingSeats <= 0) { m_lastError = "该车次已无余票"; return -1; }

    if (!m_db.adjustTrainSeats(trainId, -1)) {
        m_lastError = "扣减座位失败: " + m_db.lastError(); return -1;
    }

    OrderRecord order;
    order.userId = userId;
    order.trainId = trainId;
    order.passengerName = passengerName;
    order.purchaseTime = QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss");
    order.status = 0; // Booked

    if (!m_db.createOrder(order)) {
        m_db.adjustTrainSeats(trainId, +1); // rollback seat
        m_lastError = "创建订单失败: " + m_db.lastError(); return -1;
    }

    // Get the last inserted orderId — find the most recent order for this user
    auto orders = m_db.findOrdersByUser(userId);
    int orderId = orders.isEmpty() ? 0 : orders.last().orderId;

    qDebug() << "TicketManager: booked order" << orderId << "for" << passengerName;
    return orderId > 0 ? orderId : -1;
}

// ══════════════════════════════════════════════════════════════════════════════
// 退票
// ══════════════════════════════════════════════════════════════════════════════

bool TicketManager::refundTicket(int orderId)
{
    auto order = m_db.findOrderById(orderId);
    if (!order) { m_lastError = "订单不存在"; return false; }
    if (order->status != 0) { m_lastError = "该订单无法退票（已退票或已改签）"; return false; }

    if (!m_db.updateOrderStatus(orderId, 1)) {
        m_lastError = "退票失败: " + m_db.lastError(); return false;
    }
    m_db.adjustTrainSeats(order->trainId, +1);

    qDebug() << "TicketManager: refunded order" << orderId;
    return true;
}

// ══════════════════════════════════════════════════════════════════════════════
// 改签
// ══════════════════════════════════════════════════════════════════════════════

bool TicketManager::changeTicket(int orderId, int newTrainId)
{
    auto oldOrder = m_db.findOrderById(orderId);
    if (!oldOrder) { m_lastError = "订单不存在"; return false; }
    if (oldOrder->status != 0) { m_lastError = "只能改签已预订的订单"; return false; }

    auto newTrain = m_db.findTrainById(newTrainId);
    if (!newTrain) { m_lastError = "目标车次不存在"; return false; }
    if (newTrain->remainingSeats <= 0) { m_lastError = "目标车次已无余票"; return false; }

    // Mark old order as changed
    m_db.updateOrderStatus(orderId, 2);
    m_db.adjustTrainSeats(oldOrder->trainId, +1);

    // Create new order
    if (!m_db.adjustTrainSeats(newTrainId, -1)) {
        // Rollback: restore old state
        m_db.updateOrderStatus(orderId, 0);
        m_db.adjustTrainSeats(oldOrder->trainId, -1);
        m_lastError = "目标车次扣座失败"; return false;
    }

    OrderRecord newOrder;
    newOrder.userId = oldOrder->userId;
    newOrder.trainId = newTrainId;
    newOrder.passengerName = oldOrder->passengerName;
    newOrder.purchaseTime = QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss");
    newOrder.status = 0;

    if (!m_db.createOrder(newOrder)) {
        m_db.adjustTrainSeats(newTrainId, +1);
        m_db.updateOrderStatus(orderId, 0);
        m_db.adjustTrainSeats(oldOrder->trainId, -1);
        m_lastError = "创建新订单失败"; return false;
    }

    qDebug() << "TicketManager: changed order" << orderId << "to train" << newTrainId;
    return true;
}

// ══════════════════════════════════════════════════════════════════════════════
// 余票查询
// ══════════════════════════════════════════════════════════════════════════════

int TicketManager::remainingSeats(int trainId) const
{
    auto train = m_db.findTrainById(trainId);
    return train ? train->remainingSeats : -1;
}

// ══════════════════════════════════════════════════════════════════════════════
// 订单查询 — 适配为新 API，返回 QVariantMap 保持 UI 兼容
// ══════════════════════════════════════════════════════════════════════════════

static QVariantMap orderToMap(const OrderRecord &o) {
    QVariantMap m;
    m["orderId"] = o.orderId; m["userId"] = o.userId;
    m["trainId"] = o.trainId; m["passengerName"] = o.passengerName;
    m["purchaseTime"] = o.purchaseTime; m["status"] = o.status;
    return m;
}

QVector<QVariantMap> TicketManager::queryOrdersByUser(int userId) const
{
    QVector<QVariantMap> result;
    for (auto &o : m_db.findOrdersByUser(userId))
        result.append(orderToMap(o));
    return result;
}

QVector<QVariantMap> TicketManager::queryOrdersByPassenger(const QString &name) const
{
    QVector<QVariantMap> result;
    for (auto &o : m_db.findOrdersByPassenger(name))
        result.append(orderToMap(o));
    return result;
}

QVector<QVariantMap> TicketManager::queryOrderByOrderId(int orderId) const
{
    QVector<QVariantMap> result;
    auto o = m_db.findOrderById(orderId);
    if (o) result.append(orderToMap(*o));
    return result;
}

QVector<QVariantMap> TicketManager::queryAllOrders() const
{
    QVector<QVariantMap> result;
    for (auto &d : m_db.findAllOrdersWithDetails()) {
        QVariantMap m;
        m["orderId"] = d.orderId; m["userId"] = d.userId;
        m["trainId"] = d.trainId; m["status"] = d.status;
        m["trainNumber"] = d.trainNumber; m["passengerName"] = d.passengerName;
        m["purchaseTime"] = d.purchaseTime;
        m["departureStation"] = d.departureStationName;
        m["arrivalStation"] = d.arrivalStationName;
        result.append(m);
    }
    return result;
}

// ══════════════════════════════════════════════════════════════════════════════
// 车次搜索
// ══════════════════════════════════════════════════════════════════════════════

static QVariantMap trainWithStationsToMap(const DatabaseManager::TrainWithStations &t) {
    QVariantMap m;
    m["trainId"] = t.trainId; m["trainNumber"] = t.trainNumber;
    m["departureStation"] = t.departureStationName;
    m["arrivalStation"] = t.arrivalStationName;
    m["departureTime"] = t.departureTime; m["arrivalTime"] = t.arrivalTime;
    m["remainingSeats"] = t.remainingSeats; m["totalSeats"] = t.totalSeats;
    return m;
}

QVector<QVariantMap> TicketManager::searchTrains(
    const QString &dep, const QString &arr, const QString &date) const
{
    QVector<QVariantMap> result;
    for (auto &t : m_db.searchTrainsByStation(dep, arr, date))
        result.append(trainWithStationsToMap(t));
    return result;
}

QVector<QVariantMap> TicketManager::searchByTrainNumber(const QString &number) const
{
    auto t = m_db.findTrainByNumber(number);
    if (!t || !t->enabled) return {};

    // Get station names
    auto depStation = m_db.findStationById(t->departureStationId);
    auto arrStation = m_db.findStationById(t->arrivalStationId);

    QVariantMap m;
    m["trainId"] = t->trainId; m["trainNumber"] = t->trainNumber;
    m["departureStation"] = depStation ? depStation->stationName : "";
    m["arrivalStation"] = arrStation ? arrStation->stationName : "";
    m["departureTime"] = t->departureTime; m["arrivalTime"] = t->arrivalTime;
    m["remainingSeats"] = t->remainingSeats; m["totalSeats"] = t->totalSeats;
    return {m};
}

QString TicketManager::lastError() const { return m_lastError; }
