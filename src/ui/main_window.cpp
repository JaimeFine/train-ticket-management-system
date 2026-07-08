/**
 * @file main_window.cpp - Issue 9 shared interface change
 *
 * Changes: embed TicketWindow, init DatabaseManager with new API, seed test data.
 */
#include "main_window.h"
#include "database_manager.h"
#include "ticket_manager.h"
#include "statistics_manager.h"
#include "ticket_window.h"

#include <QLabel>
#include <QStatusBar>
#include <QVBoxLayout>
#include <QMessageBox>
#include <QDebug>

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent)
{
    setWindowTitle("火车票务管理系统 v1.0");
    resize(1280, 820);
    setMinimumSize(1024, 680);
    setStyleSheet("QMainWindow { background: #f0f2f5; }");

    initDatabase();
    insertTestData();
    setupUi();

    auto *sl = new QLabel("  系统就绪  |  数据库: train_ticket.db  |  当前用户: 售票员 (seller1)");
    sl->setStyleSheet("font-size:14px; color:#475569; padding:6px 12px;"
                      "background:#e2e8f0; border-top:2px solid #cbd5e1;");
    sl->setMinimumHeight(36);
    statusBar()->addPermanentWidget(sl, 1);
}

MainWindow::~MainWindow() = default;

// ══════════════════════════════════════════════════════════════════════════════
// Database init — using PM's new initialize() API
// ══════════════════════════════════════════════════════════════════════════════

void MainWindow::initDatabase()
{
    m_db = std::make_unique<DatabaseManager>();
    if (!m_db->initialize()) {
        QMessageBox::critical(this, "数据库错误", "无法初始化数据库: " + m_db->lastError());
        return;
    }
    m_ticketMgr = std::make_unique<TicketManager>(*m_db);
    m_statsMgr  = std::make_unique<StatisticsManager>(*m_db);
}

// ══════════════════════════════════════════════════════════════════════════════
// Test data — using PM's typed API (addStation / addTrain / addUser)
// ══════════════════════════════════════════════════════════════════════════════

void MainWindow::insertTestData()
{
    // Stations
    const char *stationNames[] = {
        "北京","上海","广州","深圳","杭州","南京","武汉","成都",
        "重庆","西安","郑州","长沙","天津","苏州","昆明","哈尔滨",
        "沈阳","大连","青岛","厦门"
    };
    for (auto name : stationNames) {
        StationRecord s; s.stationName = name;
        m_db->addStation(s); // ignores duplicates
    }

    // Helper: find stationId
    auto sid = [&](const char *name) {
        auto s = m_db->findStationByName(name);
        return s ? s->stationId : 0;
    };

    // Trains
    struct Seed { const char *num, *dep, *arr, *dTime, *aTime; int total, rem; };
    const Seed seeds[] = {
        {"G101","北京","上海","2026-07-07 08:00","2026-07-07 12:30",200,185},
        {"G102","上海","北京","2026-07-07 09:00","2026-07-07 13:30",200,192},
        {"G201","广州","深圳","2026-07-07 07:30","2026-07-07 08:15",150,140},
        {"G301","北京","广州","2026-07-07 10:00","2026-07-07 18:00",300,267},
        {"G302","广州","北京","2026-07-07 11:00","2026-07-07 19:00",300,289},
        {"G401","武汉","成都","2026-07-07 08:30","2026-07-07 13:00",180,156},
        {"G501","杭州","南京","2026-07-07 07:00","2026-07-07 08:30",120,98},
        {"G601","西安","郑州","2026-07-07 09:30","2026-07-07 11:40",160,142},
        {"D101","上海","杭州","2026-07-08 06:30","2026-07-08 07:45",100,3},
        {"D201","北京","天津","2026-07-08 07:00","2026-07-08 07:35",80,0},
        {"G701","成都","重庆","2026-07-08 08:00","2026-07-08 09:30",120,55},
        {"G801","南京","武汉","2026-07-08 10:00","2026-07-08 13:30",200,120},
    };
    for (auto &s : seeds) {
        TrainRecord t;
        t.trainNumber = s.num;
        t.departureStationId = sid(s.dep); t.arrivalStationId = sid(s.arr);
        t.departureTime = s.dTime; t.arrivalTime = s.aTime;
        t.totalSeats = s.total; t.remainingSeats = s.rem;
        t.enabled = true;
        m_db->addTrain(t);
    }

    // Users (seed if not exists)
    UserRecord seller; seller.username = "seller1"; seller.password = "123456"; seller.role = 1; seller.enabled = true;
    m_db->addUser(seller);
    UserRecord guest; guest.username = "guest"; guest.password = ""; guest.role = 0; guest.enabled = true;
    m_db->addUser(guest);

    qDebug() << "MainWindow: test data seeded";
}

// ══════════════════════════════════════════════════════════════════════════════
// UI
// ══════════════════════════════════════════════════════════════════════════════

void MainWindow::setupUi()
{
    auto *cw = new QWidget(this);
    auto *lo = new QVBoxLayout(cw);
    lo->setContentsMargins(4, 4, 4, 4);

    m_ticketWindow = new TicketWindow(*m_ticketMgr, *m_statsMgr, 1, cw);
    lo->addWidget(m_ticketWindow);
    setCentralWidget(cw);
}
