/**
 * @file ticket_api_smoke_test.cpp - Issue 9 smoke test
 * Tests bookTicket, refundTicket, changeTicket with transactions
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
    if (!db.initialize()) { qCritical()<<"DB init failed:"<<db.lastError(); return 1; }

    // Setup: user, stations, train
    UserRecord u; u.username=uniq("tuser"); u.password="pw"; u.role=1; u.enabled=true;
    if(!db.addUser(u)){qCritical()<<"addUser failed";return 1;}
    auto user=db.findUserByUsername(u.username);
    if(!user){qCritical()<<"findUser failed";return 1;}

    StationRecord s1,s2; s1.stationName=uniq("dep"); s2.stationName=uniq("arr");
    if(!db.addStation(s1)||!db.addStation(s2)){qCritical()<<"addStation failed";return 1;}
    auto dep=db.findStationByName(s1.stationName),arr=db.findStationByName(s2.stationName);

    TrainRecord t; t.trainNumber=uniq("T"); t.departureStationId=dep->stationId;
    t.arrivalStationId=arr->stationId; t.departureTime="08:00"; t.arrivalTime="12:00";
    t.totalSeats=10; t.remainingSeats=10; t.enabled=true;
    if(!db.addTrain(t)){qCritical()<<"addTrain failed";return 1;}
    auto train=db.findTrainByNumber(t.trainNumber);
    if(!train){qCritical()<<"findTrain failed";return 1;}

    TicketManager tm(db);

    // --- Test 1: bookTicket ---
    int orderId=tm.bookTicket(user->userId,train->trainId,"PassengerA");
    if(orderId<=0){qCritical()<<"bookTicket failed:"<<tm.lastError();return 1;}
    qDebug()<<"PASS: bookTicket -> orderId"<<orderId;

    // Verify remainingSeats
    int seats=tm.remainingSeats(train->trainId);
    if(seats!=9){qCritical()<<"remainingSeats expected 9, got"<<seats;return 1;}
    qDebug()<<"PASS: remainingSeats ="<<seats;

    // --- Test 2: refundTicket ---
    if(!tm.refundTicket(orderId)){qCritical()<<"refundTicket failed:"<<tm.lastError();return 1;}
    qDebug()<<"PASS: refundTicket order"<<orderId;

    seats=tm.remainingSeats(train->trainId);
    if(seats!=10){qCritical()<<"remainingSeats after refund expected 10, got"<<seats;return 1;}
    qDebug()<<"PASS: seats restored to"<<seats;

    // --- Test 3: refund again should fail ---
    if(tm.refundTicket(orderId)){qCritical()<<"refundTicket should fail on already-refunded";return 1;}
    qDebug()<<"PASS: double-refund correctly rejected";

    // --- Test 4: book then change ---
    int o2=tm.bookTicket(user->userId,train->trainId,"PassengerB");
    if(o2<=0){qCritical()<<"bookTicket 2 failed";return 1;}

    TrainRecord t2; t2.trainNumber=uniq("T2"); t2.departureStationId=dep->stationId;
    t2.arrivalStationId=arr->stationId; t2.departureTime="10:00"; t2.arrivalTime="14:00";
    t2.totalSeats=5; t2.remainingSeats=5; t2.enabled=true;
    if(!db.addTrain(t2)){qCritical()<<"addTrain 2 failed";return 1;}
    auto train2=db.findTrainByNumber(t2.trainNumber);

    if(!tm.changeTicket(o2,train2->trainId)){qCritical()<<"changeTicket failed:"<<tm.lastError();return 1;}
    qDebug()<<"PASS: changeTicket order"<<o2<<"-> train"<<train2->trainId;

    // Verify seat counts
    if(tm.remainingSeats(train->trainId)!=10){qCritical()<<"old train seats wrong";return 1;}
    if(tm.remainingSeats(train2->trainId)!=4){qCritical()<<"new train seats wrong";return 1;}
    qDebug()<<"PASS: seat counts after change correct";

    // --- Test 5: book on disabled train ---
    TrainRecord t3; t3.trainNumber=uniq("T3"); t3.departureStationId=dep->stationId;
    t3.arrivalStationId=arr->stationId; t3.departureTime="12:00"; t3.arrivalTime="16:00";
    t3.totalSeats=3; t3.remainingSeats=3; t3.enabled=false;
    if(!db.addTrain(t3)){qCritical()<<"addTrain 3 failed";return 1;}
    auto train3=db.findTrainByNumber(t3.trainNumber);
    int o3=tm.bookTicket(user->userId,train3->trainId,"PassengerC");
    if(o3>0){qCritical()<<"bookTicket should fail on disabled train";return 1;}
    qDebug()<<"PASS: disabled train correctly blocked";

    // --- Test 6: book with no seats ---
    TrainRecord t4; t4.trainNumber=uniq("T4"); t4.departureStationId=dep->stationId;
    t4.arrivalStationId=arr->stationId; t4.departureTime="14:00"; t4.arrivalTime="18:00";
    t4.totalSeats=0; t4.remainingSeats=0; t4.enabled=true;
    if(!db.addTrain(t4)){qCritical()<<"addTrain 4 failed";return 1;}
    auto train4=db.findTrainByNumber(t4.trainNumber);
    int o4=tm.bookTicket(user->userId,train4->trainId,"PassengerD");
    if(o4>0){qCritical()<<"bookTicket should fail on zero-seat train";return 1;}
    qDebug()<<"PASS: sold-out train correctly blocked";

    qDebug()<<"\nAll ticket API smoke tests PASSED";
    return 0;
}
