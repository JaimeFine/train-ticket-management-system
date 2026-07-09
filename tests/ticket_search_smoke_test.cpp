/**
 * @file ticket_search_smoke_test.cpp - Issue 10 smoke test
 * Tests order queries and train search APIs
 */
#include "database_manager.h"
#include "ticket_manager.h"

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

    // Setup
    UserRecord u; u.username=uniq("su"); u.password="pw"; u.role=1; u.enabled=true;
    db.addUser(u); auto user=db.findUserByUsername(u.username);
    StationRecord s1,s2; s1.stationName="BeijingTest"; s2.stationName="ShanghaiTest";
    db.addStation(s1); db.addStation(s2);
    auto dep=db.findStationByName("BeijingTest"),arr=db.findStationByName("ShanghaiTest");

    TrainRecord t; t.trainNumber=uniq("G"); t.departureStationId=dep->stationId;
    t.arrivalStationId=arr->stationId; t.departureTime="2026-07-10 08:00";
    t.arrivalTime="2026-07-10 12:00"; t.totalSeats=50; t.remainingSeats=50; t.enabled=true;
    db.addTrain(t); auto train=db.findTrainByNumber(t.trainNumber);

    TicketManager tm(db);

    // Create a few orders
    tm.bookTicket(user->userId,train->trainId,"Alice");
    tm.bookTicket(user->userId,train->trainId,"Bob");
    tm.bookTicket(user->userId,train->trainId,"Charlie");

    // --- Test 1: queryOrdersByUser ---
    auto orders = tm.queryOrdersByUser(user->userId);
    if (orders.size()!=3) { qCritical()<<"Expected 3 orders, got"<<orders.size(); return 1; }
    qDebug()<<"PASS: queryOrdersByUser returns"<<orders.size();

    // --- Test 2: queryOrdersByPassenger ---
    auto byName = tm.queryOrdersByPassenger("Bob");
    if (byName.size()!=1 || byName[0]["passengerName"].toString()!="Bob")
    { qCritical()<<"queryOrdersByPassenger failed"; return 1; }
    qDebug()<<"PASS: queryOrdersByPassenger found Bob";

    // --- Test 3: queryOrderByOrderId ---
    int firstId = orders[0]["orderId"].toInt();
    auto byId = tm.queryOrderByOrderId(firstId);
    if (byId.isEmpty()) { qCritical()<<"queryOrderByOrderId failed"; return 1; }
    qDebug()<<"PASS: queryOrderByOrderId found order"<<firstId;

    // --- Test 4: queryAllOrders ---
    auto all = tm.queryAllOrders();
    if (all.size()<3) { qCritical()<<"queryAllOrders insufficient"; return 1; }
    qDebug()<<"PASS: queryAllOrders returns"<<all.size()<<"with details";

    // --- Test 5: searchTrains ---
    auto trains = tm.searchTrains("Beijing","Shanghai","2026-07-10");
    if (trains.isEmpty()) { qCritical()<<"searchTrains failed"; return 1; }
    qDebug()<<"PASS: searchTrains found"<<trains.size()<<"trains";

    // --- Test 6: searchByTrainNumber ---
    auto byNum = tm.searchByTrainNumber(train->trainNumber);
    if (byNum.isEmpty()) { qCritical()<<"searchByTrainNumber failed"; return 1; }
    qDebug()<<"PASS: searchByTrainNumber found"<<byNum[0]["trainNumber"].toString();

    // --- Test 7: search non-existent ---
    auto empty = tm.searchTrains("Mars","Venus","");
    if (!empty.isEmpty()) { qCritical()<<"searchTrains should return empty"; return 1; }
    qDebug()<<"PASS: searchTrains correctly empty for Mars->Venus";

    qDebug()<<"\nAll search API smoke tests PASSED";
    return 0;
}
