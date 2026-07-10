/**
 * @file order_history_dialog.cpp - Issue 11: 订单历史面板
 */
#include "order_history_dialog.h"
#include "database_manager.h"
#include "ticket_manager.h"

#include <QHeaderView>
#include <QLabel>
#include <QPushButton>
#include <QTableWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>

OrderHistoryDialog::OrderHistoryDialog(DatabaseManager &db, int userId,
                                       QWidget *parent)
    : QDialog(parent), m_db(db), m_userId(userId)
{
    const bool allOrders = (userId < 0);
    setWindowTitle(allOrders ? QStringLiteral("全部订单历史")
                             : QStringLiteral("我的订单历史"));
    setModal(true);
    resize(860, 520);

    setStyleSheet(QStringLiteral(R"QSS(
        QDialog {
            background: #eef2f3;
            color: #1f2933;
            font-family: "Microsoft YaHei UI", "Microsoft YaHei", "Segoe UI";
            font-size: 14px;
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

    auto *titleLabel = new QLabel(
        allOrders ? QStringLiteral("全部订单历史") : QStringLiteral("我的订单历史"), this);
    titleLabel->setObjectName(QStringLiteral("pageTitle"));

    auto *hintLabel = new QLabel(
        allOrders ? QStringLiteral("查看系统中所有订单记录。")
                  : QStringLiteral("查看我的历史订单。"), this);
    hintLabel->setObjectName(QStringLiteral("pageHint"));

    root->addWidget(titleLabel);
    root->addWidget(hintLabel);

    // ── Orders table ───────────────────────────────────────────
    m_ordersTable = new QTableWidget(this);
    m_ordersTable->setColumnCount(7);
    m_ordersTable->setHorizontalHeaderLabels({
        QStringLiteral("订单号"), QStringLiteral("车次"),
        QStringLiteral("乘客"),   QStringLiteral("出发站"),
        QStringLiteral("到达站"), QStringLiteral("购票时间"),
        QStringLiteral("状态")
    });
    m_ordersTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_ordersTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_ordersTable->verticalHeader()->setVisible(false);
    m_ordersTable->horizontalHeader()->setStretchLastSection(true);
    m_ordersTable->setMinimumHeight(300);

    root->addWidget(m_ordersTable);

    // ── Bottom buttons ─────────────────────────────────────────
    auto *bottom = new QHBoxLayout;
    auto *refreshBtn = new QPushButton(QStringLiteral("刷新"), this);
    connect(refreshBtn, &QPushButton::clicked, this, [this]() { refresh(); });

    auto *closeBtn = new QPushButton(QStringLiteral("关闭"), this);
    connect(closeBtn, &QPushButton::clicked, this, [this]() { close(); });

    bottom->addWidget(refreshBtn);
    bottom->addStretch();
    bottom->addWidget(closeBtn);
    root->addLayout(bottom);

    refresh();
}

static QString statusText(int status)
{
    switch (status) {
    case 0: return QStringLiteral("已预订");
    case 1: return QStringLiteral("已退票");
    case 2: return QStringLiteral("已改签");
    default: return QStringLiteral("未知");
    }
}

void OrderHistoryDialog::refresh()
{
    TicketManager tm(m_db);
    const auto orders = tm.queryAllOrders();

    m_ordersTable->setRowCount(0);

    int row = 0;
    for (const auto &o : orders) {
        // Filter by userId if not showing all
        if (m_userId >= 0 && o["userId"].toInt() != m_userId)
            continue;

        m_ordersTable->insertRow(row);

        auto *idItem = new QTableWidgetItem;
        idItem->setData(Qt::DisplayRole, o["orderId"].toInt());
        m_ordersTable->setItem(row, 0, idItem);

        m_ordersTable->setItem(row, 1, new QTableWidgetItem(o["trainNumber"].toString()));
        m_ordersTable->setItem(row, 2, new QTableWidgetItem(o["passengerName"].toString()));
        m_ordersTable->setItem(row, 3, new QTableWidgetItem(o["departureStation"].toString()));
        m_ordersTable->setItem(row, 4, new QTableWidgetItem(o["arrivalStation"].toString()));
        m_ordersTable->setItem(row, 5, new QTableWidgetItem(o["purchaseTime"].toString()));

        auto *statusItem = new QTableWidgetItem(statusText(o["status"].toInt()));
        statusItem->setTextAlignment(Qt::AlignCenter);
        m_ordersTable->setItem(row, 6, statusItem);

        ++row;
    }
    m_ordersTable->resizeRowsToContents();
}
