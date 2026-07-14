#include <QtTest/QtTest>
#include <QSqlDatabase>
#include <QSqlQuery>

#include "database_manager.h"
#include "login_manager.h"
#include "statistics_manager.h"
#include "ticket_manager.h"
#include "train_manager.h"

namespace {
void clearDatabaseTables()
{
    QSqlQuery query(QSqlDatabase::database(QStringLiteral("train_ticket_connection")));
    query.exec(QStringLiteral("DELETE FROM OperationLog"));
    query.exec(QStringLiteral("DELETE FROM \"Order\""));
    query.exec(QStringLiteral("DELETE FROM Train"));
    query.exec(QStringLiteral("DELETE FROM Station"));
    query.exec(QStringLiteral("DELETE FROM User"));
    query.exec(QStringLiteral("DELETE FROM sqlite_sequence"));
}
}

class TestBusinessLogic : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();
    void init();

    void test_TrainManager_CRUDAndStatusControl();
    void test_TicketManager_SearchBookingRefundChange();
    void test_LoginManager_AuthenticationAndAccountManagement();
    void test_StatisticsAndOperationLogs();

    void test_Deferred_DynamicPricing();
    void test_Deferred_RouteRecommendation();
    void test_Deferred_DateDrivenModel();

private:
    int userId(const QString &username) const;
    int stationId(const QString &stationName) const;

    DatabaseManager *m_db = nullptr;
    TrainManager *m_trainMgr = nullptr;
    TicketManager *m_ticketMgr = nullptr;
    LoginManager *m_loginMgr = nullptr;
    StatisticsManager *m_statsMgr = nullptr;
};

void TestBusinessLogic::initTestCase()
{
    m_db = new DatabaseManager();
    QVERIFY2(m_db->initialize(), qPrintable(m_db->lastError()));

    m_trainMgr = new TrainManager(m_db);
    m_ticketMgr = new TicketManager(*m_db);
    m_loginMgr = new LoginManager(m_db);
    m_statsMgr = new StatisticsManager(*m_db);
}

void TestBusinessLogic::cleanupTestCase()
{
    delete m_trainMgr;
    delete m_ticketMgr;
    delete m_loginMgr;
    delete m_statsMgr;
    delete m_db;
}

void TestBusinessLogic::init()
{
    clearDatabaseTables();

    StationRecord s1;
    s1.stationName = QStringLiteral("北京南");
    QVERIFY(m_db->addStation(s1));

    StationRecord s2;
    s2.stationName = QStringLiteral("上海虹桥");
    QVERIFY(m_db->addStation(s2));

    StationRecord s3;
    s3.stationName = QStringLiteral("南京南");
    QVERIFY(m_db->addStation(s3));

    UserRecord admin;
    admin.username = QStringLiteral("admin");
    admin.password = QStringLiteral("123456");
    admin.role = 2;
    admin.enabled = true;
    QVERIFY(m_db->addUser(admin));

    UserRecord seller;
    seller.username = QStringLiteral("seller");
    seller.password = QStringLiteral("123456");
    seller.role = 1;
    seller.enabled = true;
    QVERIFY(m_db->addUser(seller));

    UserRecord user;
    user.username = QStringLiteral("passenger");
    user.password = QStringLiteral("123456");
    user.role = 3;
    user.enabled = true;
    QVERIFY(m_db->addUser(user));
}

void TestBusinessLogic::test_TrainManager_CRUDAndStatusControl()
{
    Train train;
    train.trainNumber = QStringLiteral("G1001");
    train.departureStationId = stationId(QStringLiteral("北京南"));
    train.arrivalStationId = stationId(QStringLiteral("上海虹桥"));
    train.departureTime = QStringLiteral("2026-07-14 08:00");
    train.arrivalTime = QStringLiteral("2026-07-14 12:00");
    train.totalSeats = 100;
    train.remainingSeats = 100;
    train.enabled = true;

    QVERIFY(m_trainMgr->addTrain(train));
    auto trains = m_trainMgr->getAllTrains(false);
    QCOMPARE(trains.size(), 1);
    QCOMPARE(trains[0].enabled, true);

    train.trainId = trains[0].trainId;
    train.totalSeats = 120;
    train.remainingSeats = 120;
    QVERIFY(m_trainMgr->updateTrain(train));
    QCOMPARE(m_trainMgr->getAllTrains(false)[0].totalSeats, 120);

    QVERIFY(m_trainMgr->deleteTrain(train.trainId));
    QCOMPARE(m_trainMgr->getAllTrains(true).size(), 0);
    QCOMPARE(m_trainMgr->getAllTrains(false)[0].enabled, false);

    QVERIFY(m_trainMgr->resumeTrain(train.trainId));
    QCOMPARE(m_trainMgr->getAllTrains(true).size(), 1);
    QCOMPARE(m_trainMgr->getAllTrains(false)[0].enabled, true);

    QVERIFY(m_trainMgr->deleteTrainPermanently(train.trainId));
    QCOMPARE(m_trainMgr->getAllTrains(false).size(), 0);
}

void TestBusinessLogic::test_TicketManager_SearchBookingRefundChange()
{
    Train t1;
    t1.trainNumber = QStringLiteral("A1");
    t1.departureStationId = stationId(QStringLiteral("北京南"));
    t1.arrivalStationId = stationId(QStringLiteral("上海虹桥"));
    t1.departureTime = QStringLiteral("2026-07-15 10:00");
    t1.arrivalTime = QStringLiteral("2026-07-15 12:00");
    t1.totalSeats = 1;
    t1.remainingSeats = 1;
    t1.enabled = true;
    QVERIFY(m_trainMgr->addTrain(t1));

    Train t2;
    t2.trainNumber = QStringLiteral("B1");
    t2.departureStationId = stationId(QStringLiteral("北京南"));
    t2.arrivalStationId = stationId(QStringLiteral("南京南"));
    t2.departureTime = QStringLiteral("2026-07-15 14:00");
    t2.arrivalTime = QStringLiteral("2026-07-15 16:00");
    t2.totalSeats = 10;
    t2.remainingSeats = 10;
    t2.enabled = true;
    QVERIFY(m_trainMgr->addTrain(t2));

    const auto trains = m_trainMgr->getAllTrains(false);
    QCOMPARE(trains.size(), 2);
    const int trainIdA = trains[0].trainId;
    const int trainIdB = trains[1].trainId;

    const auto searchResults = m_ticketMgr->searchTrains(QStringLiteral("北京南"),
                                                         QStringLiteral("上海虹桥"));
    QCOMPARE(searchResults.size(), 1);
    QCOMPARE(searchResults[0].value(QStringLiteral("trainNumber")).toString(),
             QStringLiteral("A1"));

    const int passengerId = userId(QStringLiteral("passenger"));
    const int orderId = m_ticketMgr->bookTicket(passengerId, trainIdA, QStringLiteral("张三"));
    QVERIFY(orderId > 0);
    QCOMPARE(m_ticketMgr->remainingSeats(trainIdA), 0);

    const int failId = m_ticketMgr->bookTicket(passengerId, trainIdA, QStringLiteral("李四"));
    QCOMPARE(failId, -1);
    QVERIFY(m_ticketMgr->lastError().contains(QStringLiteral("无余票")));

    QVERIFY(m_ticketMgr->refundTicket(orderId));
    auto refunded = m_ticketMgr->queryOrderByOrderId(orderId);
    QCOMPARE(refunded.size(), 1);
    QCOMPARE(refunded[0].value(QStringLiteral("status")).toInt(), 1);

    const int newOrderId = m_ticketMgr->bookTicket(passengerId, trainIdA, QStringLiteral("张三"));
    QVERIFY(newOrderId > 0);
    QVERIFY(m_ticketMgr->changeTicket(newOrderId, trainIdB));

    auto changed = m_ticketMgr->queryOrderByOrderId(newOrderId);
    QCOMPARE(changed[0].value(QStringLiteral("status")).toInt(), 2);
    QCOMPARE(m_ticketMgr->remainingSeats(trainIdB), 9);
}

void TestBusinessLogic::test_LoginManager_AuthenticationAndAccountManagement()
{
    LoginResult login = m_loginMgr->authenticate(QStringLiteral("passenger"),
                                                 QStringLiteral("123456"));
    QVERIFY(login.success);
    QCOMPARE(login.role, UserRole::User);

    LoginResult badLogin = m_loginMgr->authenticate(QStringLiteral("passenger"),
                                                    QStringLiteral("wrong"));
    QVERIFY(!badLogin.success);

    LoginResult guest = m_loginMgr->loginAsGuest();
    QVERIFY(guest.success);
    QCOMPARE(guest.role, UserRole::Guest);

    AccountResult reg = m_loginMgr->registerUser(QStringLiteral("newuser"),
                                                 QStringLiteral("654321"));
    QVERIFY(reg.success);
    QVERIFY(m_loginMgr->authenticate(QStringLiteral("newuser"),
                                     QStringLiteral("654321")).success);

    const int adminId = userId(QStringLiteral("admin"));
    AccountResult createSeller = m_loginMgr->createSellerAccount(adminId,
                                                                 QStringLiteral("seller2"),
                                                                 QStringLiteral("seller654"));
    QVERIFY(createSeller.success);

    SellerAccountListResult sellerList = m_loginMgr->sellerAccounts(adminId);
    QVERIFY(sellerList.success);
    QVERIFY(sellerList.accounts.size() >= 2);

    AccountResult disableSeller = m_loginMgr->setSellerEnabled(adminId,
                                                               QStringLiteral("seller2"),
                                                               false);
    QVERIFY(disableSeller.success);
    LoginResult disabledLogin = m_loginMgr->authenticate(QStringLiteral("seller2"),
                                                         QStringLiteral("seller654"));
    QVERIFY(!disabledLogin.success);

    AccountResult resetSeller = m_loginMgr->resetSellerPasswordToDefault(adminId,
                                                                         QStringLiteral("seller2"));
    QVERIFY(resetSeller.success);
    AccountResult enableSeller = m_loginMgr->setSellerEnabled(adminId,
                                                              QStringLiteral("seller2"),
                                                              true);
    QVERIFY(enableSeller.success);
    QVERIFY(m_loginMgr->authenticate(QStringLiteral("seller2"),
                                     QStringLiteral("123456")).success);

    AccountResult changePassword =
        m_loginMgr->changeOwnPassword(QStringLiteral("newuser"),
                                      UserRole::User,
                                      QStringLiteral("654321"),
                                      QStringLiteral("888888"));
    QVERIFY(changePassword.success);
    QVERIFY(!m_loginMgr->authenticate(QStringLiteral("newuser"),
                                      QStringLiteral("654321")).success);
    QVERIFY(m_loginMgr->authenticate(QStringLiteral("newuser"),
                                     QStringLiteral("888888")).success);
}

void TestBusinessLogic::test_StatisticsAndOperationLogs()
{
    Train train;
    train.trainNumber = QStringLiteral("S1");
    train.departureStationId = stationId(QStringLiteral("北京南"));
    train.arrivalStationId = stationId(QStringLiteral("上海虹桥"));
    train.departureTime = QStringLiteral("2026-07-15 10:00");
    train.arrivalTime = QStringLiteral("2026-07-15 12:00");
    train.totalSeats = 10;
    train.remainingSeats = 10;
    train.enabled = true;
    QVERIFY(m_trainMgr->addTrain(train));

    const int trainId = m_trainMgr->getAllTrains(false)[0].trainId;
    const int passengerId = userId(QStringLiteral("passenger"));

    const int orderId1 = m_ticketMgr->bookTicket(passengerId, trainId, QStringLiteral("张三"));
    const int orderId2 = m_ticketMgr->bookTicket(passengerId, trainId, QStringLiteral("李四"));
    QVERIFY(orderId1 > 0);
    QVERIFY(orderId2 > 0);

    QCOMPARE(m_statsMgr->totalOrders(), 2);
    QCOMPARE(m_statsMgr->totalBooked(), 2);

    QVERIFY(m_ticketMgr->refundTicket(orderId1));
    QCOMPARE(m_statsMgr->totalRefunded(), 1);
    QCOMPARE(m_statsMgr->totalBooked(), 1);

    QVERIFY(m_db->addOperationLog(QStringLiteral("admin"),
                                  QStringLiteral("查看日志"),
                                  QStringLiteral("管理员检查系统日志")));
    const auto logs = m_db->findAllOperationLogs();
    QVERIFY(!logs.isEmpty());
    QCOMPARE(logs[0].action, QStringLiteral("查看日志"));
}

void TestBusinessLogic::test_Deferred_DynamicPricing()
{
    QSKIP("Dynamic pricing is deferred in 分工2.md and is not implemented in the current repo.");
}

void TestBusinessLogic::test_Deferred_RouteRecommendation()
{
    QSKIP("Dijkstra/Floyd route recommendation is deferred in 分工2.md and is not implemented in the current repo.");
}

void TestBusinessLogic::test_Deferred_DateDrivenModel()
{
    QSKIP("Date-driven trip/ticket data model is deferred in 分工2.md and is not implemented in the current repo.");
}

int TestBusinessLogic::userId(const QString &username) const
{
    const auto user = m_db->findUserByUsername(username);
    return user.has_value() ? user->userId : 0;
}

int TestBusinessLogic::stationId(const QString &stationName) const
{
    const auto station = m_db->findStationByName(stationName);
    return station.has_value() ? station->stationId : 0;
}

QTEST_MAIN(TestBusinessLogic)
#include "tests.moc"
