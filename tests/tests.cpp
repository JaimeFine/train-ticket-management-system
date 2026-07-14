#include <QtTest/QtTest>
#include <QTemporaryDir>
#include <QSqlDatabase>
#include <QSqlQuery>

#include "train_manager.h"
#include "ticket_manager.h"
#include "login_manager.h"
#include "statistics_manager.h"
#include "database_manager.h"

//清理数据库状态
void clearDatabaseTables(DatabaseManager *db) {
	QSqlQuery q(db->getDatabase()); 
	q.exec("DELETE FROM OperationLog");
	q.exec("DELETE FROM TrainOrder");
	q.exec("DELETE FROM Train");
	q.exec("DELETE FROM Station");
	q.exec("DELETE FROM User");
	// 重置自增ID
	q.exec("DELETE FROM sqlite_sequence WHERE name='Train'");
	q.exec("DELETE FROM sqlite_sequence WHERE name='TrainOrder'");
	q.exec("DELETE FROM sqlite_sequence WHERE name='User'");
	q.exec("DELETE FROM sqlite_sequence WHERE name='Station'");
}

class TestBusinessLogic : public QObject
{
	Q_OBJECT
	
private slots:
	void initTestCase();
	void cleanupTestCase();
	void init();
	void cleanup();
	
	// --- 测试用例声明 ---
	void test_TrainManager_CRUD();
	void test_TrainManager_Validation();
	void test_TicketManager_BookingFlow();
	void test_TicketManager_RefundAndChange();
	void test_LoginManager_Authentication();
	void test_LoginManager_AccountManagement();
	void test_StatisticsManager_BasicStats();
	
private:
	QTemporaryDir *m_tempDir;
	DatabaseManager *m_db;
	TrainManager *m_trainMgr;
	TicketManager *m_ticketMgr;
	LoginManager *m_loginMgr;
	StatisticsManager *m_statsMgr;
};

void TestBusinessLogic::initTestCase()
{
	// 1. 创建临时目录用于存放测试数据库
	m_tempDir = new QTemporaryDir();
	QVERIFY2(m_tempDir->isValid(), "Failed to create temporary directory");
	
	// 2. 初始化数据库管理器
	m_db = new DatabaseManager(m_tempDir->path() + "/test_db.sqlite");
	QVERIFY(m_db->initialize()); 
	
	// 3. 初始化业务管理器
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
	delete m_tempDir;
}

void TestBusinessLogic::init()
{
	// 每个测试用例执行前，清空数据，保证环境纯净
	clearDatabaseTables(m_db);
	
	// 插入基础数据：一个车站，一个管理员，一个普通用户
	StationRecord s; s.stationName = "测试站"; m_db->addStation(s);
	
	UserRecord admin; admin.username = "admin"; admin.password = "123456"; admin.role = 2; admin.enabled = true;
	m_db->addUser(admin);
	
	UserRecord user; user.username = "passenger"; user.password = "123456"; user.role = 3; user.enabled = true;
	m_db->addUser(user);
}


// ==========================================
// 1. 车次管理测试 (TrainManager)
// ==========================================
void TestBusinessLogic::test_TrainManager_CRUD()
{
	Train t;
	t.trainNumber = "G1001";
	t.departureStationId = 1;
	t.arrivalStationId = 1; // 简化测试
	t.departureTime = "2026-07-14 08:00";
	t.arrivalTime = "2026-07-14 12:00";
	t.totalSeats = 100;
	t.remainingSeats = 100;
	t.enabled = true;
	
	// 1. 添加车次
	QVERIFY(m_trainMgr->addTrain(t));
	auto trains = m_trainMgr->getAllTrains(false);
	QCOMPARE(trains.size(), 1);
	QCOMPARE(trains[0].trainNumber, QString("G1001"));
	
	// 2. 更新车次
	t.trainId = trains[0].trainId;
	t.totalSeats = 120;
	QVERIFY(m_trainMgr->updateTrain(t));
	auto updated = m_trainMgr->getAllTrains(false)[0];
	QCOMPARE(updated.totalSeats, 120);
	
	// 3. 逻辑删除 (停运)
	QVERIFY(m_trainMgr->deleteTrain(t.trainId));
	auto activeTrains = m_trainMgr->getAllTrains(true);
	QCOMPARE(activeTrains.size(), 0);
}

void TestBusinessLogic::test_TrainManager_Validation()
{
	Train t;
	t.trainNumber = ""; // 无效：空车次号
	QVERIFY(!m_trainMgr->addTrain(t));
	QVERIFY(m_trainMgr->statusMessage().contains("车次号不能为空"));
	
	t.trainNumber = "G1002";
	t.departureTime = "2026-07-14 12:00";
	t.arrivalTime = "2026-07-14 08:00"; // 无效：到达早于出发
	QVERIFY(!m_trainMgr->addTrain(t));
	QVERIFY(m_trainMgr->statusMessage().contains("出发时间必须早于到达时间"));
}


// ==========================================
// 2. 票务处理测试 (TicketManager)
// ==========================================
void TestBusinessLogic::test_TicketManager_BookingFlow()
{
	// 准备车次
	Train t;
	t.trainNumber = "T1"; t.departureStationId = 1; t.arrivalStationId = 1;
	t.departureTime = "2026-07-15 10:00"; t.arrivalTime = "2026-07-15 12:00";
	t.totalSeats = 1; t.remainingSeats = 1; t.enabled = true;
	m_trainMgr->addTrain(t);
	int trainId = m_trainMgr->getAllTrains(false)[0].trainId;
	
	// 1. 订票
	int orderId = m_ticketMgr->bookTicket(2, trainId, "张三"); // 用户ID 2 是 passenger
	QVERIFY(orderId > 0);
	
	// 2. 检查余票是否减少
	QCOMPARE(m_ticketMgr->remainingSeats(trainId), 0);
	
	// 3. 再次订票应失败 (无票)
	int failId = m_ticketMgr->bookTicket(2, trainId, "李四");
	QCOMPARE(failId, -1);
	QVERIFY(m_ticketMgr->lastError().contains("无余票"));
}

void TestBusinessLogic::test_TicketManager_RefundAndChange()
{
	// 准备车次 A (原车次) 和 B (改签车次)
	Train t1, t2;
	t1.trainNumber = "A1"; t1.departureStationId = 1; t1.arrivalStationId = 1;
	t1.departureTime = "2026-07-15 10:00"; t1.arrivalTime = "2026-07-15 12:00";
	t1.totalSeats = 1; t1.remainingSeats = 1; t1.enabled = true;
	m_trainMgr->addTrain(t1);
	int trainIdA = m_trainMgr->getAllTrains(false)[0].trainId;
	
	t2.trainNumber = "B1"; t2.departureStationId = 1; t2.arrivalStationId = 1;
	t2.departureTime = "2026-07-15 14:00"; t2.arrivalTime = "2026-07-15 16:00";
	t2.totalSeats = 10; t2.remainingSeats = 10; t2.enabled = true;
	m_trainMgr->addTrain(t2);
	int trainIdB = m_trainMgr->getAllTrains(false)[1].trainId;
	
	// 订票
	int orderId = m_ticketMgr->bookTicket(2, trainIdA, "张三");
	QVERIFY(orderId > 0);
	
	// 1. 退票测试
	QVERIFY(m_ticketMgr->refundTicket(orderId));
	auto orders = m_ticketMgr->queryOrderByOrderId(orderId);
	QCOMPARE(orders[0]["status"].toInt(), 1); // 状态应为已退票
	
	// 2. 改签测试 (重新订票再改签)
	int newOrderId = m_ticketMgr->bookTicket(2, trainIdA, "张三");
	QVERIFY(m_ticketMgr->changeTicket(newOrderId, trainIdB));
	
	// 检查旧订单状态
	auto oldOrder = m_ticketMgr->queryOrderByOrderId(newOrderId)[0];
	QCOMPARE(oldOrder["status"].toInt(), 2); // 已改签
	// 检查新车次余票
	QCOMPARE(m_ticketMgr->remainingSeats(trainIdB), 9);
}


// ==========================================
// 3. 登录认证测试 (LoginManager)
// ==========================================
void TestBusinessLogic::test_LoginManager_Authentication()
{
	// 1. 正确登录
	LoginResult res = m_loginMgr->authenticate("passenger", "123456");
	QVERIFY(res.success);
	QCOMPARE(res.role, UserRole::User);
	
	// 2. 密码错误
	LoginResult res2 = m_loginMgr->authenticate("passenger", "wrong");
	QVERIFY(!res2.success);
	QVERIFY(res2.message.contains("无效"));
	
	// 3. 游客登录
	LoginResult guest = m_loginMgr->loginAsGuest();
	QVERIFY(guest.success);
	QCOMPARE(guest.role, UserRole::Guest);
}

void TestBusinessLogic::test_LoginManager_AccountManagement()
{
	// 1. 用户注册
	AccountResult reg = m_loginMgr->registerUser("newuser", "654321");
	QVERIFY(reg.success);
	QVERIFY(m_loginMgr->authenticate("newuser", "654321").success);
	
	// 2. 重复注册
	AccountResult reg2 = m_loginMgr->registerUser("newuser", "111");
	QVERIFY(!reg2.success);
	QVERIFY(reg2.message.contains("已存在"));
	
	// 3. 修改密码
	AccountResult change = m_loginMgr->changeOwnPassword("newuser", UserRole::User, "654321", "888888");
	QVERIFY(change.success);
	QVERIFY(!m_loginMgr->authenticate("newuser", "654321").success); // 旧密码失效
	QVERIFY(m_loginMgr->authenticate("newuser", "888888").success);  // 新密码生效
}


// ==========================================
// 4. 数据统计测试 (StatisticsManager)
// ==========================================
void TestBusinessLogic::test_StatisticsManager_BasicStats()
{
	// 准备数据
	Train t; t.trainNumber = "S1"; t.departureStationId = 1; t.arrivalStationId = 1;
	t.departureTime = "2026-07-15 10:00"; t.arrivalTime = "2026-07-15 12:00";
	t.totalSeats = 10; t.remainingSeats = 10; t.enabled = true;
	m_trainMgr->addTrain(t);
	int trainId = m_trainMgr->getAllTrains(false)[0].trainId;
	
	// 产生订单
	m_ticketMgr->bookTicket(2, trainId, "张三");
	m_ticketMgr->bookTicket(2, trainId, "李四");
	
	// 1. 检查总数
	QCOMPARE(m_statsMgr->totalOrders(), 2);
	QCOMPARE(m_statsMgr->totalBooked(), 2);
	
	// 2. 退票后检查统计
	auto orders = m_ticketMgr->queryOrdersByUser(2);
	m_ticketMgr->refundTicket(orders[0]["orderId"].toInt());
	
	QCOMPARE(m_statsMgr->totalRefunded(), 1);
	QCOMPARE(m_statsMgr->totalBooked(), 1);
}

QTEST_MAIN(TestBusinessLogic)
#include "tst_businesslogic.moc"
