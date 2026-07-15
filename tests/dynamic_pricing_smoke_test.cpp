#include "database_manager.h"
#include "ticket_manager.h"

#include <QCoreApplication>
#include <QDate>
#include <QDateTime>
#include <QDebug>

#include <cmath>

namespace {
QString uniqueName(const QString &prefix)
{
    return QStringLiteral("%1_%2")
        .arg(prefix)
        .arg(QDateTime::currentMSecsSinceEpoch());
}

QVariantMap findResult(const QVector<QVariantMap> &results,
                       const QString &trainNumber)
{
    for (const QVariantMap &result : results) {
        if (result.value(QStringLiteral("trainNumber")).toString() == trainNumber) {
            return result;
        }
    }
    return {};
}
}

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);

    DatabaseManager databaseManager;
    if (!databaseManager.initialize()) {
        qCritical() << "Database initialization failed:" << databaseManager.lastError();
        return 1;
    }

    StationRecord departure;
    departure.stationName = uniqueName(QStringLiteral("price_dep"));
    StationRecord arrival;
    arrival.stationName = uniqueName(QStringLiteral("price_arr"));
    if (!databaseManager.addStation(departure)
        || !databaseManager.addStation(arrival)) {
        qCritical() << "Could not create pricing test stations.";
        return 1;
    }

    const auto storedDeparture = databaseManager.findStationByName(departure.stationName);
    const auto storedArrival = databaseManager.findStationByName(arrival.stationName);
    if (!storedDeparture || !storedArrival) {
        qCritical() << "Could not reload pricing test stations.";
        return 1;
    }

    const QString travelDate = QDate::currentDate().addDays(1)
                                   .toString(QStringLiteral("yyyy-MM-dd"));

    TrainRecord normalTrain;
    normalTrain.trainNumber = uniqueName(QStringLiteral("G_PRICE_NORMAL"));
    normalTrain.departureStationId = storedDeparture->stationId;
    normalTrain.arrivalStationId = storedArrival->stationId;
    normalTrain.departureTime = QStringLiteral("11:00:00");
    normalTrain.arrivalTime = QStringLiteral("13:00:00");
    normalTrain.totalSeats = 100;

    TrainRecord busyTrain = normalTrain;
    busyTrain.trainNumber = uniqueName(QStringLiteral("G_PRICE_BUSY"));

    TrainRecord holidayTrain = normalTrain;
    holidayTrain.trainNumber = uniqueName(QStringLiteral("G_PRICE_HOLIDAY"));
    holidayTrain.departureTime = QStringLiteral("11:00:00");
    holidayTrain.arrivalTime = QStringLiteral("13:00:00");

    TrainRecord ordinaryTrain = holidayTrain;
    ordinaryTrain.trainNumber = uniqueName(QStringLiteral("G_PRICE_ORDINARY"));
    ordinaryTrain.departureTime = QStringLiteral("11:00:00");
    ordinaryTrain.arrivalTime = QStringLiteral("13:00:00");

    TrainRecord lastMinuteTrain = normalTrain;
    lastMinuteTrain.trainNumber = uniqueName(QStringLiteral("G_PRICE_LAST_MINUTE"));
    const QDateTime nearDeparture = QDateTime::currentDateTime().addSecs(3 * 60 * 60);
    lastMinuteTrain.departureTime = nearDeparture.time().toString(QStringLiteral("HH:mm:ss"));
    lastMinuteTrain.arrivalTime = nearDeparture.addSecs(2 * 60 * 60)
                                      .time().toString(QStringLiteral("HH:mm:ss"));

    if (!databaseManager.addTrain(normalTrain)
        || !databaseManager.addTrain(busyTrain)
        || !databaseManager.addTrain(holidayTrain)
        || !databaseManager.addTrain(ordinaryTrain)
        || !databaseManager.addTrain(lastMinuteTrain)) {
        qCritical() << "Could not create pricing test trains:" << databaseManager.lastError();
        return 1;
    }

    const auto storedNormalTrain = databaseManager.findTrainByNumber(normalTrain.trainNumber);
    const auto storedBusyTrain = databaseManager.findTrainByNumber(busyTrain.trainNumber);
    const auto storedHolidayTrain = databaseManager.findTrainByNumber(holidayTrain.trainNumber);
    const auto storedOrdinaryTrain = databaseManager.findTrainByNumber(ordinaryTrain.trainNumber);
    const auto storedLastMinuteTrain = databaseManager.findTrainByNumber(lastMinuteTrain.trainNumber);
    if (!storedNormalTrain || !storedBusyTrain || !storedHolidayTrain
        || !storedOrdinaryTrain || !storedLastMinuteTrain) {
        qCritical() << "Could not reload pricing test trains.";
        return 1;
    }

    const auto normalTripId =
        databaseManager.createTrip(storedNormalTrain->trainId, travelDate, 100);
    const auto busyTripId =
        databaseManager.createTrip(storedBusyTrain->trainId, travelDate, 100);
    const auto holidayTripId = databaseManager.createTrip(
        storedHolidayTrain->trainId, QStringLiteral("2026-10-02"), 100);
    const auto ordinaryTripId = databaseManager.createTrip(
        storedOrdinaryTrain->trainId, QStringLiteral("2026-10-12"), 100);
    const auto lastMinuteTripId = databaseManager.createTrip(
        storedLastMinuteTrain->trainId,
        nearDeparture.date().toString(QStringLiteral("yyyy-MM-dd")),
        100);
    if (!normalTripId || !busyTripId || !holidayTripId
        || !ordinaryTripId || !lastMinuteTripId) {
        qCritical() << "Could not create pricing test trips:" << databaseManager.lastError();
        return 1;
    }

    if (!databaseManager.adjustTripSeats(*busyTripId, -90)) {
        qCritical() << "Could not reduce busy-trip seats:" << databaseManager.lastError();
        return 1;
    }

    TicketManager ticketManager(databaseManager);
    const QVector<QVariantMap> results = ticketManager.searchTrips(
        departure.stationName,
        arrival.stationName,
        travelDate);

    const QVariantMap normalResult = findResult(results, normalTrain.trainNumber);
    const QVariantMap busyResult = findResult(results, busyTrain.trainNumber);
    if (normalResult.isEmpty() || busyResult.isEmpty()) {
        qCritical() << "Pricing search did not return both test trains.";
        return 1;
    }

    const QVector<QVariantMap> allResults = ticketManager.searchTrips(
        QString(), QString(), QString());
    if (findResult(allResults, normalTrain.trainNumber).isEmpty()
        || findResult(allResults, busyTrain.trainNumber).isEmpty()) {
        qCritical() << "Empty search conditions did not return all active trains.";
        return 1;
    }

    if (normalResult.value(QStringLiteral("travelMinutes")).toInt() != 120) {
        qCritical() << "Travel duration was not calculated correctly.";
        return 1;
    }

    const double basePrice = normalResult.value(QStringLiteral("basePrice")).toDouble();
    if (std::abs(basePrice - 221.0) > 0.001
        || std::abs(busyResult.value(QStringLiteral("basePrice")).toDouble()
                    - basePrice) > 0.001) {
        qCritical() << "Base fare was not calculated from the train type correctly:"
                    << basePrice;
        return 1;
    }

    const double normalPrice = normalResult.value(QStringLiteral("dynamicPrice")).toDouble();
    const double busyPrice = busyResult.value(QStringLiteral("dynamicPrice")).toDouble();
    if (normalPrice <= 0.0 || busyPrice <= normalPrice) {
        qCritical() << "Dynamic price did not increase when seats became scarce:"
                    << normalPrice << busyPrice;
        return 1;
    }

    const QVariantMap holidayResult = findResult(
        ticketManager.searchByTrainNumber(holidayTrain.trainNumber),
        holidayTrain.trainNumber);
    const QVariantMap ordinaryResult = findResult(
        ticketManager.searchByTrainNumber(ordinaryTrain.trainNumber),
        ordinaryTrain.trainNumber);
    if (holidayResult.isEmpty() || ordinaryResult.isEmpty()) {
        qCritical() << "Could not reload holiday pricing test trains.";
        return 1;
    }
    if (holidayResult.value(QStringLiteral("dynamicPrice")).toDouble()
        <= ordinaryResult.value(QStringLiteral("dynamicPrice")).toDouble()) {
        qCritical() << "Holiday fare did not apply a holiday demand factor.";
        return 1;
    }

    const QVariantMap lastMinuteResult = findResult(
        ticketManager.searchByTrainNumber(lastMinuteTrain.trainNumber),
        lastMinuteTrain.trainNumber);
    if (lastMinuteResult.isEmpty()) {
        qCritical() << "Could not reload the last-minute pricing test train.";
        return 1;
    }
    if (lastMinuteResult.value(QStringLiteral("dynamicPrice")).toDouble()
        >= lastMinuteResult.value(QStringLiteral("basePrice")).toDouble()) {
        qCritical() << "Last-minute fare was not discounted with many seats left.";
        return 1;
    }

    const QVector<QVariantMap> numberResult =
        ticketManager.searchByTrainNumber(busyTrain.trainNumber);
    if (numberResult.size() != 1
        || std::abs(numberResult.first()
                        .value(QStringLiteral("dynamicPrice"))
                        .toDouble() - busyPrice) > 0.001) {
        qCritical() << "Train-number search returned a different dynamic price.";
        return 1;
    }

    qDebug() << "Dynamic pricing smoke test passed.";
    qDebug() << "Normal price:" << normalPrice << "Busy price:" << busyPrice;
    return 0;
}
