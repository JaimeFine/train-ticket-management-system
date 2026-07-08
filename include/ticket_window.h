#pragma once

#include <QWidget>
#include <QTableWidget>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>
#include <QSpinBox>
#include <QTabWidget>
#include <QVBoxLayout>
#include <QGroupBox>
#include <QMessageBox>
#include <QVector>

class TicketManager;
class StatisticsManager;

/**
 * @brief 售票系统主界面 — 包含订票、退改签、统计三个页签
 *
 * 遵循架构规则：此 UI 类不直接写 SQL，所有操作通过 TicketManager/StatisticsManager
 */
class TicketWindow : public QWidget
{
    Q_OBJECT

public:
    explicit TicketWindow(TicketManager &ticketMgr,
                          StatisticsManager &statsMgr,
                          int currentUserId = 1,
                          QWidget *parent = nullptr);

    void setCurrentUser(int userId, const QString &userName = "");
    void refreshAll();

private slots:
    // 订票页签
    void onSearchTrains();
    void onBookTicket();

    // 退改签页签
    void onSearchOrders();
    void onRefundOrder();
    void onChangeTicket();

    // 统计页签
    void onRefreshStatistics();

private:
    void setupUi();
    QWidget* createBookingTab();
    QWidget* createRefundChangeTab();
    QWidget* createStatisticsTab();

    // ── 成员 ─────────────────────────────────────────────────────
    TicketManager    &m_ticketMgr;
    StatisticsManager &m_statsMgr;
    int               m_currentUserId = 1;

    // ── 订票页签控件 ─────────────────────────────────────────────
    QLineEdit    *m_depStationEdit  = nullptr;
    QLineEdit    *m_arrStationEdit  = nullptr;
    QLineEdit    *m_dateEdit        = nullptr;
    QLineEdit    *m_trainNumberEdit = nullptr;
    QTableWidget *m_trainTable      = nullptr;
    QLineEdit    *m_passengerEdit   = nullptr;
    QPushButton  *m_bookBtn         = nullptr;

    // ── 退改签页签控件 ───────────────────────────────────────────
    QLineEdit    *m_orderSearchEdit = nullptr;
    QTableWidget *m_orderTable      = nullptr;
    QPushButton  *m_refundBtn       = nullptr;
    QLineEdit    *m_newTrainEdit    = nullptr;
    QPushButton  *m_changeBtn       = nullptr;

    // ── 统计页签控件 ─────────────────────────────────────────────
    QTableWidget      *m_orderHistoryTable = nullptr;
    QTableWidget      *m_routesTable       = nullptr;
    QTableWidget      *m_monthlyTable    = nullptr;
    QVector<QLabel*>   m_overviewCards;
};
