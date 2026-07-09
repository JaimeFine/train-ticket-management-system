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
    if(!m_db.createOrder(o)){m_db.rollbackTransaction();m_lastError="创建订单失败";return -1;}
    if(!m_db.commitTransaction()){m_db.rollbackTransaction();m_lastError="提交事务失败";return -1;}
    auto orders=m_db.findOrdersByUser(userId);
    int id=orders.isEmpty()?0:orders.last().orderId;
    return id>0?id:-1;
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
bool TicketManager::refundTicket(int orderId) {
    auto o=m_db.findOrderById(orderId);
    if(!o){m_lastError="订单不存在";return false;}
    if(o->status!=0){m_lastError="该订单无法退票";return false;}
    if(!m_db.beginTransaction()){m_lastError="无法开启事务";return false;}
    if(!m_db.updateOrderStatus(orderId,1)){m_db.rollbackTransaction();m_lastError="退票失败";return false;}
    if(!m_db.adjustTrainSeats(o->trainId,+1)){m_db.rollbackTransaction();m_lastError="恢复座位失败";return false;}
    if(!m_db.commitTransaction()){m_db.rollbackTransaction();m_lastError="提交事务失败";return false;}
    return true;
}
bool TicketManager::changeTicket(int orderId, int newTrainId) {
    auto old=m_db.findOrderById(orderId);
    if(!old){m_lastError="订单不存在";return false;}
    if(old->status!=0){m_lastError="只能改签已预订的订单";return false;}
    auto nt=m_db.findTrainById(newTrainId);
    if(!nt){m_lastError="目标车次不存在";return false;}
    if(!nt->enabled){m_lastError="目标车次已停运";return false;}
    if(nt->remainingSeats<=0){m_lastError="目标车次已无余票";return false;}
    if(!m_db.beginTransaction()){m_lastError="无法开启事务";return false;}
    if(!m_db.updateOrderStatus(orderId,2)){m_db.rollbackTransaction();m_lastError="更新旧订单失败";return false;}
    if(!m_db.adjustTrainSeats(old->trainId,+1)){m_db.rollbackTransaction();m_lastError="恢复旧座位失败";return false;}
    if(!m_db.adjustTrainSeats(newTrainId,-1)){m_db.rollbackTransaction();m_lastError="扣减新座位失败";return false;}
    OrderRecord no;no.userId=old->userId;no.trainId=newTrainId;no.passengerName=old->passengerName;
    no.purchaseTime=QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss");no.status=0;
    if(!m_db.createOrder(no)){m_db.rollbackTransaction();m_lastError="创建新订单失败";return false;}
    if(!m_db.commitTransaction()){m_db.rollbackTransaction();m_lastError="提交事务失败";return false;}
    return true;
}
static QVariantMap ord2map(const OrderRecord &o){
    QVariantMap m;m["orderId"]=o.orderId;m["userId"]=o.userId;
    m["trainId"]=o.trainId;m["passengerName"]=o.passengerName;
    m["purchaseTime"]=o.purchaseTime;m["status"]=o.status;return m;
}
QVector<QVariantMap> TicketManager::queryOrdersByUser(int userId) const {
    QVector<QVariantMap> r;
    for(auto &o:m_db.findOrdersByUser(userId))r.append(ord2map(o));
    return r;
}
QVector<QVariantMap> TicketManager::queryOrdersByPassenger(const QString &name) const {
    QVector<QVariantMap> r;
    for(auto &o:m_db.findOrdersByPassenger(name))r.append(ord2map(o));
    return r;
}
QVector<QVariantMap> TicketManager::queryOrderByOrderId(int orderId) const {
    QVector<QVariantMap> r;auto o=m_db.findOrderById(orderId);
    if(o)r.append(ord2map(*o));return r;
}
