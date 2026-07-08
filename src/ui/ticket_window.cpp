#include "ticket_window.h"
#include "ticket_manager.h"
#include "statistics_manager.h"

#include <QHBoxLayout>
#include <QFormLayout>
#include <QHeaderView>
#include <QGroupBox>
#include <QSplitter>
#include <QMessageBox>
#include <QDateTime>
#include <QApplication>
#include <QStyle>
#include <QFont>

// ── 辅助函数：创建居中对齐的表格项 ──────────────────────────
static QTableWidgetItem* centerItem(const QString &text)
{
    auto *item = new QTableWidgetItem(text);
    item->setTextAlignment(Qt::AlignCenter);
    return item;
}

// ══════════════════════════════════════════════════════════════════════════════════════════════════
// 全局样式表
// ══════════════════════════════════════════════════════════════════════════════
static const char* GLOBAL_STYLE = R"(
    /* ── 全局字体 ── */
    * {
        font-family: "Microsoft YaHei", "PingFang SC", "Helvetica Neue", sans-serif;
        font-size: 15px;
    }

    /* ── 页签栏 ── */
    QTabWidget::pane {
        border: 2px solid #d0d5dd;
        border-radius: 10px;
        background: #f8f9fb;
        padding: 12px;
    }
    QTabBar::tab {
        font-size: 17px;
        font-weight: bold;
        padding: 14px 32px;
        margin-right: 6px;
        border: none;
        border-top-left-radius: 10px;
        border-top-right-radius: 10px;
        background: #e8ecf1;
        color: #5a6270;
        min-width: 140px;
    }
    QTabBar::tab:selected {
        background: qlineargradient(x1:0,y1:0, x2:0,y2:1, stop:0 #4a6cf7, stop:1 #3651d5);
        color: white;
    }
    QTabBar::tab:hover:!selected {
        background: #d5dce6;
        color: #333840;
    }

    /* ── 分组框（卡片风格） ── */
    QGroupBox {
        font-size: 18px;
        font-weight: bold;
        color: #1e293b;
        background: white;
        border: 1px solid #e2e8f0;
        border-radius: 10px;
        margin-top: 22px;
        padding: 28px 18px 18px 18px;
    }
    QGroupBox::title {
        subcontrol-origin: margin;
        left: 20px;
        padding: 6px 18px;
        background: qlineargradient(x1:0,y1:0, x2:0,y2:1, stop:0 #4a6cf7, stop:1 #3651d5);
        color: white;
        border-radius: 6px;
        font-size: 17px;
    }

    /* ── 输入框 ── */
    QLineEdit {
        font-size: 16px;
        padding: 12px 16px;
        border: 2px solid #d0d5dd;
        border-radius: 8px;
        background: #fafbfc;
        min-height: 22px;
    }
    QLineEdit:focus {
        border: 2px solid #4a6cf7;
        background: white;
    }
    QLineEdit:hover:!focus {
        border-color: #a0a8b8;
    }

    /* ── 表格 ── */
    QTableWidget {
        font-size: 15px;
        background: white;
        border: 1px solid #e2e8f0;
        border-radius: 8px;
        gridline-color: #eef0f4;
        alternate-background-color: #f7f9fc;
        selection-background-color: #dce6ff;
        selection-color: #1e293b;
    }
    QTableWidget::item {
        padding: 12px 10px;
    }
    QHeaderView::section {
        font-size: 15px;
        font-weight: bold;
        padding: 14px 10px;
        background: qlineargradient(x1:0,y1:0, x2:0,y2:1, stop:0 #3b4f8c, stop:1 #2a3a6a);
        color: white;
        border: none;
        border-right: 1px solid #4a5fa0;
    }

    /* ── 普通按钮 ── */
    QPushButton {
        font-size: 16px;
        font-weight: bold;
        padding: 12px 28px;
        border: none;
        border-radius: 8px;
        min-height: 22px;
    }

    /* ── 滚动条 ── */
    QScrollBar:vertical {
        width: 12px;
        background: #f0f2f5;
        border-radius: 6px;
    }
    QScrollBar::handle:vertical {
        background: #c2cad6;
        border-radius: 6px;
        min-height: 40px;
    }
    QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { height: 0; }

    /* ── 提示标签 ── */
    QLabel {
        font-size: 16px;
        color: #334155;
    }
)";

// ══════════════════════════════════════════════════════════════════════════════
// 构造函数
// ══════════════════════════════════════════════════════════════════════════════

TicketWindow::TicketWindow(TicketManager &ticketMgr,
                           StatisticsManager &statsMgr,
                           int currentUserId,
                           QWidget *parent)
    : QWidget(parent)
    , m_ticketMgr(ticketMgr)
    , m_statsMgr(statsMgr)
    , m_currentUserId(currentUserId)
{
    setupUi();
}

void TicketWindow::setCurrentUser(int userId, const QString & /*userName*/)
{
    m_currentUserId = userId;
}

void TicketWindow::refreshAll()
{
    onRefreshStatistics();
}

// ══════════════════════════════════════════════════════════════════════════════
// 主布局
// ══════════════════════════════════════════════════════════════════════════════

void TicketWindow::setupUi()
{
    setStyleSheet(GLOBAL_STYLE);

    auto *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(16, 12, 16, 16);
    mainLayout->setSpacing(10);

    auto *tabWidget = new QTabWidget(this);
    tabWidget->setDocumentMode(true);
    tabWidget->addTab(createBookingTab(),      QStringLiteral("  🚆  车票预订  "));
    tabWidget->addTab(createRefundChangeTab(), QStringLiteral("  🔄  退票 / 改签  "));
    tabWidget->addTab(createStatisticsTab(),   QStringLiteral("  📊  订单历史与统计  "));

    mainLayout->addWidget(tabWidget);
}

// ══════════════════════════════════════════════════════════════════════════════
// 搜索/操作按钮样式宏
// ══════════════════════════════════════════════════════════════════════════════
static const char* BTN_PRIMARY = R"(
    QPushButton {
        background: qlineargradient(x1:0,y1:0, x2:0,y2:1, stop:0 #4a6cf7, stop:1 #3651d5);
        color: white;
        font-size: 16px;
        font-weight: bold;
        padding: 12px 28px;
        border-radius: 8px;
        border: none;
    }
    QPushButton:hover { background: qlineargradient(x1:0,y1:0, x2:0,y2:1, stop:0 #5b7af9, stop:1 #4460e3); }
    QPushButton:pressed { background: qlineargradient(x1:0,y1:0, x2:0,y2:1, stop:0 #3651d5, stop:1 #2940b0); }
)";

static const char* BTN_SUCCESS = R"(
    QPushButton {
        background: qlineargradient(x1:0,y1:0, x2:0,y2:1, stop:0 #10b981, stop:1 #059669);
        color: white;
        font-size: 18px;
        font-weight: bold;
        padding: 16px 40px;
        border-radius: 10px;
        border: none;
    }
    QPushButton:hover { background: qlineargradient(x1:0,y1:0, x2:0,y2:1, stop:0 #34d399, stop:1 #10b981); }
    QPushButton:pressed { background: qlineargradient(x1:0,y1:0, x2:0,y2:1, stop:0 #059669, stop:1 #047857); }
)";

static const char* BTN_DANGER = R"(
    QPushButton {
        background: qlineargradient(x1:0,y1:0, x2:0,y2:1, stop:0 #f43f5e, stop:1 #e11d48);
        color: white;
        font-size: 17px;
        font-weight: bold;
        padding: 14px 36px;
        border-radius: 10px;
        border: none;
    }
    QPushButton:hover { background: qlineargradient(x1:0,y1:0, x2:0,y2:1, stop:0 #fb7185, stop:1 #f43f5e); }
    QPushButton:pressed { background: qlineargradient(x1:0,y1:0, x2:0,y2:1, stop:0 #e11d48, stop:1 #be123c); }
)";

static const char* BTN_WARN = R"(
    QPushButton {
        background: qlineargradient(x1:0,y1:0, x2:0,y2:1, stop:0 #f59e0b, stop:1 #d97706);
        color: white;
        font-size: 17px;
        font-weight: bold;
        padding: 14px 36px;
        border-radius: 10px;
        border: none;
    }
    QPushButton:hover { background: qlineargradient(x1:0,y1:0, x2:0,y2:1, stop:0 #fbbf24, stop:1 #f59e0b); }
    QPushButton:pressed { background: qlineargradient(x1:0,y1:0, x2:0,y2:1, stop:0 #d97706, stop:1 #b45309); }
)";

// ══════════════════════════════════════════════════════════════════════════════
// 页签 1：车票预订
// ══════════════════════════════════════════════════════════════════════════════

QWidget* TicketWindow::createBookingTab()
{
    auto *page = new QWidget(this);
    auto *layout = new QVBoxLayout(page);
    layout->setContentsMargins(8, 8, 8, 8);
    layout->setSpacing(14);

    // ── 搜索区域 ─────────────────────────────────────────────────
    auto *searchGroup = new QGroupBox(QStringLiteral("  搜索车次  "), page);
    auto *searchLayout = new QHBoxLayout(searchGroup);
    searchLayout->setSpacing(14);

    // 出发站
    auto *depLabel = new QLabel(QStringLiteral("出发站"), searchGroup);
    depLabel->setStyleSheet("font-weight: bold; font-size: 16px; color: #334155;");
    searchLayout->addWidget(depLabel);
    m_depStationEdit = new QLineEdit(searchGroup);
    m_depStationEdit->setPlaceholderText(QStringLiteral("输入城市名，如：北京"));
    m_depStationEdit->setMinimumWidth(130);
    searchLayout->addWidget(m_depStationEdit);

    // 到达站
    auto *arrLabel = new QLabel(QStringLiteral("到达站"), searchGroup);
    arrLabel->setStyleSheet("font-weight: bold; font-size: 16px; color: #334155;");
    searchLayout->addWidget(arrLabel);
    m_arrStationEdit = new QLineEdit(searchGroup);
    m_arrStationEdit->setPlaceholderText(QStringLiteral("输入城市名，如：上海"));
    m_arrStationEdit->setMinimumWidth(130);
    searchLayout->addWidget(m_arrStationEdit);

    // 日期
    auto *dateLabel = new QLabel(QStringLiteral("日期"), searchGroup);
    dateLabel->setStyleSheet("font-weight: bold; font-size: 16px; color: #334155;");
    searchLayout->addWidget(dateLabel);
    m_dateEdit = new QLineEdit(searchGroup);
    m_dateEdit->setPlaceholderText(QStringLiteral("2026-07-08"));
    m_dateEdit->setMaximumWidth(130);
    searchLayout->addWidget(m_dateEdit);

    // 搜索按钮
    auto *searchBtn = new QPushButton(QStringLiteral("🔍  搜索车次"), searchGroup);
    searchBtn->setStyleSheet(BTN_PRIMARY);
    searchBtn->setCursor(Qt::PointingHandCursor);
    searchLayout->addWidget(searchBtn);
    connect(searchBtn, &QPushButton::clicked, this, &TicketWindow::onSearchTrains);

    // 分隔线
    auto *sep = new QLabel(searchGroup);
    sep->setFixedWidth(2);
    sep->setStyleSheet("background: #d0d5dd; border-radius: 1px; min-height: 40px;");
    searchLayout->addWidget(sep);

    // 车次号快搜
    auto *numLabel = new QLabel(QStringLiteral("车次号"), searchGroup);
    numLabel->setStyleSheet("font-weight: bold; font-size: 16px; color: #334155;");
    searchLayout->addWidget(numLabel);
    m_trainNumberEdit = new QLineEdit(searchGroup);
    m_trainNumberEdit->setPlaceholderText(QStringLiteral("G101"));
    m_trainNumberEdit->setMaximumWidth(100);
    searchLayout->addWidget(m_trainNumberEdit);

    auto *numSearchBtn = new QPushButton(QStringLiteral("快速搜索"), searchGroup);
    numSearchBtn->setStyleSheet(BTN_WARN);
    numSearchBtn->setCursor(Qt::PointingHandCursor);
    searchLayout->addWidget(numSearchBtn);
    connect(numSearchBtn, &QPushButton::clicked, this, [this]() {
        auto trains = m_ticketMgr.searchByTrainNumber(m_trainNumberEdit->text().trimmed());
        m_trainTable->setRowCount(0);
        for (const auto &t : trains) {
            int r = m_trainTable->rowCount();
            m_trainTable->insertRow(r);
            m_trainTable->setItem(r, 0, new QTableWidgetItem(t["trainNumber"].toString()));
            m_trainTable->setItem(r, 1, new QTableWidgetItem(t["departureStation"].toString()));
            m_trainTable->setItem(r, 2, new QTableWidgetItem(t["arrivalStation"].toString()));
            m_trainTable->setItem(r, 3, new QTableWidgetItem(t["departureTime"].toString()));
            m_trainTable->setItem(r, 4, new QTableWidgetItem(t["arrivalTime"].toString()));
            m_trainTable->setItem(r, 5, new QTableWidgetItem(t["remainingSeats"].toString()));
            m_trainTable->setItem(r, 6, new QTableWidgetItem(t["totalSeats"].toString()));
            // 余票高亮
            int remaining = t["remainingSeats"].toInt();
            auto *seatItem = m_trainTable->item(r, 5);
            if (remaining <= 5) {
                seatItem->setForeground(QColor("#e11d48"));
                seatItem->setData(Qt::FontRole, []{ QFont f; f.setBold(true); f.setPointSize(14); return f; }());
            } else if (remaining > 50) {
                seatItem->setForeground(QColor("#059669"));
            }
        }
    });

    layout->addWidget(searchGroup);

    // ── 车次列表 ─────────────────────────────────────────────────
    m_trainTable = new QTableWidget(page);
    m_trainTable->setColumnCount(7);
    m_trainTable->setHorizontalHeaderLabels({
        QStringLiteral("  车次号  "),
        QStringLiteral("  出发站  "),
        QStringLiteral("  到达站  "),
        QStringLiteral("  出发时间  "),
        QStringLiteral("  到达时间  "),
        QStringLiteral("  余票  "),
        QStringLiteral("  总座位  ")
    });
    m_trainTable->horizontalHeader()->setStretchLastSection(true);
    m_trainTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    m_trainTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_trainTable->setSelectionMode(QAbstractItemView::SingleSelection);
    m_trainTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_trainTable->setAlternatingRowColors(true);
    m_trainTable->verticalHeader()->setDefaultSectionSize(48);
    m_trainTable->setShowGrid(false);
    layout->addWidget(m_trainTable, 1);

    // ── 订票操作 ─────────────────────────────────────────────────
    auto *bookGroup = new QGroupBox(QStringLiteral("  预订车票  "), page);
    auto *bookLayout = new QHBoxLayout(bookGroup);
    bookLayout->setSpacing(18);

    auto *passLabel = new QLabel(QStringLiteral("乘客姓名"), bookGroup);
    passLabel->setStyleSheet("font-weight: bold; font-size: 17px; color: #334155;");
    bookLayout->addWidget(passLabel);

    m_passengerEdit = new QLineEdit(bookGroup);
    m_passengerEdit->setPlaceholderText(QStringLiteral("请输入乘客真实姓名"));
    m_passengerEdit->setMinimumWidth(200);
    m_passengerEdit->setStyleSheet("font-size: 17px; padding: 14px 18px;");
    bookLayout->addWidget(m_passengerEdit);

    bookLayout->addStretch();

    m_bookBtn = new QPushButton(QStringLiteral("✅  预订所选车次"), bookGroup);
    m_bookBtn->setStyleSheet(BTN_SUCCESS);
    m_bookBtn->setCursor(Qt::PointingHandCursor);
    m_bookBtn->setMinimumHeight(50);
    bookLayout->addWidget(m_bookBtn);
    connect(m_bookBtn, &QPushButton::clicked, this, &TicketWindow::onBookTicket);

    layout->addWidget(bookGroup);

    return page;
}

// ══════════════════════════════════════════════════════════════════════════════
// 订票逻辑
// ══════════════════════════════════════════════════════════════════════════════

void TicketWindow::onSearchTrains()
{
    auto trains = m_ticketMgr.searchTrains(
        m_depStationEdit->text().trimmed(),
        m_arrStationEdit->text().trimmed(),
        m_dateEdit->text().trimmed());

    m_trainTable->setRowCount(0);
    for (const auto &t : trains) {
        int r = m_trainTable->rowCount();
        m_trainTable->insertRow(r);

        auto *numItem    = new QTableWidgetItem(t["trainNumber"].toString());
        auto *depItem    = new QTableWidgetItem(t["departureStation"].toString());
        auto *arrItem    = new QTableWidgetItem(t["arrivalStation"].toString());
        auto *depTItem   = new QTableWidgetItem(t["departureTime"].toString());
        auto *arrTItem   = new QTableWidgetItem(t["arrivalTime"].toString());
        int remaining = t["remainingSeats"].toInt();
        auto *seatItem   = new QTableWidgetItem(QString::number(remaining));
        auto *totalItem  = new QTableWidgetItem(t["totalSeats"].toString());

        // 对齐
        numItem->setTextAlignment(Qt::AlignCenter);
        depItem->setTextAlignment(Qt::AlignCenter);
        arrItem->setTextAlignment(Qt::AlignCenter);
        depTItem->setTextAlignment(Qt::AlignCenter);
        arrTItem->setTextAlignment(Qt::AlignCenter);
        seatItem->setTextAlignment(Qt::AlignCenter);
        totalItem->setTextAlignment(Qt::AlignCenter);

        // 余票颜色
        if (remaining <= 5 && remaining > 0) {
            seatItem->setForeground(QColor("#f59e0b"));
            QFont f = seatItem->font(); f.setBold(true); seatItem->setFont(f);
        } else if (remaining == 0) {
            seatItem->setForeground(QColor("#e11d48"));
            QFont f = seatItem->font(); f.setBold(true); seatItem->setFont(f);
        } else {
            seatItem->setForeground(QColor("#059669"));
        }

        m_trainTable->setItem(r, 0, numItem);
        m_trainTable->setItem(r, 1, depItem);
        m_trainTable->setItem(r, 2, arrItem);
        m_trainTable->setItem(r, 3, depTItem);
        m_trainTable->setItem(r, 4, arrTItem);
        m_trainTable->setItem(r, 5, seatItem);
        m_trainTable->setItem(r, 6, totalItem);
    }

    if (trains.isEmpty()) {
        QMessageBox::information(this, QStringLiteral("搜索结果"),
                                 QStringLiteral("未找到符合条件的车次。"));
    }
}

void TicketWindow::onBookTicket()
{
    int row = m_trainTable->currentRow();
    if (row < 0) {
        QMessageBox msg(this);
        msg.setWindowTitle(QStringLiteral("提示"));
        msg.setText(QStringLiteral("请先在表格中点击选择要预订的车次。"));
        msg.setIcon(QMessageBox::Warning);
        msg.setStyleSheet("QLabel{font-size:16px;} QPushButton{font-size:15px; padding:10px 30px;}");
        msg.exec();
        return;
    }

    QString passengerName = m_passengerEdit->text().trimmed();
    if (passengerName.isEmpty()) {
        QMessageBox msg(this);
        msg.setWindowTitle(QStringLiteral("提示"));
        msg.setText(QStringLiteral("请输入乘客姓名。"));
        msg.setIcon(QMessageBox::Warning);
        msg.setStyleSheet("QLabel{font-size:16px;} QPushButton{font-size:15px; padding:10px 30px;}");
        msg.exec();
        return;
    }

    QString trainNumber = m_trainTable->item(row, 0)->text().trimmed();
    auto trains = m_ticketMgr.searchByTrainNumber(trainNumber);
    if (trains.isEmpty()) {
        QMessageBox::warning(this, QStringLiteral("错误"), QStringLiteral("车次信息异常。"));
        return;
    }

    int trainId = trains.first()["trainId"].toInt();
    int orderId = m_ticketMgr.bookTicket(m_currentUserId, trainId, passengerName);

    if (orderId > 0) {
        QMessageBox msg(this);
        msg.setWindowTitle(QStringLiteral("🎉 预订成功"));
        msg.setText(QStringLiteral("<div style='font-size:18px; line-height:1.8;'>"
                                   "<b>订单号：</b>%1<br>"
                                   "<b>乘  客：</b>%2<br>"
                                   "<b>车  次：</b>%3<br>"
                                   "<b>出发站：</b>%4<br>"
                                   "<b>到达站：</b>%5</div>")
                    .arg(orderId).arg(passengerName).arg(trainNumber)
                    .arg(m_trainTable->item(row, 1)->text())
                    .arg(m_trainTable->item(row, 2)->text()));
        msg.setIcon(QMessageBox::Information);
        msg.setStyleSheet("QLabel{font-size:16px;} QPushButton{font-size:16px; padding:12px 36px;}");
        msg.exec();
        onSearchTrains();
    } else {
        QMessageBox msg(this);
        msg.setWindowTitle(QStringLiteral("预订失败"));
        msg.setText(m_ticketMgr.lastError());
        msg.setIcon(QMessageBox::Critical);
        msg.setStyleSheet("QLabel{font-size:16px;} QPushButton{font-size:15px; padding:10px 30px;}");
        msg.exec();
    }
}

// ══════════════════════════════════════════════════════════════════════════════
// 页签 2：退票 / 改签
// ══════════════════════════════════════════════════════════════════════════════

QWidget* TicketWindow::createRefundChangeTab()
{
    auto *page = new QWidget(this);
    auto *layout = new QVBoxLayout(page);
    layout->setContentsMargins(8, 8, 8, 8);
    layout->setSpacing(14);

    // ── 搜索 ──────────────────────────────────────────────────────
    auto *searchGroup = new QGroupBox(QStringLiteral("  查询订单  "), page);
    auto *searchLayout = new QHBoxLayout(searchGroup);
    searchLayout->setSpacing(14);

    auto *searchLabel = new QLabel(QStringLiteral("搜索"), searchGroup);
    searchLabel->setStyleSheet("font-weight: bold; font-size: 16px; color: #334155;");
    searchLayout->addWidget(searchLabel);

    m_orderSearchEdit = new QLineEdit(searchGroup);
    m_orderSearchEdit->setPlaceholderText(QStringLiteral("输入乘客姓名或订单号"));
    m_orderSearchEdit->setMinimumWidth(250);
    searchLayout->addWidget(m_orderSearchEdit);

    auto *searchBtn = new QPushButton(QStringLiteral("🔍  查询订单"), searchGroup);
    searchBtn->setStyleSheet(BTN_PRIMARY);
    searchBtn->setCursor(Qt::PointingHandCursor);
    searchLayout->addWidget(searchBtn);
    connect(searchBtn, &QPushButton::clicked, this, &TicketWindow::onSearchOrders);

    auto *allBtn = new QPushButton(QStringLiteral("📋  显示全部订单"), searchGroup);
    allBtn->setStyleSheet(BTN_WARN);
    allBtn->setCursor(Qt::PointingHandCursor);
    searchLayout->addWidget(allBtn);
    connect(allBtn, &QPushButton::clicked, this, [this]() {
        m_orderSearchEdit->clear();
        onSearchOrders();
    });

    layout->addWidget(searchGroup);

    // ── 订单表格 ─────────────────────────────────────────────────
    m_orderTable = new QTableWidget(page);
    m_orderTable->setColumnCount(8);
    m_orderTable->setHorizontalHeaderLabels({
        QStringLiteral("  订单号  "), QStringLiteral("  车次号  "),
        QStringLiteral("  出发站  "), QStringLiteral("  到达站  "),
        QStringLiteral("  乘客  "),   QStringLiteral("  购票时间  "),
        QStringLiteral("  状态  "),   QStringLiteral("  备注  ")
    });
    m_orderTable->horizontalHeader()->setStretchLastSection(true);
    m_orderTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    m_orderTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_orderTable->setSelectionMode(QAbstractItemView::SingleSelection);
    m_orderTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_orderTable->setAlternatingRowColors(true);
    m_orderTable->verticalHeader()->setDefaultSectionSize(48);
    m_orderTable->setShowGrid(false);
    layout->addWidget(m_orderTable, 1);

    // ── 操作栏 ───────────────────────────────────────────────────
    auto *actionGroup = new QGroupBox(QStringLiteral("  订单操作  "), page);
    auto *actionLayout = new QHBoxLayout(actionGroup);
    actionLayout->setSpacing(20);

    m_refundBtn = new QPushButton(QStringLiteral("💵  退  票"), actionGroup);
    m_refundBtn->setStyleSheet(BTN_DANGER);
    m_refundBtn->setCursor(Qt::PointingHandCursor);
    m_refundBtn->setMinimumSize(160, 52);
    actionLayout->addWidget(m_refundBtn);
    connect(m_refundBtn, &QPushButton::clicked, this, &TicketWindow::onRefundOrder);

    actionLayout->addStretch();

    auto *changeLabel = new QLabel(QStringLiteral("改签目标车次"), actionGroup);
    changeLabel->setStyleSheet("font-weight: bold; font-size: 17px; color: #334155;");
    actionLayout->addWidget(changeLabel);

    m_newTrainEdit = new QLineEdit(actionGroup);
    m_newTrainEdit->setPlaceholderText(QStringLiteral("输入新车次号，如 G102"));
    m_newTrainEdit->setMinimumWidth(200);
    m_newTrainEdit->setStyleSheet("font-size: 17px; padding: 14px 18px;");
    actionLayout->addWidget(m_newTrainEdit);

    m_changeBtn = new QPushButton(QStringLiteral("🔄  改  签"), actionGroup);
    m_changeBtn->setStyleSheet(BTN_PRIMARY);
    m_changeBtn->setCursor(Qt::PointingHandCursor);
    m_changeBtn->setMinimumSize(160, 52);
    actionLayout->addWidget(m_changeBtn);
    connect(m_changeBtn, &QPushButton::clicked, this, &TicketWindow::onChangeTicket);

    layout->addWidget(actionGroup);

    return page;
}

void TicketWindow::onSearchOrders()
{
    QString keyword = m_orderSearchEdit->text().trimmed();
    QVector<QVariantMap> orders;

    if (keyword.isEmpty()) {
        orders = m_ticketMgr.queryAllOrders();
    } else {
        bool isNumber = false;
        int orderId = keyword.toInt(&isNumber);
        if (isNumber) {
            orders = m_ticketMgr.queryOrderByOrderId(orderId);
        } else {
            orders = m_ticketMgr.queryOrdersByPassenger(keyword);
        }
    }

    auto statusText = [](int s) -> QString {
        switch (s) {
        case 0: return QStringLiteral("✅ 已预订");
        case 1: return QStringLiteral("❌ 已退票");
        case 2: return QStringLiteral("🔄 已改签");
        default: return QStringLiteral("未知");
        }
    };

    m_orderTable->setRowCount(0);
    for (const auto &o : orders) {
        int r = m_orderTable->rowCount();
        m_orderTable->insertRow(r);

        auto *idItem       = new QTableWidgetItem(o["orderId"].toString());
        auto *trainItem    = new QTableWidgetItem(o.value("trainNumber", "").toString());
        auto *depItem      = new QTableWidgetItem(o.value("departureStation", "").toString());
        auto *arrItem      = new QTableWidgetItem(o.value("arrivalStation", "").toString());
        auto *passItem     = new QTableWidgetItem(o["passengerName"].toString());
        auto *timeItem     = new QTableWidgetItem(o["purchaseTime"].toString());
        int status = o["status"].toInt();
        auto *statusItem   = new QTableWidgetItem(statusText(status));

        idItem->setTextAlignment(Qt::AlignCenter);
        trainItem->setTextAlignment(Qt::AlignCenter);
        depItem->setTextAlignment(Qt::AlignCenter);
        arrItem->setTextAlignment(Qt::AlignCenter);
        passItem->setTextAlignment(Qt::AlignCenter);
        timeItem->setTextAlignment(Qt::AlignCenter);
        statusItem->setTextAlignment(Qt::AlignCenter);

        // 状态颜色
        switch (status) {
        case 0: statusItem->setForeground(QColor("#059669")); break;
        case 1: statusItem->setForeground(QColor("#e11d48")); break;
        case 2: statusItem->setForeground(QColor("#4a6cf7")); break;
        }

        m_orderTable->setItem(r, 0, idItem);
        m_orderTable->setItem(r, 1, trainItem);
        m_orderTable->setItem(r, 2, depItem);
        m_orderTable->setItem(r, 3, arrItem);
        m_orderTable->setItem(r, 4, passItem);
        m_orderTable->setItem(r, 5, timeItem);
        m_orderTable->setItem(r, 6, statusItem);
        m_orderTable->setItem(r, 7, new QTableWidgetItem(
            status == 2 ? QStringLiteral("已改签为新单") : ""));
    }
}

void TicketWindow::onRefundOrder()
{
    int row = m_orderTable->currentRow();
    if (row < 0) {
        QMessageBox::warning(this, QStringLiteral("提示"), QStringLiteral("请先选择一个订单。"));
        return;
    }

    int orderId = m_orderTable->item(row, 0)->text().toInt();
    QString statusText = m_orderTable->item(row, 6)->text();

    if (!statusText.contains(QStringLiteral("已预订"))) {
        QMessageBox::warning(this, QStringLiteral("提示"),
                             QStringLiteral("只能退订状态为 [已预订] 的订单。"));
        return;
    }

    QMessageBox msg(this);
    msg.setWindowTitle(QStringLiteral("确认退票"));
    msg.setText(QStringLiteral("<div style='font-size:17px;'>确定要退订 <b>订单 #%1</b> 吗？<br>"
                               "此操作不可撤销。</div>").arg(orderId));
    msg.setIcon(QMessageBox::Question);
    msg.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
    msg.setStyleSheet("QLabel{font-size:16px;} QPushButton{font-size:15px; padding:10px 30px;}");

    if (msg.exec() == QMessageBox::Yes) {
        if (m_ticketMgr.refundTicket(orderId)) {
            QMessageBox::information(this, QStringLiteral("退票成功"),
                                     QStringLiteral("订单 #%1 已成功退票。").arg(orderId));
            onSearchOrders();
        } else {
            QMessageBox::critical(this, QStringLiteral("退票失败"), m_ticketMgr.lastError());
        }
    }
}

void TicketWindow::onChangeTicket()
{
    int row = m_orderTable->currentRow();
    if (row < 0) {
        QMessageBox::warning(this, QStringLiteral("提示"), QStringLiteral("请先选择一个订单。"));
        return;
    }

    int orderId = m_orderTable->item(row, 0)->text().toInt();
    QString statusText = m_orderTable->item(row, 6)->text();

    if (!statusText.contains(QStringLiteral("已预订"))) {
        QMessageBox::warning(this, QStringLiteral("提示"),
                             QStringLiteral("只能改签状态为 [已预订] 的订单。"));
        return;
    }

    QString newTrainNumber = m_newTrainEdit->text().trimmed();
    if (newTrainNumber.isEmpty()) {
        QMessageBox::warning(this, QStringLiteral("提示"), QStringLiteral("请输入目标车次号。"));
        return;
    }

    auto trains = m_ticketMgr.searchByTrainNumber(newTrainNumber);
    if (trains.isEmpty()) {
        QMessageBox::warning(this, QStringLiteral("错误"),
                             QStringLiteral("找不到车次号: %1").arg(newTrainNumber));
        return;
    }

    int newTrainId = trains.first()["trainId"].toInt();

    QMessageBox msg(this);
    msg.setWindowTitle(QStringLiteral("确认改签"));
    msg.setText(QStringLiteral("<div style='font-size:17px;'>确定将 <b>订单 #%1</b> 改签至 "
                               "<b>%2</b> 吗？</div>").arg(orderId).arg(newTrainNumber));
    msg.setIcon(QMessageBox::Question);
    msg.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
    msg.setStyleSheet("QLabel{font-size:16px;} QPushButton{font-size:15px; padding:10px 30px;}");

    if (msg.exec() == QMessageBox::Yes) {
        if (m_ticketMgr.changeTicket(orderId, newTrainId)) {
            QMessageBox::information(this, QStringLiteral("改签成功"),
                                     QStringLiteral("订单 #%1 已改签至 %2。").arg(orderId).arg(newTrainNumber));
            onSearchOrders();
        } else {
            QMessageBox::critical(this, QStringLiteral("改签失败"), m_ticketMgr.lastError());
        }
    }
}

// ══════════════════════════════════════════════════════════════════════════════
// 页签 3：统计信息
// ══════════════════════════════════════════════════════════════════════════════

// 状态卡片样式
static const char* STAT_CARD_BLUE = R"(
    background: qlineargradient(x1:0,y1:0, x2:0,y2:1, stop:0 #4a6cf7, stop:1 #3651d5);
    border-radius: 12px; padding: 20px; color: white;
)";
static const char* STAT_CARD_GREEN = R"(
    background: qlineargradient(x1:0,y1:0, x2:0,y2:1, stop:0 #10b981, stop:1 #059669);
    border-radius: 12px; padding: 20px; color: white;
)";
static const char* STAT_CARD_RED = R"(
    background: qlineargradient(x1:0,y1:0, x2:0,y2:1, stop:0 #f43f5e, stop:1 #e11d48);
    border-radius: 12px; padding: 20px; color: white;
)";
static const char* STAT_CARD_ORANGE = R"(
    background: qlineargradient(x1:0,y1:0, x2:0,y2:1, stop:0 #f59e0b, stop:1 #d97706);
    border-radius: 12px; padding: 20px; color: white;
)";

QWidget* TicketWindow::createStatisticsTab()
{
    auto *page = new QWidget(this);
    auto *layout = new QVBoxLayout(page);
    layout->setContentsMargins(8, 8, 8, 8);
    layout->setSpacing(14);

    // ── 数据卡片行 ───────────────────────────────────────────────
    auto *cardRow = new QHBoxLayout();
    cardRow->setSpacing(16);

    struct CardInfo { const char *title; const char *style; int TicketWindow::*fn; };
    // 用 lambda 创建卡片
    auto makeCard = [&](const QString &title, const QString &style, int count, const QString &icon) {
        auto *card = new QLabel(page);
        card->setStyleSheet(style);
        card->setText(QStringLiteral("<div style='text-align:center;'>"
                     "<div style='font-size:44px;'>%1</div>"
                     "<div style='font-size:38px; font-weight:bold;'>%2</div>"
                     "<div style='font-size:17px; margin-top:8px;'>%3</div></div>")
                     .arg(icon).arg(count).arg(title));
        card->setMinimumHeight(140);
        card->setAlignment(Qt::AlignCenter);
        return card;
    };

    // 先占位，refresh 时更新
    m_overviewCards.clear();
    auto *c1 = makeCard(QStringLiteral("总订单"),  STAT_CARD_BLUE,   0, "📋");
    auto *c2 = makeCard(QStringLiteral("已预订"),  STAT_CARD_GREEN,   0, "✅");
    auto *c3 = makeCard(QStringLiteral("已退票"),  STAT_CARD_RED,     0, "❌");
    auto *c4 = makeCard(QStringLiteral("已改签"),  STAT_CARD_ORANGE,  0, "🔄");

    m_overviewCards << c1 << c2 << c3 << c4;
    for (auto *c : m_overviewCards) {
        cardRow->addWidget(c);
    }

    layout->addLayout(cardRow);

    // 刷新按钮
    auto *refreshBar = new QHBoxLayout();
    refreshBar->addStretch();
    auto *refreshBtn = new QPushButton(QStringLiteral("🔄  刷新统计数据"), page);
    refreshBtn->setStyleSheet(BTN_PRIMARY);
    refreshBtn->setCursor(Qt::PointingHandCursor);
    refreshBtn->setMinimumSize(200, 50);
    refreshBar->addWidget(refreshBtn);
    refreshBar->addStretch();
    layout->addLayout(refreshBar);
    connect(refreshBtn, &QPushButton::clicked, this, &TicketWindow::onRefreshStatistics);

    // ── 表格区域：订单历史 | 热门路线 ───────────────────────────
    auto *splitter = new QSplitter(Qt::Horizontal, page);

    // 订单历史（来自 TicketManager）
    auto *historyGroup = new QGroupBox(QStringLiteral("  订单历史  "), splitter);
    auto *historyLayout = new QVBoxLayout(historyGroup);
    m_orderHistoryTable = new QTableWidget(historyGroup);
    m_orderHistoryTable->setColumnCount(7);
    m_orderHistoryTable->setHorizontalHeaderLabels({
        QStringLiteral("订单号"), QStringLiteral("车次号"),
        QStringLiteral("出发站"), QStringLiteral("到达站"),
        QStringLiteral("乘客"),   QStringLiteral("购票时间"),
        QStringLiteral("状态")
    });
    m_orderHistoryTable->horizontalHeader()->setStretchLastSection(true);
    m_orderHistoryTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    m_orderHistoryTable->setAlternatingRowColors(true);
    m_orderHistoryTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_orderHistoryTable->verticalHeader()->setDefaultSectionSize(44);
    m_orderHistoryTable->setShowGrid(false);
    historyLayout->addWidget(m_orderHistoryTable);
    splitter->addWidget(historyGroup);

    // 热门路线
    auto *routesGroup = new QGroupBox(QStringLiteral("  热门路线 Top 10  "), splitter);
    auto *routesLayout2 = new QVBoxLayout(routesGroup);
    m_routesTable = new QTableWidget(routesGroup);
    m_routesTable->setColumnCount(4);
    m_routesTable->setHorizontalHeaderLabels({
        QStringLiteral("排名"), QStringLiteral("出发站"), QStringLiteral("到达站"), QStringLiteral("订单数")
    });
    m_routesTable->horizontalHeader()->setStretchLastSection(true);
    m_routesTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    m_routesTable->setAlternatingRowColors(true);
    m_routesTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_routesTable->verticalHeader()->setDefaultSectionSize(44);
    m_routesTable->setShowGrid(false);
    routesLayout2->addWidget(m_routesTable);
    splitter->addWidget(routesGroup);

    layout->addWidget(splitter, 1);

    // ── 月度客流 ─────────────────────────────────────────────────
    auto *monthlyGroup = new QGroupBox(QStringLiteral("  月度客流汇总  "), page);
    auto *monthlyLayout2 = new QVBoxLayout(monthlyGroup);
    m_monthlyTable = new QTableWidget(monthlyGroup);
    m_monthlyTable->setColumnCount(4);
    m_monthlyTable->setHorizontalHeaderLabels({
        QStringLiteral("月份"), QStringLiteral("总订单"), QStringLiteral("已预订"), QStringLiteral("已退票")
    });
    m_monthlyTable->horizontalHeader()->setStretchLastSection(true);
    m_monthlyTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    m_monthlyTable->setAlternatingRowColors(true);
    m_monthlyTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_monthlyTable->verticalHeader()->setDefaultSectionSize(44);
    m_monthlyTable->setShowGrid(false);
    m_monthlyTable->setMaximumHeight(250);
    monthlyLayout2->addWidget(m_monthlyTable);
    layout->addWidget(monthlyGroup);

    // 初始刷新
    onRefreshStatistics();

    return page;
}

void TicketWindow::onRefreshStatistics()
{
    // ── 更新卡片 ─────────────────────────────────────────────────
    int total    = m_statsMgr.totalOrders();
    int booked   = m_statsMgr.totalBooked();
    int refunded = m_statsMgr.totalRefunded();
    int changed  = m_statsMgr.totalChanged();

    auto updateCard = [](QLabel *card, const QString &icon, int count, const QString &title) {
        card->setText(QStringLiteral("<div style='text-align:center;'>"
                     "<div style='font-size:44px;'>%1</div>"
                     "<div style='font-size:38px; font-weight:bold;'>%2</div>"
                     "<div style='font-size:17px; margin-top:8px;'>%3</div></div>")
                     .arg(icon).arg(count).arg(title));
    };

    if (m_overviewCards.size() >= 4) {
        updateCard(m_overviewCards[0], "📋", total,    QStringLiteral("总订单"));
        updateCard(m_overviewCards[1], "✅", booked,   QStringLiteral("已预订"));
        updateCard(m_overviewCards[2], "❌", refunded, QStringLiteral("已退票"));
        updateCard(m_overviewCards[3], "🔄", changed,  QStringLiteral("已改签"));
    }

    // ── 订单历史（来自 TicketManager） ──────────────────────────
    auto orders = m_ticketMgr.queryAllOrders();
    auto statusText = [](int s) -> QString {
        switch (s) {
        case 0: return QStringLiteral("✅ 已预订");
        case 1: return QStringLiteral("❌ 已退票");
        case 2: return QStringLiteral("🔄 已改签");
        default: return QStringLiteral("未知");
        }
    };
    m_orderHistoryTable->setRowCount(0);
    for (const auto &o : orders) {
        int r = m_orderHistoryTable->rowCount();
        m_orderHistoryTable->insertRow(r);
        int status = o["status"].toInt();
        auto *statusItem = centerItem(statusText(status));
        switch (status) {
        case 0: statusItem->setForeground(QColor("#059669")); break;
        case 1: statusItem->setForeground(QColor("#e11d48")); break;
        case 2: statusItem->setForeground(QColor("#4a6cf7")); break;
        }
        m_orderHistoryTable->setItem(r, 0, centerItem(o["orderId"].toString()));
        m_orderHistoryTable->setItem(r, 1, centerItem(o.value("trainNumber", "").toString()));
        m_orderHistoryTable->setItem(r, 2, centerItem(o.value("departureStation", "").toString()));
        m_orderHistoryTable->setItem(r, 3, centerItem(o.value("arrivalStation", "").toString()));
        m_orderHistoryTable->setItem(r, 4, centerItem(o["passengerName"].toString()));
        m_orderHistoryTable->setItem(r, 5, centerItem(o["purchaseTime"].toString()));
        m_orderHistoryTable->setItem(r, 6, statusItem);
    }

    // ── 热门路线 ─────────────────────────────────────────────────
    auto routes = m_statsMgr.popularRoutes();
    m_routesTable->setRowCount(0);
    int rank = 0;
    for (const auto &rt : routes) {
        int r = m_routesTable->rowCount();
        m_routesTable->insertRow(r);
        ++rank;
        auto *rankItem = centerItem(QString::number(rank));
        // 前三名特殊标记
        if (rank == 1)      { rankItem->setText("🥇 1"); rankItem->setForeground(QColor("#f59e0b")); }
        else if (rank == 2) { rankItem->setText("🥈 2"); rankItem->setForeground(QColor("#94a3b8")); }
        else if (rank == 3) { rankItem->setText("🥉 3"); rankItem->setForeground(QColor("#d97706")); }

        m_routesTable->setItem(r, 0, rankItem);
        m_routesTable->setItem(r, 1, centerItem(rt["departureStation"].toString()));
        m_routesTable->setItem(r, 2, centerItem(rt["arrivalStation"].toString()));
        m_routesTable->setItem(r, 3, centerItem(rt["orderCount"].toString()));
    }

    // ── 月度客流 ─────────────────────────────────────────────────
    auto monthly = m_statsMgr.monthlyPassengerFlow();
    m_monthlyTable->setRowCount(0);
    for (const auto &m : monthly) {
        int r = m_monthlyTable->rowCount();
        m_monthlyTable->insertRow(r);
        m_monthlyTable->setItem(r, 0, centerItem(m["month"].toString()));
        m_monthlyTable->setItem(r, 1, centerItem(m["totalOrders"].toString()));
        m_monthlyTable->setItem(r, 2, centerItem(m["booked"].toString()));
        m_monthlyTable->setItem(r, 3, centerItem(m["refunded"].toString()));
    }
}

