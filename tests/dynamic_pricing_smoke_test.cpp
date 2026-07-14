#include "database_manager.h"
#include "ticket_manager.h"

#include <QCoreApplication>
#include <QDate>
#include <QDateTime>
#include <QDebug>
#include <QSqlDatabase>
#include <QSqlQuery>

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

QDate nextWeekdayDate()
{
    QDate date = QDate::currentDate().addDays(7);
    while (date.dayOfWeek() == Qt::Saturday || date.dayOfWeek() == Qt::Sunday) {
        date = date.addDays(1);
    }
    return date;
}

QString timeText(const QDateTime &dateTime)
{
    return dateTime.time().toString(QStringLiteral("HH:mm"));
}

bool setTripBasePrice(int tripId, double basePrice)
{
    QSqlQuery query(QSqlDatabase::database(QStringLiteral("train_ticket_connection")));
    query.prepare(QStringLiteral("UPDATE Trip SET basePrice = :basePrice WHERE tripId = :tripId"));
    query.bindValue(QStringLiteral(":basePrice"), basePrice);
    query.bindValue(QStringLiteral(":tripId"), tripId);
    return query.exec() && query.numRowsAffected() == 1;
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

    const QDate weekdayDate = nextWeekdayDate();
    const QString weekdayDateText = weekdayDate.toString(QStringLiteral("yyyy-MM-dd"));
    const QDate holidayDate(QDate::currentDate().year(), 10, 1);
    const QString holidayDateText = holidayDate.toString(QStringLiteral("yyyy-MM-dd"));
    const QDateTime nearDeparture = QDateTime::currentDateTime().addSecs(2 * 60 * 60);
    const QString nearDepartureDateText = nearDeparture.date().toString(QStringLiteral("yyyy-MM-dd"));
    const QString nearDepartureTimeText = timeText(nearDeparture);
    const QString nearArrivalTimeText = timeText(nearDeparture.addSecs(2 * 60 * 60));

    TrainRecord normalTrain;
    normalTrain.trainNumber = uniqueName(QStringLiteral("G_PRICE_NORMAL"));
    normalTrain.departureStationId = storedDeparture->stationId;
    normalTrain.arrivalStationId = storedArrival->stationId;
    normalTrain.departureTime = QStringLiteral("11:00");
    normalTrain.arrivalTime = QStringLiteral("13:00");
    normalTrain.totalSeats = 100;

    TrainRecord busyTrain = normalTrain;
    busyTrain.trainNumber = uniqueName(QStringLiteral("G_PRICE_BUSY"));

    TrainRecord holidayTrain = normalTrain;
    holidayTrain.trainNumber = uniqueName(QStringLiteral("G_PRICE_HOLIDAY"));

    TrainRecord nearDepartureTrain = normalTrain;
    nearDepartureTrain.trainNumber = uniqueName(QStringLiteral("G_PRICE_NEAR"));
    nearDepartureTrain.departureTime = nearDepartureTimeText;
    nearDepartureTrain.arrivalTime = nearArrivalTimeText;

    if (!databaseManager.addTrain(normalTrain)
        || !databaseManager.addTrain(busyTrain)
        || !databaseManager.addTrain(holidayTrain)
        || !databaseManager.addTrain(nearDepartureTrain)) {
        qCritical() << "Could not create pricing test trains:" << databaseManager.lastError();
        return 1;
    }

    const auto storedNormalTrain = databaseManager.findTrainByNumber(normalTrain.trainNumber);
    const auto storedBusyTrain = databaseManager.findTrainByNumber(busyTrain.trainNumber);
    const auto storedHolidayTrain = databaseManager.findTrainByNumber(holidayTrain.trainNumber);
    const auto storedNearDepartureTrain = databaseManager.findTrainByNumber(nearDepartureTrain.trainNumber);
    if (!storedNormalTrain || !storedBusyTrain || !storedHolidayTrain || !storedNearDepartureTrain) {
        qCritical() << "Could not reload pricing test trains.";
        return 1;
    }

    const auto normalTripId =
        databaseManager.createTrip(storedNormalTrain->trainId, weekdayDateText, 100);
    const auto busyTripId =
        databaseManager.createTrip(storedBusyTrain->trainId, weekdayDateText, 100);
    const auto holidayTripId =
        databaseManager.createTrip(storedHolidayTrain->trainId, holidayDateText, 100);
    const auto nearDepartureTripId =
        databaseManager.createTrip(storedNearDepartureTrain->trainId, nearDepartureDateText, 100);
    if (!normalTripId || !busyTripId || !holidayTripId || !nearDepartureTripId) {
        qCritical() << "Could not create pricing test trips:" << databaseManager.lastError();
        return 1;
    }

    if (!databaseManager.adjustTripSeats(*busyTripId, -90)) {
        qCritical() << "Could not reduce busy-trip seats:" << databaseManager.lastError();
        return 1;
    }
    if (!databaseManager.adjustTripSeats(*nearDepartureTripId, -10)) {
        qCritical() << "Could not reduce near-departure trip seats:" << databaseManager.lastError();
        return 1;
    }
    if (!setTripBasePrice(*normalTripId, 300.0)
        || !setTripBasePrice(*busyTripId, 300.0)
        || !setTripBasePrice(*holidayTripId, 300.0)
        || !setTripBasePrice(*nearDepartureTripId, 300.0)) {
        qCritical() << "Could not set pricing test base fares.";
        return 1;
    }

    TicketManager ticketManager(databaseManager);
    const QVector<QVariantMap> results = ticketManager.searchTrips(
        departure.stationName,
        arrival.stationName,
        QString());

    const QVariantMap normalResult = findResult(results, normalTrain.trainNumber);
    const QVariantMap busyResult = findResult(results, busyTrain.trainNumber);
    const QVariantMap holidayResult = findResult(results, holidayTrain.trainNumber);
    const QVariantMap nearDepartureResult = findResult(results, nearDepartureTrain.trainNumber);
    if (normalResult.isEmpty() || busyResult.isEmpty()
        || holidayResult.isEmpty() || nearDepartureResult.isEmpty()) {
        qCritical() << "Pricing search did not return all pricing test trains.";
        return 1;
    }

    const QVector<QVariantMap> allResults = ticketManager.searchTrips(
        QString(), QString(), QString());
    if (findResult(allResults, normalTrain.trainNumber).isEmpty()
        || findResult(allResults, busyTrain.trainNumber).isEmpty()) {
        qCritical() << "Empty search conditions did not return all active trains.";
        return 1;
    }

    if (normalResult.value(QStringLiteral("travelMinutes")).toInt() != 120
        || nearDepartureResult.value(QStringLiteral("travelMinutes")).toInt() != 120) {
        qCritical() << "Travel duration was not calculated correctly.";
        return 1;
    }

    const double basePrice = normalResult.value(QStringLiteral("basePrice")).toDouble();
    if (std::abs(basePrice - 300.0) > 0.001
        || std::abs(busyResult.value(QStringLiteral("basePrice")).toDouble() - basePrice) > 0.001
        || std::abs(holidayResult.value(QStringLiteral("basePrice")).toDouble() - basePrice) > 0.001
        || std::abs(nearDepartureResult.value(QStringLiteral("basePrice")).toDouble() - basePrice) > 0.001) {
        qCritical() << "Base fare was not sourced from the trip database value correctly:"
                    << basePrice;
        return 1;
    }

    const double normalPrice = normalResult.value(QStringLiteral("dynamicPrice")).toDouble();
    const double busyPrice = busyResult.value(QStringLiteral("dynamicPrice")).toDouble();
    const double holidayPrice = holidayResult.value(QStringLiteral("dynamicPrice")).toDouble();
    const double nearDeparturePrice = nearDepartureResult.value(QStringLiteral("dynamicPrice")).toDouble();

    if (normalPrice <= 0.0 || busyPrice <= normalPrice) {
        qCritical() << "Dynamic price did not increase when seats became scarce:"
                    << normalPrice << busyPrice;
        return 1;
    }
    if (holidayPrice <= normalPrice) {
        qCritical() << "Holiday travel did not price above a normal weekday:"
                    << normalPrice << holidayPrice;
        return 1;
    }
    if (nearDeparturePrice >= normalPrice) {
        qCritical() << "Near-departure trip with high remaining seats was not discounted:"
                    << normalPrice << nearDeparturePrice;
        return 1;
    }
    if (nearDeparturePrice < 210.0) {
        qCritical() << "Near-departure price broke the 70% base-fare floor:"
                    << nearDeparturePrice;
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
    qDebug() << "Normal/Buzzy/Holiday/Near prices:"
             << normalPrice << busyPrice << holidayPrice << nearDeparturePrice;
    return 0;
}
