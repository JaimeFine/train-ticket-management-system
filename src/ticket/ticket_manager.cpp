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

QDateTime readTripDepartureDateTime(const QString &travelDate, const QString &departureTime)
{
    const QDate date = QDate::fromString(travelDate, QStringLiteral("yyyy-MM-dd"));
    const QTime time = readTime(departureTime);
    if (!date.isValid() || !time.isValid()) {
        return QDateTime();
    }
    return QDateTime(date, time);
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
        if ((hour >= 7 && hour < 10) || (hour >= 17 && hour < 20)) {
            timeFactor = 1.10;
        }
    }

    const QDate travelDate = QDate::fromString(trip.travelDate, QStringLiteral("yyyy-MM-dd"));
    const double holidayFactor = holidayDemandFactor(travelDate);

    QDateTime departure;
    if (travelDate.isValid() && departureClock.isValid()) {
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
    const double protectedPrice = std::max(basePrice * 0.70, result);
    return std::round(protectedPrice);
}

QVariantMap tripToMap(const DatabaseManager::TrainWithStations &trip)
{
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

TicketManager::TicketManager(DatabaseManager &db) : m_db(db) {}

int TicketManager::bookTicket(int userId, int tripId, const QString &passengerName)
{
    const auto trip = m_db.findTripById(tripId);
    if (!trip.has_value()) {
        m_lastError = QStringLiteral("班次不存在");
        return -1;
    }
    if (!trip->enabled) {
        m_lastError = QStringLiteral("该班次已停运");
        return -1;
    }
    if (trip->remainingSeats <= 0) {
        m_lastError = QStringLiteral("该班次已无余票");
        return -1;
    }

    const auto train = m_db.findTrainById(trip->trainId);
    if (!train.has_value() || !train->enabled) {
        m_lastError = QStringLiteral("该车次已停运");
        return -1;
    }

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
    order.status = 0;

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
    const auto trip = m_db.findTripById(tripId);
    return trip.has_value() ? trip->remainingSeats : -1;
}

QVector<QVariantMap> TicketManager::searchTrips(const QString &dep,
                                                const QString &arr,
                                                const QString &date) const
{
    QVector<QVariantMap> results;
    for (const auto &trip : m_db.searchTripsByStation(dep, arr, date)) {
        results.append(tripToMap(trip));
    }
    return results;
}

QVector<QVariantMap> TicketManager::searchByTrainNumber(const QString &number) const
{
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
    return m_lastError;
}

bool TicketManager::refundTicket(int orderId)
{
    const auto order = m_db.findOrderById(orderId);
    if (!order.has_value()) {
        m_lastError = QStringLiteral("订单不存在");
        return false;
    }
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

    if (!m_db.updateOrderStatus(orderId, 2)) {
        m_db.rollbackTransaction();
        m_lastError = QStringLiteral("更新旧订单失败");
        return false;
    }
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

    OrderRecord newOrder;
    newOrder.userId = oldOrder->userId;
    newOrder.trainId = newTrip->trainId;
    newOrder.tripId = newTripId;
    newOrder.passengerName = oldOrder->passengerName;
    newOrder.travelDate = newTrip->travelDate;
    newOrder.purchaseTime = QDateTime::currentDateTime()
                                .toString(QStringLiteral("yyyy-MM-dd HH:mm:ss"));
    newOrder.price = newTrip->basePrice;
    newOrder.status = 0;

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

static QVariantMap orderToMap(const OrderRecord &order)
{
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
    QVector<QVariantMap> results;
    const auto orders = m_db.findOrdersByUser(userId);
    for (const auto &order : orders) {
        results.append(orderToMap(order));
    }
    return results;
}

QVector<QVariantMap> TicketManager::queryOrdersByPassenger(const QString &name) const
{
    QVector<QVariantMap> results;
    const auto orders = m_db.findOrdersByPassenger(name);
    for (const auto &order : orders) {
        results.append(orderToMap(order));
    }
    return results;
}

QVector<QVariantMap> TicketManager::queryOrderByOrderId(int orderId) const
{
    QVector<QVariantMap> results;
    const auto order = m_db.findOrderById(orderId);
    if (order.has_value()) {
        results.append(orderToMap(*order));
    }
    return results;
}

QVector<QVariantMap> TicketManager::queryAllOrders() const
{
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
    return m_db.addOperationLog(operatorUsername, action, detail);
}
