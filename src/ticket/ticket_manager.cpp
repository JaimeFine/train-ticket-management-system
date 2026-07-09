#include "ticket_manager.h"
#include "database_manager.h"
#include <QDateTime>

TicketManager::TicketManager(DatabaseManager &db) : m_db(db) {}

// ============================================ Issue 9 ============================================

int TicketManager::bookTicket(int userId, int trainId, const QString &passengerName)
{
    auto train = m_db.findTrainById(trainId);
    if (!train)            { m_lastError = "车次不存在";     return -1; }
    if (!train->enabled)   { m_lastError = "该车次已停运";   return -1; }
    if (train->remainingSeats <= 0) { m_lastError = "该车次已无余票"; return -1; }

    if (!m_db.beginTransaction()) { m_lastError = "无法开启事务"; return -1; }

    if (!m_db.adjustTrainSeats(trainId, -1)) {
        m_db.rollbackTransaction();
        m_lastError = "扣减座位失败";
        return -1;
    }

    OrderRecord order;
    order.userId        = userId;
    order.trainId       = trainId;
    order.passengerName = passengerName;
    order.purchaseTime  = QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss");
    order.status        = 0;

    if (!m_db.createOrder(order)) {
        m_db.rollbackTransaction();
        m_lastError = "创建订单失败";
        return -1;
    }

    if (!m_db.commitTransaction()) {
        m_db.rollbackTransaction();
        m_lastError = "提交事务失败";
        return -1;
    }

    auto orders = m_db.findOrdersByUser(userId);
    int orderId = orders.isEmpty() ? 0 : orders.last().orderId;
    return orderId > 0 ? orderId : -1;
}

int TicketManager::remainingSeats(int trainId) const
{
    auto train = m_db.findTrainById(trainId);
    return train ? train->remainingSeats : -1;
}

static QVariantMap trainToMap(const DatabaseManager::TrainWithStations &t)
{
    QVariantMap map;
    map["trainId"]          = t.trainId;
    map["trainNumber"]      = t.trainNumber;
    map["departureStation"] = t.departureStationName;
    map["arrivalStation"]   = t.arrivalStationName;
    map["departureTime"]    = t.departureTime;
    map["arrivalTime"]      = t.arrivalTime;
    map["remainingSeats"]   = t.remainingSeats;
    map["totalSeats"]       = t.totalSeats;
    return map;
}

QVector<QVariantMap> TicketManager::searchTrains(
    const QString &departure, const QString &arrival, const QString &date) const
{
    QVector<QVariantMap> result;
    auto trains = m_db.searchTrainsByStation(departure, arrival, date);
    for (const auto &t : trains) {
        result.append(trainToMap(t));
    }
    return result;
}

QVector<QVariantMap> TicketManager::searchByTrainNumber(const QString &number) const
{
    auto train = m_db.findTrainByNumber(number);
    if (!train || !train->enabled) return {};

    auto departureStation = m_db.findStationById(train->departureStationId);
    auto arrivalStation   = m_db.findStationById(train->arrivalStationId);

    QVariantMap map;
    map["trainId"]          = train->trainId;
    map["trainNumber"]      = train->trainNumber;
    map["departureStation"] = departureStation ? departureStation->stationName : "";
    map["arrivalStation"]   = arrivalStation   ? arrivalStation->stationName   : "";
    map["departureTime"]    = train->departureTime;
    map["arrivalTime"]      = train->arrivalTime;
    map["remainingSeats"]   = train->remainingSeats;
    map["totalSeats"]       = train->totalSeats;
    return { map };
}

// ============================================ Issue 10 ============================================

bool TicketManager::refundTicket(int orderId)
{
    auto order = m_db.findOrderById(orderId);
    if (!order)           { m_lastError = "订单不存在";  return false; }
    if (order->status != 0) {
        m_lastError = "该订单无法退票（已退票或已改签）";
        return false;
    }

    if (!m_db.beginTransaction()) { m_lastError = "无法开启事务"; return false; }

    if (!m_db.updateOrderStatus(orderId, 1)) {
        m_db.rollbackTransaction();
        m_lastError = "退票失败";
        return false;
    }

    if (!m_db.adjustTrainSeats(order->trainId, +1)) {
        m_db.rollbackTransaction();
        m_lastError = "恢复座位失败";
        return false;
    }

    if (!m_db.commitTransaction()) {
        m_db.rollbackTransaction();
        m_lastError = "提交事务失败";
        return false;
    }
    return true;
}

bool TicketManager::changeTicket(int orderId, int newTrainId)
{
    auto oldOrder = m_db.findOrderById(orderId);
    if (!oldOrder)           { m_lastError = "订单不存在";               return false; }
    if (oldOrder->status != 0) { m_lastError = "只能改签已预订的订单";  return false; }

    auto newTrain = m_db.findTrainById(newTrainId);
    if (!newTrain)               { m_lastError = "目标车次不存在";      return false; }
    if (!newTrain->enabled)      { m_lastError = "目标车次已停运";      return false; }
    if (newTrain->remainingSeats <= 0) { m_lastError = "目标车次已无余票"; return false; }

    if (!m_db.beginTransaction()) { m_lastError = "无法开启事务"; return false; }

    if (!m_db.updateOrderStatus(orderId, 2)) {
        m_db.rollbackTransaction(); m_lastError = "更新旧订单失败"; return false;
    }
    if (!m_db.adjustTrainSeats(oldOrder->trainId, +1)) {
        m_db.rollbackTransaction(); m_lastError = "恢复旧座位失败"; return false;
    }
    if (!m_db.adjustTrainSeats(newTrainId, -1)) {
        m_db.rollbackTransaction(); m_lastError = "扣减新座位失败"; return false;
    }

    OrderRecord newOrder;
    newOrder.userId        = oldOrder->userId;
    newOrder.trainId       = newTrainId;
    newOrder.passengerName = oldOrder->passengerName;
    newOrder.purchaseTime  = QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss");
    newOrder.status        = 0;

    if (!m_db.createOrder(newOrder)) {
        m_db.rollbackTransaction(); m_lastError = "创建新订单失败"; return false;
    }
    if (!m_db.commitTransaction()) {
        m_db.rollbackTransaction(); m_lastError = "提交事务失败"; return false;
    }
    return true;
}

static QVariantMap orderToMap(const OrderRecord &order)
{
    QVariantMap map;
    map["orderId"]       = order.orderId;
    map["userId"]        = order.userId;
    map["trainId"]       = order.trainId;
    map["passengerName"] = order.passengerName;
    map["purchaseTime"]  = order.purchaseTime;
    map["status"]        = order.status;
    return map;
}

QVector<QVariantMap> TicketManager::queryOrdersByUser(int userId) const
{
    QVector<QVariantMap> result;
    auto orders = m_db.findOrdersByUser(userId);
    for (const auto &order : orders) result.append(orderToMap(order));
    return result;
}

QVector<QVariantMap> TicketManager::queryOrdersByPassenger(const QString &name) const
{
    QVector<QVariantMap> result;
    auto orders = m_db.findOrdersByPassenger(name);
    for (const auto &order : orders) result.append(orderToMap(order));
    return result;
}

QVector<QVariantMap> TicketManager::queryOrderByOrderId(int orderId) const
{
    QVector<QVariantMap> result;
    auto order = m_db.findOrderById(orderId);
    if (order) result.append(orderToMap(*order));
    return result;
}

// ============================================ Issue 11 ============================================

QVector<QVariantMap> TicketManager::queryAllOrders() const
{
    QVector<QVariantMap> result;
    auto details = m_db.findAllOrdersWithDetails();
    for (const auto &d : details) {
        QVariantMap map;
        map["orderId"]          = d.orderId;
        map["userId"]           = d.userId;
        map["trainId"]          = d.trainId;
        map["status"]           = d.status;
        map["trainNumber"]      = d.trainNumber;
        map["passengerName"]    = d.passengerName;
        map["purchaseTime"]     = d.purchaseTime;
        map["departureStation"] = d.departureStationName;
        map["arrivalStation"]   = d.arrivalStationName;
        result.append(map);
    }
    return result;
}

QString TicketManager::lastError() const { return m_lastError; }
