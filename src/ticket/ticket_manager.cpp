#include "ticket_manager.h"
#include "database_manager.h"

#include <QDate>
#include <QDateTime>
#include <QTime>

#include <algorithm>
#include <cmath>

namespace {
struct HolidayPeriod
{
    QDate startDate;
    QDate endDate;
    double factor = 1.0;
};

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
    // 数据库中既可能保存完整日期时间，也可能只保存 HH:mm。
    // 先按完整时间计算，失败后再退回到时刻计算，兼容两种已有数据。
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
        // 到达时刻小于出发时刻时，按跨天车次处理。
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

double estimateBasePrice(const QString &trainNumber, int travelMinutes)
{
    if (travelMinutes <= 0) {
        return 0.0;
    }

    const FareProfile profile = fareProfileForTrain(trainNumber);
    const double travelHours = travelMinutes / 60.0;
    const double estimatedDistance = travelHours * profile.averageSpeed;
    const double basePrice = std::max(20.0,
                                      estimatedDistance * profile.pricePerKilometer);
    return std::round(basePrice);
}

double calculateBasePrice(const DatabaseManager::TrainWithStations &trip, int travelMinutes)
{
    // Trip.basePrice 是正式基础票价，动态票价优先使用数据库中的值。
    // 旧数据库中的基础票价可能还是 0，此时才根据车型和时长估算，
    // 这样旧数据仍能显示票价，也给数据库模块保留了正式接入位置。
    if (trip.basePrice > 0.0) {
        return trip.basePrice;
    }

    return estimateBasePrice(trip.trainNumber, travelMinutes);
}

double holidayDemandFactor(const QDate &travelDate)
{
    if (!travelDate.isValid()) {
        return 1.0;
    }

    // 2026 年日期按照国务院办公厅公布的放假安排填写。
    // 以后更新年份时只需要继续补充这个数组，不用改票价计算过程。
    static const HolidayPeriod holidayPeriods[] = {
        {QDate(2026, 1, 1), QDate(2026, 1, 3), 1.10},
        {QDate(2026, 2, 15), QDate(2026, 2, 23), 1.20},
        {QDate(2026, 4, 4), QDate(2026, 4, 6), 1.10},
        {QDate(2026, 5, 1), QDate(2026, 5, 5), 1.15},
        {QDate(2026, 6, 19), QDate(2026, 6, 21), 1.10},
        {QDate(2026, 9, 25), QDate(2026, 9, 27), 1.12},
        {QDate(2026, 10, 1), QDate(2026, 10, 7), 1.20}
    };

    for (const HolidayPeriod &period : holidayPeriods) {
        if (travelDate >= period.startDate && travelDate <= period.endDate) {
            return period.factor;
        }
    }

    // 调休上班日虽然在周末，但不按周末客流计算。
    static const QDate workingWeekends[] = {
        QDate(2026, 1, 4), QDate(2026, 2, 14), QDate(2026, 2, 28),
        QDate(2026, 5, 9), QDate(2026, 9, 20), QDate(2026, 10, 10)
    };
    for (const QDate &workingDate : workingWeekends) {
        if (travelDate == workingDate) {
            return 1.0;
        }
    }

    if (travelDate.dayOfWeek() == Qt::Saturday
        || travelDate.dayOfWeek() == Qt::Sunday) {
        return 1.05;
    }
    return 1.0;
}

double calculateDynamicPrice(const DatabaseManager::TrainWithStations &trip,
                             int travelMinutes)
{
    // 计算顺序为：基础价 × 余票系数 × 时段系数 × 日期系数 × 临近发车系数。
    // 各部分分开计算，后续调整某一项规则时不会影响其他规则。
    const double basePrice = calculateBasePrice(trip, travelMinutes);
    if (basePrice <= 0.0) {
        return 0.0;
    }

    double seatFactor = 1.0;
    double remainingRate = 0.0;
    if (trip.totalSeats > 0) {
        remainingRate = std::clamp(static_cast<double>(trip.remainingSeats)
                                       / trip.totalSeats,
                                   0.0,
                                   1.0);
        const double soldRate = 1.0 - remainingRate;
        // 余票越少，说明当前班次需求越高。这里只分成三档，
        // 避免余票每变化一张，界面上的价格就频繁跳动。
        if (soldRate >= 0.8) {
            seatFactor = 1.25;
        } else if (soldRate >= 0.5) {
            seatFactor = 1.12;
        }
    }

    double timeFactor = 1.0;
    const QTime departureClock = readTime(trip.departureTime);
    if (departureClock.isValid()) {
        const int hour = departureClock.hour();
        // 早晚通勤时段使用同一档系数，普通时段保持原价。
        if ((hour >= 7 && hour < 10) || (hour >= 17 && hour < 20)) {
            timeFactor = 1.10;
        }
    }

    const QDate travelDate = QDate::fromString(trip.travelDate, QStringLiteral("yyyy-MM-dd"));
    const double holidayFactor = holidayDemandFactor(travelDate);

    QDateTime departure;
    if (travelDate.isValid() && departureClock.isValid()) {
        // Trip 将出行日期和发车时刻分开保存，计算临近发车时间前需要先合并。
        departure = QDateTime(travelDate, departureClock);
    } else {
        departure = readDateTime(trip.departureTime);
    }

    // 临近发车但余票仍超过六成时，用分档折扣促进余票销售。
    // 节假日需求系数仍会参与计算，最终票价最低保留基础票价的七成。
    double lastMinuteFactor = 1.0;
    if (departure.isValid()) {
        const qint64 secondsBeforeDeparture =
            QDateTime::currentDateTime().secsTo(departure);
        const double hoursBeforeDeparture = secondsBeforeDeparture / 3600.0;
        const bool canDiscount = remainingRate >= 0.60;

        if (canDiscount && hoursBeforeDeparture >= 0.0) {
            if (hoursBeforeDeparture <= 6.0) {
                lastMinuteFactor = 0.75;
            } else if (hoursBeforeDeparture <= 24.0) {
                lastMinuteFactor = 0.84;
            } else if (hoursBeforeDeparture <= 48.0) {
                lastMinuteFactor = 0.92;
            }
        }
    }

    const double result = basePrice * seatFactor * timeFactor
                          * holidayFactor * lastMinuteFactor;
    // 多个折扣叠加后仍不能低于基础票价的七成。
    const double protectedPrice = std::max(basePrice * 0.70, result);
    return std::round(protectedPrice);
}

QVariantMap tripToMap(const DatabaseManager::TrainWithStations &trip)
{
    // Manager 在这里把数据库记录整理成 UI 需要的字段。
    // 界面只读取 basePrice 和 dynamicPrice，不接触数据库，也不重复票价算法。
    QVariantMap map;
    map[QStringLiteral("trainId")] = trip.trainId;
    map[QStringLiteral("tripId")] = trip.tripId;
    map[QStringLiteral("trainNumber")] = trip.trainNumber;
    map[QStringLiteral("travelDate")] = trip.travelDate;
    map[QStringLiteral("departureStation")] = trip.departureStationName;
    map[QStringLiteral("arrivalStation")] = trip.arrivalStationName;
    map[QStringLiteral("departureTime")] = trip.departureTime;
    map[QStringLiteral("arrivalTime")] = trip.arrivalTime;
    map[QStringLiteral("remainingSeats")] = trip.remainingSeats;
    map[QStringLiteral("totalSeats")] = trip.totalSeats;

    const int travelMinutes = calculateTravelMinutes(trip.departureTime,
                                                     trip.arrivalTime);
    map[QStringLiteral("travelMinutes")] = travelMinutes;
    map[QStringLiteral("basePrice")] = calculateBasePrice(trip, travelMinutes);
    map[QStringLiteral("dynamicPrice")] = calculateDynamicPrice(trip, travelMinutes);
    return map;
}
}

// 构造函数：保存数据库管理器引用，所有数据操作都经由它完成
TicketManager::TicketManager(DatabaseManager &db) : m_db(db) {}

int TicketManager::bookTicket(int userId, int tripId, const QString &passengerName)
{
    const auto trip = m_db.findTripById(tripId);
    if (!trip.has_value()) {
        m_lastError = QStringLiteral("班次不存在");
        return -1;
    }
    // 先检查 trip.enabled：班次停运后，直接禁止继续售票。
    if (!trip->enabled) {
        m_lastError = QStringLiteral("该班次已停运");
        return -1;
    }
    if (trip->remainingSeats <= 0) {
        m_lastError = QStringLiteral("该班次已无余票");
        return -1;
    }

    // 购票前要同时确认班次和所属车次都处于启用状态。
    const auto train = m_db.findTrainById(trip->trainId);
    if (!train.has_value() || !train->enabled) {
        m_lastError = QStringLiteral("该车次已停运");
        return -1;
    }

    // 扣座和建单放在同一事务：任一步失败整体回滚，防止超卖或丢单
    if (!m_db.beginTransaction()) {
        m_lastError = QStringLiteral("无法开启事务");
        return -1;
    }
    if (!m_db.adjustTripSeats(tripId, -1)) {
        m_db.rollbackTransaction();
        m_lastError = QStringLiteral("扣减座位失败");
        return -1;
    }

    OrderRecord order;
    order.userId = userId;
    order.trainId = trip->trainId;
    order.tripId = tripId;
    order.passengerName = passengerName;
    order.travelDate = trip->travelDate;
    order.purchaseTime = QDateTime::currentDateTime()
                             .toString(QStringLiteral("yyyy-MM-dd HH:mm:ss"));
    order.price = trip->basePrice;
    order.status = 0;  // 状态0=已预订

    const auto orderId = m_db.createOrder(order);
    if (!orderId.has_value()) {
        m_db.rollbackTransaction();
        m_lastError = QStringLiteral("创建订单失败");
        return -1;
    }

    if (!m_db.commitTransaction()) {
        m_db.rollbackTransaction();
        m_lastError = QStringLiteral("提交事务失败");
        return -1;
    }

    return *orderId;
}

int TicketManager::remainingSeats(int tripId) const
{
    // 查询指定班次的余票数；班次不存在时返回-1
    const auto trip = m_db.findTripById(tripId);
    return trip.has_value() ? trip->remainingSeats : -1;
}

QVector<QVariantMap> TicketManager::searchTrips(const QString &dep,
                                                const QString &arr,
                                                const QString &date) const
{
    // 按出发站/到达站（可选出行日期）模糊搜索班次
    QVector<QVariantMap> results;
    for (const auto &trip : m_db.searchTripsByStation(dep, arr, date)) {
        results.append(tripToMap(trip));
    }
    return results;
}

QVector<QVariantMap> TicketManager::searchByTrainNumber(const QString &number) const
{
    // 按车次号精确查询，返回该车次下所有班次；车次不存在或停运时返回空
    const auto train = m_db.findTrainByNumber(number);
    if (!train.has_value() || !train->enabled) {
        return {};
    }

    const auto departureStation = m_db.findStationById(train->departureStationId);
    const auto arrivalStation = m_db.findStationById(train->arrivalStationId);
    const auto trips = m_db.findTripsByTrain(train->trainId);

    QVector<QVariantMap> results;
    for (const auto &tripRecord : trips) {
        if (!tripRecord.enabled) {
            continue;
        }

        DatabaseManager::TrainWithStations trip;
        trip.trainId = train->trainId;
        trip.tripId = tripRecord.tripId;
        trip.trainNumber = train->trainNumber;
        trip.travelDate = tripRecord.travelDate;
        trip.departureStationName = departureStation
                                        ? departureStation->stationName
                                        : QString();
        trip.arrivalStationName = arrivalStation
                                      ? arrivalStation->stationName
                                      : QString();
        trip.departureTime = tripRecord.departureTime;
        trip.arrivalTime = tripRecord.arrivalTime;
        trip.totalSeats = tripRecord.totalSeats;
        trip.remainingSeats = tripRecord.remainingSeats;
        trip.basePrice = tripRecord.basePrice;
        trip.enabled = tripRecord.enabled;
        results.append(tripToMap(trip));
    }

    return results;
}

QString TicketManager::lastError() const
{
    // 返回最近一次操作失败的错误信息
    return m_lastError;
}

bool TicketManager::refundTicket(int orderId)
{
    // 退票：订单状态改为1=已退票，并回补班次座位；事务保证状态与座位一致
    const auto order = m_db.findOrderById(orderId);
    if (!order.has_value()) {
        m_lastError = QStringLiteral("订单不存在");
        return false;
    }
    // 仅状态0(已预订)或3的订单可退，已退票/已改签的旧单不能重复退
    if (order->status != 0 && order->status != 3) {
        m_lastError = QStringLiteral("该订单无法退票（已退票或已改签）");
        return false;
    }

    if (!m_db.beginTransaction()) {
        m_lastError = QStringLiteral("无法开启事务");
        return false;
    }

    if (!m_db.updateOrderStatus(orderId, 1)) {
        m_db.rollbackTransaction();
        m_lastError = QStringLiteral("退票失败");
        return false;
    }

    // 回补座位：退票后余票+1，须与状态更新同事务
    if (!m_db.adjustTripSeats(order->tripId, +1)) {
        m_db.rollbackTransaction();
        m_lastError = QStringLiteral("恢复座位失败");
        return false;
    }

    if (!m_db.commitTransaction()) {
        m_db.rollbackTransaction();
        m_lastError = QStringLiteral("提交事务失败");
        return false;
    }

    return true;
}

// 改签：旧订单标记为2=已改签、回补旧班次座位，再扣新班次座位并生成新订单（同一事务）
bool TicketManager::changeTicket(int orderId, int newTripId)
{
    const auto oldOrder = m_db.findOrderById(orderId);
    if (!oldOrder.has_value()) {
        m_lastError = QStringLiteral("订单不存在");
        return false;
    }
    if (oldOrder->status != 0) {
        m_lastError = QStringLiteral("只能改签已预订的订单");
        return false;
    }

    const auto newTrip = m_db.findTripById(newTripId);
    if (!newTrip.has_value() || !newTrip->enabled) {
        m_lastError = QStringLiteral("目标班次不可用");
        return false;
    }
    if (newTrip->remainingSeats <= 0) {
        m_lastError = QStringLiteral("目标班次已无余票");
        return false;
    }

    if (!m_db.beginTransaction()) {
        m_lastError = QStringLiteral("无法开启事务");
        return false;
    }

    // 旧单置为2=已改签（保留记录不删除），便于追溯
    if (!m_db.updateOrderStatus(orderId, 2)) {
        m_db.rollbackTransaction();
        m_lastError = QStringLiteral("更新旧订单失败");
        return false;
    }
    // 座位转移：旧班次+1、新班次-1，任一失败全部回滚，保证不超卖不丢座
    // 座位转移：旧班次+1、新班次-1，任一失败全部回滚，保证不超卖不丢座
    if (!m_db.adjustTripSeats(oldOrder->tripId, +1)) {
        m_db.rollbackTransaction();
        m_lastError = QStringLiteral("恢复旧座位失败");
        return false;
    }
    if (!m_db.adjustTripSeats(newTripId, -1)) {
        m_db.rollbackTransaction();
        m_lastError = QStringLiteral("扣减新座位失败");
        return false;
    }

    // 用新班次信息生成一张新订单，乘客信息沿用旧单
    OrderRecord newOrder;
    newOrder.userId = oldOrder->userId;
    newOrder.trainId = newTrip->trainId;
    newOrder.tripId = newTripId;
    newOrder.passengerName = oldOrder->passengerName;
    newOrder.travelDate = newTrip->travelDate;
    newOrder.purchaseTime = QDateTime::currentDateTime()
                                .toString(QStringLiteral("yyyy-MM-dd HH:mm:ss"));
    newOrder.price = newTrip->basePrice;   // 新单按新班次票价结算
    newOrder.status = 0;  // 新单状态0=已预订

    if (!m_db.createOrder(newOrder).has_value()) {
        m_db.rollbackTransaction();
        m_lastError = QStringLiteral("创建新订单失败");
        return false;
    }
    if (!m_db.commitTransaction()) {
        m_db.rollbackTransaction();
        m_lastError = QStringLiteral("提交事务失败");
        return false;
    }

    return true;
}

// 把订单记录转成QVariantMap，供UI展示
static QVariantMap orderToMap(const OrderRecord &order)
{
    // 把订单记录转成 QVariantMap，供 UI 展示
    QVariantMap map;
    map[QStringLiteral("orderId")] = order.orderId;
    map[QStringLiteral("userId")] = order.userId;
    map[QStringLiteral("trainId")] = order.trainId;
    map[QStringLiteral("tripId")] = order.tripId;
    map[QStringLiteral("passengerName")] = order.passengerName;
    map[QStringLiteral("travelDate")] = order.travelDate;
    map[QStringLiteral("purchaseTime")] = order.purchaseTime;
    map[QStringLiteral("status")] = order.status;
    return map;
}

QVector<QVariantMap> TicketManager::queryOrdersByUser(int userId) const
{
    // 按用户ID查询其全部订单
    QVector<QVariantMap> results;
    const auto orders = m_db.findOrdersByUser(userId);
    for (const auto &order : orders) {
        results.append(orderToMap(order));
    }
    return results;
}

QVector<QVariantMap> TicketManager::queryOrdersByPassenger(const QString &name) const
{
    // 按乘客姓名查询订单
    QVector<QVariantMap> results;
    const auto orders = m_db.findOrdersByPassenger(name);
    for (const auto &order : orders) {
        results.append(orderToMap(order));
    }
    return results;
}

QVector<QVariantMap> TicketManager::queryOrderByOrderId(int orderId) const
{
    // 按订单号精确查询，最多返回一条
    QVector<QVariantMap> results;
    const auto order = m_db.findOrderById(orderId);
    if (order.has_value()) {
        results.append(orderToMap(*order));
    }
    return results;
}

QVector<QVariantMap> TicketManager::queryAllOrders() const
{
    // 查询全部订单（含车次、站名、日期等关联信息），供管理端汇总展示
    QVector<QVariantMap> results;
    const auto details = m_db.findAllOrdersWithDetails();
    for (const auto &detail : details) {
        QVariantMap map;
        map[QStringLiteral("orderId")] = detail.orderId;
        map[QStringLiteral("userId")] = detail.userId;
        map[QStringLiteral("tripId")] = detail.tripId;
        map[QStringLiteral("trainId")] = detail.trainId;
        map[QStringLiteral("status")] = detail.status;
        map[QStringLiteral("trainNumber")] = detail.trainNumber;
        map[QStringLiteral("passengerName")] = detail.passengerName;
        map[QStringLiteral("purchaseTime")] = detail.purchaseTime;
        map[QStringLiteral("departureStation")] = detail.departureStationName;
        map[QStringLiteral("arrivalStation")] = detail.arrivalStationName;
        map[QStringLiteral("travelDate")] = detail.travelDate;
        results.append(map);
    }
    return results;
}

bool TicketManager::addOperationLog(const QString &operatorUsername,
                                    const QString &action,
                                    const QString &detail)
{
    // 写入一条操作日志（操作人、动作、详情）
    return m_db.addOperationLog(operatorUsername, action, detail);
}
