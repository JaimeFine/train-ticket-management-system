/**
 * @file ticket_query_dialog.cpp - Issue 10: 票务查询
 */
#include "ticket_query_dialog.h"
#include "database_manager.h"
#include "ticket_manager.h"

#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QTableWidget>
#include <QVBoxLayout>

static QString statusText(int s)
{
    switch (s) {
    case 0: return QStringLiteral("已预订");
    case 1: return QStringLiteral("已退票");
    case 2: return QStringLiteral("已改签");
    default: return QStringLiteral("未知");
    }
}

TicketQueryDialog::TicketQueryDialog(DatabaseManager &db, QWidget *parent)
    : QDialog(parent), m_db(db)
{
    setWindowTitle(QStringLiteral("票务查询"));
    setModal(true);
    resize(800, 500);

    setStyleSheet(QStringLiteral(R"QSS(
        QDialog { background:#eef2f3; color:#1f2933;
            font-family:"Microsoft YaHei UI","Microsoft YaHei","Segoe UI"; font-size:14px; }
        QGroupBox { background:#fbfcfb; border:1px solid #d8e0dc; border-radius:10px;
            margin-top:12px; padding:12px; font-weight:700; }
        QGroupBox::title { subcontrol-origin:margin; left:12px; padding:0 6px; }
        QLabel#pageTitle { color:#17231f; font-size:22px; font-weight:700; }
        QLabel#pageHint { color:#65716c; }
        QLineEdit { min-height:32px; border:1px solid #cbd8d2; border-radius:8px;
            padding:4px 10px; background:#ffffff; }
        QLineEdit:focus { border:2px solid #0f766e; padding:3px 9px; }
        QTableWidget { background:#ffffff; border:1px solid #cbd8d2; border-radius:8px;
            gridline-color:#e5ece8; selection-background-color:#d9f99d; selection-color:#153832; }
        QHeaderView::section { background:#eef5f1; color:#33433d; border:none; padding:7px; font-weight:700; }
        QPushButton { min-height:32px; border-radius:8px; padding:5px 14px; font-weight:700;
            color:#ffffff; background:#176b5b; border:none; }
        QPushButton:hover { background:#0f5749; }
    )QSS"));

    auto *root = new QVBoxLayout(this);
    root->setContentsMargins(24, 22, 24, 22);
    root->setSpacing(14);

    auto *titleLabel = new QLabel(QStringLiteral("票务查询"), this);
    titleLabel->setObjectName(QStringLiteral("pageTitle"));
    auto *hintLabel = new QLabel(QStringLiteral("按订单号或乘客姓名查询票务记录。"), this);
    hintLabel->setObjectName(QStringLiteral("pageHint"));
    root->addWidget(titleLabel);
    root->addWidget(hintLabel);

    // ── Search by order ID ─────────────────────────────────────
    auto *orderGroup = new QGroupBox(QStringLiteral("按订单号查询"), this);
    auto *orderLayout = new QHBoxLayout(orderGroup);
    orderLayout->setSpacing(12);

    m_orderEdit = new QLineEdit(orderGroup);
    m_orderEdit->setPlaceholderText(QStringLiteral("输入订单号"));
    auto *orderBtn = new QPushButton(QStringLiteral("查询"), orderGroup);
    connect(orderBtn, &QPushButton::clicked, this, &TicketQueryDialog::searchByOrderId);

    orderLayout->addWidget(m_orderEdit, 1);
    orderLayout->addWidget(orderBtn);
    root->addWidget(orderGroup);

    // ── Search by passenger ────────────────────────────────────
    auto *passGroup = new QGroupBox(QStringLiteral("按乘客姓名查询"), this);
    auto *passLayout = new QHBoxLayout(passGroup);
    passLayout->setSpacing(12);

    m_passengerEdit = new QLineEdit(passGroup);
    m_passengerEdit->setPlaceholderText(QStringLiteral("输入乘客姓名（支持模糊匹配）"));
    auto *passBtn = new QPushButton(QStringLiteral("查询"), passGroup);
    connect(passBtn, &QPushButton::clicked, this, &TicketQueryDialog::searchByPassenger);

    passLayout->addWidget(m_passengerEdit, 1);
    passLayout->addWidget(passBtn);
    root->addWidget(passGroup);

    // ── Results table ──────────────────────────────────────────
    m_table = new QTableWidget(this);
    m_table->setColumnCount(6);
    m_table->setHorizontalHeaderLabels({
        QStringLiteral("订单号"), QStringLiteral("乘客"), QStringLiteral("车次ID"),
        QStringLiteral("购票时间"), QStringLiteral("状态"), QStringLiteral("用户ID")
    });
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->verticalHeader()->setVisible(false);
    m_table->horizontalHeader()->setStretchLastSection(true);
    m_table->setMinimumHeight(200);
    root->addWidget(m_table);

    // ── Bottom ─────────────────────────────────────────────────
    auto *bottom = new QHBoxLayout;
    bottom->addStretch();
    auto *closeBtn = new QPushButton(QStringLiteral("关闭"), this);
    connect(closeBtn, &QPushButton::clicked, this, [this]() { close(); });
    bottom->addWidget(closeBtn);
    root->addLayout(bottom);
}

void TicketQueryDialog::searchByOrderId()
{
    TicketManager tm(m_db);
    bool ok;
    int id = m_orderEdit->text().toInt(&ok);
    if (!ok || id <= 0) return;
    populateTable(tm.queryOrderByOrderId(id));
}

void TicketQueryDialog::searchByPassenger()
{
    TicketManager tm(m_db);
    populateTable(tm.queryOrdersByPassenger(m_passengerEdit->text().trimmed()));
}

void TicketQueryDialog::populateTable(const QVector<QVariantMap> &results)
{
    m_table->setRowCount(results.size());
    for (int i = 0; i < results.size(); ++i) {
        const auto &r = results[i];
        auto *idItem = new QTableWidgetItem;
        idItem->setData(Qt::DisplayRole, r["orderId"].toInt());
        m_table->setItem(i, 0, idItem);
        m_table->setItem(i, 1, new QTableWidgetItem(r["passengerName"].toString()));

        auto *tid = new QTableWidgetItem;
        tid->setData(Qt::DisplayRole, r["trainId"].toInt());
        m_table->setItem(i, 2, tid);

        m_table->setItem(i, 3, new QTableWidgetItem(r["purchaseTime"].toString()));

        auto *st = new QTableWidgetItem(statusText(r["status"].toInt()));
        st->setTextAlignment(Qt::AlignCenter);
        m_table->setItem(i, 4, st);

        auto *uid = new QTableWidgetItem;
        uid->setData(Qt::DisplayRole, r["userId"].toInt());
        m_table->setItem(i, 5, uid);
    }
    m_table->resizeRowsToContents();
}
