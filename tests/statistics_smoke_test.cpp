/**
 * @file statistics_smoke_test.cpp - V2 smoke test: Trip-based booking + statistics
 */
#include "database_manager.h"
#include "ticket_manager.h"
#include "statistics_manager.h"

#include <QCoreApplication>
#include <QDateTime>
#include <QDebug>
#include <QRandomGenerator>

// 生成带时间戳+随机数的唯一名称，避免多次运行测试时数据冲突
static QString uniq(const QString &prefix) {
    return QStringLiteral("%1_%2_%3")
        .arg(prefix)
        .arg(QDateTime::currentMSecsSinceEpoch())
        .arg(QRandomGenerator::global()->bounded(10000));
}

// 冒烟测试主流程：建数据→购票/退票→校验统计与座位，任一步失败即返回1
int main(int argc, char *argv[]) {
    QCoreApplication app(argc, argv);

    // 测试前置：数据库必须能正常初始化
    DatabaseManager db;
    if (!db.initialize()) { qCritical() << "FAIL: DB init:" << db.lastError(); return 1; }

    // ── 创建测试用户并验证可查回 ──────────────────────────────────
    UserRecord user; user.username=uniq("v2_user"); user.password="pw"; user.role=3; user.enabled=true;
    if (!db.addUser(user)) { qCritical()<<"FAIL: addUser:"<<db.lastError(); return 1; }
    auto u = db.findUserByUsername(user.username);
    if (!u) { qCritical()<<"FAIL: findUser"; return 1; }
    qDebug()<<"OK: user"<<u->userId;

    // ── 创建出发/到达车站和车次，作为购票的基础数据 ────────────────
    StationRecord s1,s2;
    s1.stationName=uniq("V2_Dep"); s2.stationName=uniq("V2_Arr");
    if (!db.addStation(s1)||!db.addStation(s2)) { qCritical()<<"FAIL: addStation"; return 1; }
    auto dep=db.findStationByName(s1.stationName), arr=db.findStationByName(s2.stationName);
    if (!dep||!arr) { qCritical()<<"FAIL: findStation"; return 1; }

    TrainRecord tr;
    tr.trainNumber=uniq("V2_G");
    tr.departureStationId=dep->stationId; tr.arrivalStationId=arr->stationId;
    tr.departureTime="08:00"; tr.arrivalTime="12:00";
    tr.totalSeats=50; tr.enabled=true;
    if (!db.addTrain(tr)) { qCritical()<<"FAIL: addTrain:"<<db.lastError(); return 1; }
    auto train=db.findTrainByNumber(tr.trainNumber);
    if (!train) { qCritical()<<"FAIL: findTrain"; return 1; }
    qDebug()<<"OK: train"<<train->trainId;

    // ── V2：为今天创建班次(Trip)，座位挂在Trip而非Train上 ──────────
    const QString today = QDateTime::currentDateTime().toString("yyyy-MM-dd");
    auto tripId = db.createTrip(train->trainId, today, train->totalSeats);
    if (!tripId) { qCritical()<<"FAIL: createTrip:"<<db.lastError(); return 1; }
    auto trip = db.findTripById(*tripId);
    if (!trip) { qCritical()<<"FAIL: findTrip"; return 1; }
    qDebug()<<"OK: trip"<<trip->tripId<<"date"<<trip->travelDate
            <<"seats"<<trip->totalSeats<<"/"<<trip->remainingSeats;

    // ── 构造业务管理器，并记录统计基线（后面只校验增量） ────────────
    TicketManager tm(db);
    StatisticsManager sm(db);

    int baseOrders=sm.totalOrders(), baseBooked=sm.totalBooked();
    int baseRefunded=sm.totalRefunded(), baseChanged=sm.totalChanged();
    if (baseOrders<0||baseBooked<0||baseRefunded<0||baseChanged<0)
        { qCritical()<<"FAIL: negative baseline"; return 1; }
    qDebug()<<"OK: baselines"<<baseOrders<<baseBooked<<baseRefunded<<baseChanged;

    // ── 购3张票、退1张（V2：按tripId购票），为统计校验造数据 ────────
    int o1=tm.bookTicket(u->userId, trip->tripId, "P_A");
    int o2=tm.bookTicket(u->userId, trip->tripId, "P_B");
    int o3=tm.bookTicket(u->userId, trip->tripId, "P_C");
    if (o1<=0||o2<=0||o3<=0) { qCritical()<<"FAIL: bookTicket"<<o1<<o2<<o3<<tm.lastError(); return 1; }
    qDebug()<<"OK: booked"<<o1<<o2<<o3;

    if (!tm.refundTicket(o3)) { qCritical()<<"FAIL: refund"<<tm.lastError(); return 1; }
    qDebug()<<"OK: refunded"<<o3;

    // ── 校验统计增量：总订单+3、已订+2、已退+1、已改签+0 ────────────
    if (sm.totalOrders()  -baseOrders  !=3) { qCritical()<<"FAIL: totalOrders delta"  <<sm.totalOrders() -baseOrders; return 1; }
    if (sm.totalBooked()  -baseBooked  !=2) { qCritical()<<"FAIL: totalBooked delta"  <<sm.totalBooked() -baseBooked; return 1; }
    if (sm.totalRefunded()-baseRefunded!=1) { qCritical()<<"FAIL: totalRefunded delta"<<sm.totalRefunded()-baseRefunded; return 1; }
    if (sm.totalChanged() -baseChanged !=0) { qCritical()<<"FAIL: totalChanged delta" <<sm.totalChanged() -baseChanged; return 1; }
    qDebug()<<"PASS: stats deltas 3/2/1/0";

    // ── 校验热门路线：本测试路线应出现且有效订单数为2（退票不计） ──
    auto routes=sm.popularRoutes();
    if (routes.isEmpty()) { qCritical()<<"FAIL: popularRoutes empty"; return 1; }
    bool found=false;
    for (auto &r:routes) {
        if (r["departureStation"].toString()==s1.stationName) {
            if (r["orderCount"].toInt()!=2) { qCritical()<<"FAIL: route cnt"; return 1; }
            found=true; qDebug()<<"PASS: popularRoutes"; break;
        }
    }
    if (!found) { qCritical()<<"FAIL: route not found"; return 1; }

    // ── 校验月度客流：近几个月订单合计至少包含本次的3单 ────────────
    auto monthly=sm.monthlyPassengerFlow();
    if (monthly.isEmpty()) { qCritical()<<"FAIL: monthly empty"; return 1; }
    int sum=0;
    for (int i=0; i<qMin(3,monthly.size()); ++i) sum+=monthly[i]["totalOrders"].toInt();
    if (sum<3) { qCritical()<<"FAIL: monthly too low"<<sum; return 1; }
    qDebug()<<"PASS: monthlyPassengerFlow"<<monthly.size()<<"months sum="<<sum;

    // ── 校验queryAllOrders：能查到3位乘客且每条含V2新增字段 ────────
    auto all=tm.queryAllOrders();
    int foundABC=0;
    for (auto &o:all) {
        QString n=o["passengerName"].toString();
        if (n=="P_A"||n=="P_B"||n=="P_C") foundABC++;
        if (!o.contains("travelDate")||!o.contains("tripId"))
            { qCritical()<<"FAIL: missing V2 keys"; return 1; }
    }
    if (foundABC<3) { qCritical()<<"FAIL: missing passengers"<<foundABC; return 1; }
    qDebug()<<"PASS: queryAllOrders"<<all.size()<<"V2 keys OK";

    // ── 校验班次余票：购3退1后应为总座位-2 ─────────────────────────
    auto t2=db.findTripById(trip->tripId);
    if (!t2||t2->remainingSeats!=trip->totalSeats-2)
        { qCritical()<<"FAIL: trip seats"<< (t2?t2->remainingSeats:-1); return 1; }
    qDebug()<<"PASS: trip seats"<<t2->remainingSeats<<"/"<<t2->totalSeats;

    qDebug()<<"\nAll V2 smoke tests PASSED";
    return 0;
}
