/**
 * @file statistics_smoke_test.cpp - Issue 11 smoke test
 * Tests StatisticsManager aggregate queries + TicketManager::queryAllOrders()
 * with unique test data and baseline-aware assertions.
 */
#include "database_manager.h"
#include "ticket_manager.h"
#include "statistics_manager.h"

#include <QCoreApplication>
#include <QDateTime>
#include <QDebug>
#include <QRandomGenerator>

static QString uniq(const QString &prefix) {
    return QStringLiteral("%1_%2_%3")
        .arg(prefix)
        .arg(QDateTime::currentMSecsSinceEpoch())
        .arg(QRandomGenerator::global()->bounded(10000));
}

int main(int argc, char *argv[]) {
    QCoreApplication app(argc, argv);

    DatabaseManager db;
    if (!db.initialize()) { qCritical() << "FAIL: DB init:" << db.lastError(); return 1; }

    // ── Create unique test user ──────────────────────────────────────
    UserRecord user;
    user.username = uniq("stats_user");
    user.password = "test_pw";
    user.role     = 1;
    user.enabled  = true;
    if (!db.addUser(user)) { qCritical() << "FAIL: addUser:" << db.lastError(); return 1; }

    auto storedUser = db.findUserByUsername(user.username);
    if (!storedUser) { qCritical() << "FAIL: findUserByUsername returned null for" << user.username; return 1; }
    qDebug() << "OK: user created, id=" << storedUser->userId;

    // ── Create unique stations ───────────────────────────────────────
    StationRecord depStation, arrStation;
    depStation.stationName = uniq("Stats_Dep");
    arrStation.stationName = uniq("Stats_Arr");
    if (!db.addStation(depStation)) { qCritical() << "FAIL: addStation dep:" << db.lastError(); return 1; }
    if (!db.addStation(arrStation)) { qCritical() << "FAIL: addStation arr:" << db.lastError(); return 1; }

    auto dep = db.findStationByName(depStation.stationName);
    auto arr = db.findStationByName(arrStation.stationName);
    if (!dep)  { qCritical() << "FAIL: findStationByName dep returned null"; return 1; }
    if (!arr)  { qCritical() << "FAIL: findStationByName arr returned null"; return 1; }
    qDebug() << "OK: stations created:" << dep->stationName << "->" << arr->stationName;

    // ── Create unique train ──────────────────────────────────────────
    TrainRecord train;
    train.trainNumber        = uniq("Stats_G");
    train.departureStationId = dep->stationId;
    train.arrivalStationId   = arr->stationId;
    train.departureTime      = "08:00";
    train.arrivalTime        = "12:00";
    train.totalSeats         = 50;
    train.enabled            = true;
    if (!db.addTrain(train)) { qCritical() << "FAIL: addTrain:" << db.lastError(); return 1; }

    auto storedTrain = db.findTrainByNumber(train.trainNumber);
    if (!storedTrain) { qCritical() << "FAIL: findTrainByNumber returned null"; return 1; }
    qDebug() << "OK: train created:" << storedTrain->trainNumber;

    const auto storedTripId = db.createTrip(storedTrain->trainId, "2026-07-10", 50);
    if (!storedTripId) { qCritical() << "FAIL: createTrip:" << db.lastError(); return 1; }

    // ── Managers ─────────────────────────────────────────────────────
    TicketManager tm(db);
    StatisticsManager sm(db);

    // ── Record baselines (DB may contain data from prior runs) ──────
    const int baseOrders   = sm.totalOrders();
    const int baseBooked   = sm.totalBooked();
    const int baseRefunded = sm.totalRefunded();
    const int baseChanged  = sm.totalChanged();

    if (baseOrders < 0 || baseBooked < 0 || baseRefunded < 0 || baseChanged < 0) {
        qCritical() << "FAIL: baseline statistics returned negative"; return 1;
    }
    qDebug() << "OK: baselines =" << baseOrders << baseBooked << baseRefunded << baseChanged;

    // ── Book 3 tickets, refund 1 ────────────────────────────────────
    const int o1 = tm.bookTicket(storedUser->userId, *storedTripId, "Passenger_A");
    const int o2 = tm.bookTicket(storedUser->userId, *storedTripId, "Passenger_B");
    const int o3 = tm.bookTicket(storedUser->userId, *storedTripId, "Passenger_C");

    if (o1 <= 0) { qCritical() << "FAIL: bookTicket A returned" << o1 << "error:" << tm.lastError(); return 1; }
    if (o2 <= 0) { qCritical() << "FAIL: bookTicket B returned" << o2 << "error:" << tm.lastError(); return 1; }
    if (o3 <= 0) { qCritical() << "FAIL: bookTicket C returned" << o3 << "error:" << tm.lastError(); return 1; }
    qDebug() << "OK: 3 tickets booked, orderIds =" << o1 << o2 << o3;

    if (!tm.refundTicket(o3)) { qCritical() << "FAIL: refundTicket" << o3 << "error:" << tm.lastError(); return 1; }
    qDebug() << "OK: order" << o3 << "refunded";

    // ── Verify order-level statistics (delta from baseline) ─────────
    if (sm.totalOrders()   - baseOrders   != 3) { qCritical() << "FAIL: totalOrders expected delta=3, got" << (sm.totalOrders() - baseOrders);   return 1; }
    if (sm.totalBooked()   - baseBooked   != 2) { qCritical() << "FAIL: totalBooked expected delta=2, got" << (sm.totalBooked() - baseBooked);   return 1; }
    if (sm.totalRefunded() - baseRefunded != 1) { qCritical() << "FAIL: totalRefunded expected delta=1, got" << (sm.totalRefunded() - baseRefunded); return 1; }
    if (sm.totalChanged()  - baseChanged  != 0) { qCritical() << "FAIL: totalChanged expected delta=0, got" << (sm.totalChanged() - baseChanged);  return 1; }
    qDebug() << "PASS: statistics deltas correct (3/2/1/0)";

    // ── Verify popularRoutes — find our route in the list ──────────
    auto routes = sm.popularRoutes();
    if (routes.isEmpty()) { qCritical() << "FAIL: popularRoutes empty"; return 1; }

    bool foundOurRoute = false;
    for (const auto &r : routes) {
        const QString rDep = r["departureStation"].toString();
        const QString rArr = r["arrivalStation"].toString();
        if (rDep == depStation.stationName && rArr == arrStation.stationName) {
            const int cnt = r["orderCount"].toInt();
            if (cnt != 2) { qCritical() << "FAIL: our route orderCount expected 2, got" << cnt; return 1; }
            foundOurRoute = true;
            qDebug() << "PASS: popularRoutes found our route:" << rDep << "->" << rArr << "=" << cnt;
            break;
        }
    }
    if (!foundOurRoute) { qCritical() << "FAIL: our route not found in popularRoutes"; return 1; }

    // ── Verify monthlyPassengerFlow ──────────────────────────────────
    auto monthly = sm.monthlyPassengerFlow();
    if (monthly.isEmpty()) { qCritical() << "FAIL: monthlyPassengerFlow empty"; return 1; }

    // Sum recent months' orders (may span multiple months depending on test data)
    int totalMonthlyOrders = 0;
    const int maxMonths = qMin(3, monthly.size());
    for (int i = 0; i < maxMonths; ++i)
        totalMonthlyOrders += monthly[i]["totalOrders"].toInt();
    if (totalMonthlyOrders < 3) {
        qCritical() << "FAIL: monthlyPassengerFlow too few orders, expected >=3 got" << totalMonthlyOrders; return 1;
    }
    qDebug() << "PASS: monthlyPassengerFlow has" << monthly.size() << "month(s), recent totalOrders:" << totalMonthlyOrders;

    // ── Issue 11: verify queryAllOrders (order history with details) ─
    auto allOrders = tm.queryAllOrders();
    if (allOrders.isEmpty()) { qCritical() << "FAIL: queryAllOrders empty"; return 1; }

    bool foundA = false, foundB = false, foundC = false;
    for (const auto &o : allOrders) {
        const QString name = o["passengerName"].toString();
        if (name == "Passenger_A") foundA = true;
        if (name == "Passenger_B") foundB = true;
        if (name == "Passenger_C") foundC = true;

        // Verify expected keys exist
        if (!o.contains("trainNumber") || !o.contains("departureStation") || !o.contains("arrivalStation")) {
            qCritical() << "FAIL: queryAllOrders result missing detail keys for" << name; return 1;
        }
    }
    if (!foundA || !foundB || !foundC) {
        qCritical() << "FAIL: queryAllOrders missing expected passengers (A/B/C)"; return 1;
    }
    qDebug() << "PASS: queryAllOrders returned" << allOrders.size() << "orders with details (A/B/C present)";

    qDebug() << "\nAll Issue 11 statistics smoke tests PASSED";
    return 0;
}
