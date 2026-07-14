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
    normalTrain.departureTime = travelDate + QStringLiteral(" 11:00:00");
    normalTrain.arrivalTime = travelDate + QStringLiteral(" 13:00:00");
    normalTrain.totalSeats = 100;
    normalTrain.remainingSeats = 100;

    TrainRecord busyTrain = normalTrain;
    busyTrain.trainNumber = uniqueName(QStringLiteral("G_PRICE_BUSY"));
    busyTrain.remainingSeats = 10;

    if (!databaseManager.addTrain(normalTrain)
        || !databaseManager.addTrain(busyTrain)) {
        qCritical() << "Could not create pricing test trains:" << databaseManager.lastError();
        return 1;
    }

    TicketManager ticketManager(databaseManager);
    const QVector<QVariantMap> results = ticketManager.searchTrains(
        departure.stationName,
        arrival.stationName,
        travelDate);

    const QVariantMap normalResult = findResult(results, normalTrain.trainNumber);
    const QVariantMap busyResult = findResult(results, busyTrain.trainNumber);
    if (normalResult.isEmpty() || busyResult.isEmpty()) {
        qCritical() << "Pricing search did not return both test trains.";
        return 1;
    }

    const QVector<QVariantMap> allResults = ticketManager.searchTrains(
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
