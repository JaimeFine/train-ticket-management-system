/**
 * @file statistics_smoke_test.cpp - Issue 11 smoke test
 * Tests StatisticsManager aggregate queries
 */
#include "database_manager.h"
#include "ticket_manager.h"
#include "statistics_manager.h"

#include <QCoreApplication>
#include <QDateTime>
#include <QDebug>

static QString uniq(const QString &p) {
    return QStringLiteral("%1_%2").arg(p).arg(QDateTime::currentMSecsSinceEpoch());
}

int main(int argc, char *argv[]) {
    QCoreApplication app(argc, argv);

    DatabaseManager db;
    if (!db.initialize()) { qCritical()<<"DB init failed"; return 1; }

    UserRecord u; u.username=uniq("su"); u.password="pw"; u.role=1; u.enabled=true;
    db.addUser(u); auto user=db.findUserByUsername(u.username);

    StationRecord s1,s2; s1.stationName="Beijing"; s2.stationName="Shanghai";
    db.addStation(s1); db.addStation(s2);
    auto dep=db.findStationByName("Beijing"),arr=db.findStationByName("Shanghai");

    TrainRecord t; t.trainNumber=uniq("G"); t.departureStationId=dep->stationId;
    t.arrivalStationId=arr->stationId; t.departureTime="2026-07-10 08:00";
    t.arrivalTime="2026-07-10 12:00"; t.totalSeats=50; t.remainingSeats=50; t.enabled=true;
    db.addTrain(t); auto train=db.findTrainByNumber(t.trainNumber);

    TicketManager tm(db);
    StatisticsManager sm(db);

    // --- No orders yet ---
    if (sm.totalOrders()!=0) { qCritical()<<"totalOrders should be 0"; return 1; }
    qDebug()<<"PASS: totalOrders = 0 initially";

    // Create orders with different statuses
    int o1=tm.bookTicket(user->userId,train->trainId,"A");
    int o2=tm.bookTicket(user->userId,train->trainId,"B");
    int o3=tm.bookTicket(user->userId,train->trainId,"C");
    tm.refundTicket(o3);  // 1 refunded

    // --- Test 1: totals ---
    if (sm.totalOrders()!=3) { qCritical()<<"totalOrders expected 3, got"<<sm.totalOrders(); return 1; }
    if (sm.totalBooked()!=2) { qCritical()<<"totalBooked expected 2, got"<<sm.totalBooked(); return 1; }
    if (sm.totalRefunded()!=1) { qCritical()<<"totalRefunded expected 1, got"<<sm.totalRefunded(); return 1; }
    if (sm.totalChanged()!=0) { qCritical()<<"totalChanged expected 0, got"<<sm.totalChanged(); return 1; }
    qDebug()<<"PASS: totals correct (3/2/1/0)";

    // --- Test 2: popularRoutes ---
    auto routes = sm.popularRoutes();
    if (routes.isEmpty()) { qCritical()<<"popularRoutes should not be empty"; return 1; }
    if (routes[0]["departureStation"].toString()!="Beijing") { qCritical()<<"wrong departure"; return 1; }
    if (routes[0]["orderCount"].toInt()!=2) { qCritical()<<"wrong orderCount, got"<<routes[0]["orderCount"].toInt(); return 1; }
    qDebug()<<"PASS: popularRoutes top:"<<routes[0]["departureStation"].toString()
            <<"->"<<routes[0]["arrivalStation"].toString()<<"="<<routes[0]["orderCount"].toInt();

    // --- Test 3: monthlyPassengerFlow ---
    auto monthly = sm.monthlyPassengerFlow();
    if (monthly.isEmpty()) { qCritical()<<"monthlyPassengerFlow should not be empty"; return 1; }
    qDebug()<<"PASS: monthlyPassengerFlow has"<<monthly.size()<<"month(s)";

    qDebug()<<"\nAll statistics API smoke tests PASSED";
    return 0;
}
