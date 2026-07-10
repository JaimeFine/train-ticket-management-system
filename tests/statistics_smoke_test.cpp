/**
 * @file statistics_smoke_test.cpp - Issue 11 smoke test
 * Tests StatisticsManager aggregate queries with unique test data
 */
#include "database_manager.h"
#include "ticket_manager.h"
#include "statistics_manager.h"

#include <QCoreApplication>
#include <QDateTime>
#include <QDebug>

static QString uniq(const QString &prefix) {
    return QStringLiteral("%1_%2").arg(prefix).arg(QDateTime::currentMSecsSinceEpoch());
}

int main(int argc, char *argv[]) {
    QCoreApplication app(argc, argv);

    DatabaseManager db;
    if (!db.initialize()) { qCritical() << "DB init failed:" << db.lastError(); return 1; }

    UserRecord user;
    user.username = uniq("stats_user");
    user.password = "test_pw";
    user.role     = 1;
    user.enabled  = true;
    if (!db.addUser(user)) { qCritical() << "addUser failed:" << db.lastError(); return 1; }

    auto storedUser = db.findUserByUsername(user.username);
    if (!storedUser) { qCritical() << "findUserByUsername returned null"; return 1; }

    StationRecord depStation, arrStation;
    depStation.stationName = uniq("Stats_Dep");
    arrStation.stationName = uniq("Stats_Arr");
    if (!db.addStation(depStation)) { qCritical() << "addStation dep failed:" << db.lastError(); return 1; }
    if (!db.addStation(arrStation)) { qCritical() << "addStation arr failed:" << db.lastError(); return 1; }

    auto dep = db.findStationByName(depStation.stationName);
    auto arr = db.findStationByName(arrStation.stationName);
    if (!dep || !arr) { qCritical() << "findStationByName returned null"; return 1; }

    TrainRecord train;
    train.trainNumber        = uniq("Stats_G");
    train.departureStationId = dep->stationId;
    train.arrivalStationId   = arr->stationId;
    train.departureTime      = "2026-07-10 08:00";
    train.arrivalTime        = "2026-07-10 12:00";
    train.totalSeats         = 50;
    train.remainingSeats     = 50;
    train.enabled            = true;
    if (!db.addTrain(train)) { qCritical() << "addTrain failed:" << db.lastError(); return 1; }

    auto storedTrain = db.findTrainByNumber(train.trainNumber);
    if (!storedTrain) { qCritical() << "findTrainByNumber returned null"; return 1; }

    TicketManager tm(db);
    StatisticsManager sm(db);

    if (sm.totalOrders()  != 0) { qCritical() << "totalOrders expected 0, got"  << sm.totalOrders();  return 1; }
    if (sm.totalBooked()  != 0) { qCritical() << "totalBooked expected 0, got"  << sm.totalBooked();  return 1; }
    if (sm.totalRefunded()!= 0) { qCritical() << "totalRefunded expected 0, got"<< sm.totalRefunded();return 1; }
    if (sm.totalChanged() != 0) { qCritical() << "totalChanged expected 0, got" << sm.totalChanged(); return 1; }
    qDebug() << "PASS: totals all zero initially";

    int o1 = tm.bookTicket(storedUser->userId, storedTrain->trainId, "Passenger_A");
    int o2 = tm.bookTicket(storedUser->userId, storedTrain->trainId, "Passenger_B");
    int o3 = tm.bookTicket(storedUser->userId, storedTrain->trainId, "Passenger_C");
    if (o1 <= 0 || o2 <= 0 || o3 <= 0) { qCritical() << "bookTicket failed:" << tm.lastError(); return 1; }

    if (!tm.refundTicket(o3)) { qCritical() << "refundTicket failed:" << tm.lastError(); return 1; }

    if (sm.totalOrders()   != 3) { qCritical() << "totalOrders expected 3, got"   << sm.totalOrders();   return 1; }
    if (sm.totalBooked()   != 2) { qCritical() << "totalBooked expected 2, got"   << sm.totalBooked();   return 1; }
    if (sm.totalRefunded() != 1) { qCritical() << "totalRefunded expected 1, got" << sm.totalRefunded(); return 1; }
    if (sm.totalChanged()  != 0) { qCritical() << "totalChanged expected 0, got"  << sm.totalChanged();  return 1; }
    qDebug() << "PASS: totals correct (3/2/1/0)";

    auto routes = sm.popularRoutes();
    if (routes.isEmpty()) { qCritical() << "popularRoutes should not be empty"; return 1; }

    const QString topDep = routes[0]["departureStation"].toString();
    const QString topArr = routes[0]["arrivalStation"].toString();
    const int    topCnt  = routes[0]["orderCount"].toInt();

    if (topDep != depStation.stationName) {
        qCritical() << "wrong departure:" << topDep << "expected:" << depStation.stationName; return 1;
    }
    if (topCnt != 2) { qCritical() << "wrong orderCount:" << topCnt << "expected 2"; return 1; }
    qDebug() << "PASS: popularRoutes top:" << topDep << "->" << topArr << "=" << topCnt;

    auto monthly = sm.monthlyPassengerFlow();
    if (monthly.isEmpty()) { qCritical() << "monthlyPassengerFlow should not be empty"; return 1; }
    if (monthly[0]["totalOrders"].toInt() < 3) {
        qCritical() << "monthlyPassengerFlow totalOrders too low:" << monthly[0]["totalOrders"].toInt(); return 1;
    }
    qDebug() << "PASS: monthlyPassengerFlow has" << monthly.size() << "month(s)";

    qDebug() << "\nAll statistics API smoke tests PASSED";
    return 0;
}
