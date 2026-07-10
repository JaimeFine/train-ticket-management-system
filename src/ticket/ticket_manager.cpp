#include "ticket_manager.h"
#include "database_manager.h"
#include <QDateTime>

TicketManager::TicketManager(DatabaseManager &db) : m_db(db) {}

// ══════════════════════ Issue 9: 订票 + 余票查询 + 车次搜索 ══════════════════════
int TicketManager::bookTicket(int userId, int trainId, const QString &passengerName) {
    auto t=m_db.findTrainById(trainId);
    if(!t){m_lastError="车次不存在";return -1;}
    if(!t->enabled){m_lastError="该车次已停运";return -1;}
    if(t->remainingSeats<=0){m_lastError="该车次已无余票";return -1;}
    if(!m_db.beginTransaction()){m_lastError="无法开启事务";return -1;}
    if(!m_db.adjustTrainSeats(trainId,-1)){m_db.rollbackTransaction();m_lastError="扣减座位失败";return -1;}
    OrderRecord o; o.userId=userId; o.trainId=trainId; o.passengerName=passengerName;
    o.purchaseTime=QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss"); o.status=0;
    // >>> Jaime fix:
    const auto orderId = m_db.createOrder(o);
    if (!orderId.has_value()) {
        m_db.rollbackTransaction();
        m_lastError = "创建订单失败";
        return -1;
    }

    if (!m_db.commitTransaction()) {
        m_db.rollbackTransaction();
        m_lastError = "提交事务失败";
        return -1;
    }

    return *orderId;
}
int TicketManager::remainingSeats(int trainId) const {
    auto t=m_db.findTrainById(trainId); return t?t->remainingSeats:-1;
}
static QVariantMap train2map(const DatabaseManager::TrainWithStations &t){
    QVariantMap m;m["trainId"]=t.trainId;m["trainNumber"]=t.trainNumber;
    m["departureStation"]=t.departureStationName;m["arrivalStation"]=t.arrivalStationName;
    m["departureTime"]=t.departureTime;m["arrivalTime"]=t.arrivalTime;
    m["remainingSeats"]=t.remainingSeats;m["totalSeats"]=t.totalSeats;return m;
}
QVector<QVariantMap> TicketManager::searchTrains(const QString &dep,const QString &arr,const QString &date) const {
    QVector<QVariantMap> r;
    for(auto &t:m_db.searchTrainsByStation(dep,arr,date))r.append(train2map(t));
    return r;
}
QVector<QVariantMap> TicketManager::searchByTrainNumber(const QString &num) const {
    auto t=m_db.findTrainByNumber(num);
    if(!t||!t->enabled)return{};
    auto ds=m_db.findStationById(t->departureStationId);
    auto as=m_db.findStationById(t->arrivalStationId);
    QVariantMap m;m["trainId"]=t->trainId;m["trainNumber"]=t->trainNumber;
    m["departureStation"]=ds?ds->stationName:"";m["arrivalStation"]=as?as->stationName:"";
    m["departureTime"]=t->departureTime;m["arrivalTime"]=t->arrivalTime;
    m["remainingSeats"]=t->remainingSeats;m["totalSeats"]=t->totalSeats;
    return {m};
}
QString TicketManager::lastError() const { return m_lastError; }

// ══════════════════════ Issue 10: 退票 + 改签 + 订单查询 ══════════════════════

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
