/**
 * @file statistics_dialog.cpp - Issue 11: 管理员统计面板
 */
#include "statistics_dialog.h"
#include "database_manager.h"
#include "statistics_manager.h"

#include <QGroupBox>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QPushButton>
#include <QTableWidget>
#include <QVBoxLayout>

static QLabel *makeStatVal(QWidget *parent)
{
    auto *label = new QLabel(QStringLiteral("—"), parent);
    label->setStyleSheet(QStringLiteral(
        "color:#153832; font-size:28px; font-weight:700;"));
    return label;
}

static QLabel *makeStatLabel(const QString &text, QWidget *parent)
{
    auto *label = new QLabel(text, parent);
    label->setStyleSheet(QStringLiteral("color:#65716c; font-size:13px;"));
    return label;
}

StatisticsDialog::StatisticsDialog(DatabaseManager &db, QWidget *parent)
    : QDialog(parent), m_db(db)
{
    setWindowTitle(QStringLiteral("票务数据统计"));
    setModal(true);
    resize(760, 600);

    setStyleSheet(QStringLiteral(R"QSS(
        QDialog {
            background: #eef2f3;
            color: #1f2933;
            font-family: "Microsoft YaHei UI", "Microsoft YaHei", "Segoe UI";
            font-size: 14px;
        }
        QGroupBox {
            background: #fbfcfb;
            border: 1px solid #d8e0dc;
            border-radius: 10px;
            margin-top: 12px;
            padding: 12px;
            font-weight: 700;
        }
        QGroupBox::title {
            subcontrol-origin: margin;
            left: 12px;
            padding: 0 6px;
        }
        QLabel#pageTitle {
            color: #17231f;
            font-size: 22px;
            font-weight: 700;
        }
        QLabel#pageHint {
            color: #65716c;
        }
        QTableWidget {
            background: #ffffff;
            border: 1px solid #cbd8d2;
            border-radius: 8px;
            gridline-color: #e5ece8;
            selection-background-color: #d9f99d;
            selection-color: #153832;
        }
        QHeaderView::section {
            background: #eef5f1;
            color: #33433d;
            border: none;
            padding: 7px;
            font-weight: 700;
        }
        QPushButton {
            min-height: 32px;
            border-radius: 8px;
            padding: 5px 14px;
            font-weight: 700;
            color: #ffffff;
            background: #176b5b;
            border: none;
        }
        QPushButton:hover { background: #0f5749; }
    )QSS"));

    auto *root = new QVBoxLayout(this);
    root->setContentsMargins(24, 22, 24, 22);
    root->setSpacing(14);

    auto *titleLabel = new QLabel(QStringLiteral("票务数据统计"), this);
    titleLabel->setObjectName(QStringLiteral("pageTitle"));

    auto *hintLabel = new QLabel(QStringLiteral("查看售票、退款、客流和热门线路统计。"), this);
    hintLabel->setObjectName(QStringLiteral("pageHint"));

    root->addWidget(titleLabel);
    root->addWidget(hintLabel);

    // ── Summary cards ──────────────────────────────────────────────
    auto *summaryGroup = new QGroupBox(QStringLiteral("订单概览"), this);
    auto *summaryGrid = new QHBoxLayout(summaryGroup);
    summaryGrid->setSpacing(24);

    struct { QLabel *val; const char *text; } stats[] = {
        {m_totalOrdersVal,   "总订单数"},
        {m_totalBookedVal,   "已预订"},
        {m_totalRefundedVal, "已退票"},
        {m_totalChangedVal,  "已改签"},
    };
    for (auto &s : stats) {
        s.val = makeStatVal(summaryGroup);
        auto *col = new QVBoxLayout;
        col->setSpacing(4);
        col->addWidget(s.val, 0, Qt::AlignCenter);
        col->addWidget(makeStatLabel(QString::fromUtf8(s.text), summaryGroup), 0, Qt::AlignCenter);
        summaryGrid->addLayout(col);
    }
    root->addWidget(summaryGroup);

    // ── Popular routes ─────────────────────────────────────────────
    auto *routesGroup = new QGroupBox(QStringLiteral("热门路线"), this);
    auto *routesLayout = new QVBoxLayout(routesGroup);

    m_routesTable = new QTableWidget(routesGroup);
    m_routesTable->setColumnCount(3);
    m_routesTable->setHorizontalHeaderLabels({
        QStringLiteral("出发站"), QStringLiteral("到达站"), QStringLiteral("订单数")
    });
    m_routesTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_routesTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_routesTable->verticalHeader()->setVisible(false);
    m_routesTable->horizontalHeader()->setStretchLastSection(true);
    m_routesTable->setMinimumHeight(140);

    routesLayout->addWidget(m_routesTable);
    root->addWidget(routesGroup);

    // ── Monthly flow ───────────────────────────────────────────────
    auto *monthlyGroup = new QGroupBox(QStringLiteral("月度客流"), this);
    auto *monthlyLayout = new QVBoxLayout(monthlyGroup);

    m_monthlyTable = new QTableWidget(monthlyGroup);
    m_monthlyTable->setColumnCount(4);
    m_monthlyTable->setHorizontalHeaderLabels({
        QStringLiteral("月份"), QStringLiteral("总订单"), QStringLiteral("已预订"), QStringLiteral("已退票")
    });
    m_monthlyTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_monthlyTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_monthlyTable->verticalHeader()->setVisible(false);
    m_monthlyTable->horizontalHeader()->setStretchLastSection(true);
    m_monthlyTable->setMinimumHeight(140);

    monthlyLayout->addWidget(m_monthlyTable);
    root->addWidget(monthlyGroup);

    // ── Bottom buttons ─────────────────────────────────────────────
    auto *bottom = new QHBoxLayout;
    auto *refreshBtn = new QPushButton(QStringLiteral("刷新数据"), this);
    connect(refreshBtn, &QPushButton::clicked, this, [this]() { refresh(); });

    auto *closeBtn = new QPushButton(QStringLiteral("关闭"), this);
    connect(closeBtn, &QPushButton::clicked, this, [this]() { close(); });

    bottom->addWidget(refreshBtn);
    bottom->addStretch();
    bottom->addWidget(closeBtn);
    root->addLayout(bottom);

    refresh();
}

void StatisticsDialog::refresh()
{
    StatisticsManager sm(m_db);

    m_totalOrdersVal  ->setText(QString::number(sm.totalOrders()));
    m_totalBookedVal  ->setText(QString::number(sm.totalBooked()));
    m_totalRefundedVal->setText(QString::number(sm.totalRefunded()));
    m_totalChangedVal ->setText(QString::number(sm.totalChanged()));

    // Popular routes
    auto routes = sm.popularRoutes();
    m_routesTable->setRowCount(routes.size());
    for (int i = 0; i < routes.size(); ++i) {
        const auto &r = routes[i];
        m_routesTable->setItem(i, 0, new QTableWidgetItem(r["departureStation"].toString()));
        m_routesTable->setItem(i, 1, new QTableWidgetItem(r["arrivalStation"].toString()));
        auto *cnt = new QTableWidgetItem;
        cnt->setData(Qt::DisplayRole, r["orderCount"].toInt());
        cnt->setTextAlignment(Qt::AlignCenter);
        m_routesTable->setItem(i, 2, cnt);
    }
    m_routesTable->resizeRowsToContents();

    // Monthly flow
    auto monthly = sm.monthlyPassengerFlow();
    m_monthlyTable->setRowCount(monthly.size());
    for (int i = 0; i < monthly.size(); ++i) {
        const auto &m = monthly[i];
        m_monthlyTable->setItem(i, 0, new QTableWidgetItem(m["month"].toString()));
        for (int col = 1; col <= 3; ++col) {
            const char *keys[] = {"", "totalOrders", "booked", "refunded"};
            auto *item = new QTableWidgetItem;
            item->setData(Qt::DisplayRole, m[keys[col]].toInt());
            item->setTextAlignment(Qt::AlignCenter);
            m_monthlyTable->setItem(i, col, item);
        }
    }
    m_monthlyTable->resizeRowsToContents();
}
