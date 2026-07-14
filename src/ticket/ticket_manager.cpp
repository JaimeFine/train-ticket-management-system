#include "ticket_manager.h"
#include "database_manager.h"
#include <QDate>
#include <QDateTime>
#include <QTime>

#include <algorithm>
#include <cmath>

namespace {
QDateTime readDateTime(const QString &text)
{
    QDateTime value = QDateTime::fromString(text, QStringLiteral("yyyy-MM-dd HH:mm:ss"));
    if (!value.isValid()) {
        value = QDateTime::fromString(text, Qt::ISODate);
    }
    return value;
}

QTime readTime(const QString &text)
{
    QTime value = QTime::fromString(text, QStringLiteral("HH:mm:ss"));
    if (!value.isValid()) {
        value = QTime::fromString(text, QStringLiteral("HH:mm"));
    }
    return value;
}

int calculateTravelMinutes(const QString &departureTime, const QString &arrivalTime)
{
    const QDateTime departure = readDateTime(departureTime);
    const QDateTime arrival = readDateTime(arrivalTime);
    if (departure.isValid() && arrival.isValid()) {
        const qint64 minutes = departure.secsTo(arrival) / 60;
        return minutes > 0 ? static_cast<int>(minutes) : 0;
    }

    const QTime departureClock = readTime(departureTime);
    const QTime arrivalClock = readTime(arrivalTime);
    if (!departureClock.isValid() || !arrivalClock.isValid()) {
        return 0;
    }

    int seconds = departureClock.secsTo(arrivalClock);
    if (seconds <= 0) {
        seconds += 24 * 60 * 60;
    }
    return seconds / 60;
}

struct FareProfile
{
    double averageSpeed = 100.0;
    double pricePerKilometer = 0.15;
};

FareProfile fareProfileForTrain(const QString &trainNumber)
{
    const QString number = trainNumber.trimmed().toUpper();
    if (number.startsWith(QLatin1Char('G'))) {
        return {240.0, 0.46};
    }
    if (number.startsWith(QLatin1Char('D'))
        || number.startsWith(QLatin1Char('C'))) {
        return {180.0, 0.31};
    }
    return {100.0, 0.15};
}

double calculateBasePrice(const DatabaseManager::TrainWithStations &train,
                          int travelMinutes)
{
    if (travelMinutes <= 0) {
        return 0.0;
    }

    // 数据库暂时没有线路里程，所以先按车次类型估算平均速度，
    // 再用运行时间估算里程。这里的费率是课程项目模拟值，不代表12306实际报价。
    const FareProfile profile = fareProfileForTrain(train.trainNumber);
    const double travelHours = travelMinutes / 60.0;
    const double estimatedDistance = travelHours * profile.averageSpeed;
    const double basePrice = std::max(20.0,
                                      estimatedDistance * profile.pricePerKilometer);
    return std::round(basePrice * 10.0) / 10.0;
}

double calculateDynamicPrice(const DatabaseManager::TrainWithStations &train,
                             int travelMinutes)
{
    const double basePrice = calculateBasePrice(train, travelMinutes);
    if (basePrice <= 0.0) {
        return 0.0;
    }

    // 同一趟行程余票越少，价格适当提高。
    double seatFactor = 1.0;
    if (train.totalSeats > 0) {
        const double soldRate = 1.0 - static_cast<double>(train.remainingSeats)
                                          / train.totalSeats;
        if (soldRate >= 0.8) {
            seatFactor = 1.25;
        } else if (soldRate >= 0.5) {
            seatFactor = 1.12;
        }
    }

    // 早晚通勤高峰的需求较大，增加一个小幅高峰系数。
    double timeFactor = 1.0;
    const QDateTime departure = readDateTime(train.departureTime);
    const QTime departureClock = departure.isValid()
                                     ? departure.time()
                                     : readTime(train.departureTime);
    if (departureClock.isValid()) {
        const int hour = departureClock.hour();
        if ((hour >= 7 && hour < 10) || (hour >= 17 && hour < 20)) {
            timeFactor = 1.10;
        }
    }

    // 临近出发时乘客选择更少，因此当天和次日车次会稍贵一些。
    double dateFactor = 1.0;
    if (departure.isValid()) {
        const int daysBeforeTravel = QDate::currentDate().daysTo(departure.date());
        if (daysBeforeTravel >= 0 && daysBeforeTravel <= 1) {
            dateFactor = 1.15;
        } else if (daysBeforeTravel <= 3 && daysBeforeTravel > 1) {
            dateFactor = 1.08;
        }
    }

    const double result = basePrice * seatFactor * timeFactor * dateFactor;
    return std::round(result * 10.0) / 10.0;
}

QVariantMap trainToMap(const DatabaseManager::TrainWithStations &train)
{
    QVariantMap map;
    map[QStringLiteral("trainId")] = train.trainId;
    map[QStringLiteral("trainNumber")] = train.trainNumber;
    map[QStringLiteral("departureStation")] = train.departureStationName;
    map[QStringLiteral("arrivalStation")] = train.arrivalStationName;
    map[QStringLiteral("departureTime")] = train.departureTime;
    map[QStringLiteral("arrivalTime")] = train.arrivalTime;
    map[QStringLiteral("remainingSeats")] = train.remainingSeats;
    map[QStringLiteral("totalSeats")] = train.totalSeats;

    const int travelMinutes = calculateTravelMinutes(train.departureTime,
                                                     train.arrivalTime);
    map[QStringLiteral("travelMinutes")] = travelMinutes;
    map[QStringLiteral("basePrice")] = calculateBasePrice(train, travelMinutes);
    map[QStringLiteral("dynamicPrice")] = calculateDynamicPrice(train, travelMinutes);
    return map;
}
}

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
QVector<QVariantMap> TicketManager::searchTrains(const QString &dep,const QString &arr,const QString &date) const {
    QVector<QVariantMap> r;
    for (const auto &train : m_db.searchTrainsByStation(dep, arr, date)) {
        r.append(trainToMap(train));
    }
    return r;
}
QVector<QVariantMap> TicketManager::searchByTrainNumber(const QString &num) const {
    auto t=m_db.findTrainByNumber(num);
    if(!t||!t->enabled)return{};
    auto ds=m_db.findStationById(t->departureStationId);
    auto as=m_db.findStationById(t->arrivalStationId);

    DatabaseManager::TrainWithStations train;
    train.trainId = t->trainId;
    train.trainNumber = t->trainNumber;
    train.departureStationName = ds ? ds->stationName : QString();
    train.arrivalStationName = as ? as->stationName : QString();
    train.departureTime = t->departureTime;
    train.arrivalTime = t->arrivalTime;
    train.remainingSeats = t->remainingSeats;
    train.totalSeats = t->totalSeats;
    train.enabled = t->enabled;
    return {trainToMap(train)};
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

bool TicketManager::addOperationLog(const QString &operatorUsername,
                                    const QString &action,
                                    const QString &detail)
{
    return m_db.addOperationLog(operatorUsername, action, detail);
}
