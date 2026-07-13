#include "order_history_dialog.h"

#include "ticket_manager.h"

#include <QAbstractItemView>
#include <QHeaderView>
#include <QLabel>
#include <QPushButton>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QVBoxLayout>

OrderHistoryDialog::OrderHistoryDialog(const TicketManager &ticketManager,
                                       int userId,
                                       QWidget *parent)
    : QDialog(parent)
    , m_ticketManager(&ticketManager)
    , m_userId(userId)
{
    setWindowTitle(QStringLiteral("我的订单"));
    setModal(true);
    resize(760, 520);

    setStyleSheet(QStringLiteral(R"QSS(
        QDialog {
            background: #eef2f3;
            color: #1f2933;
            font-family: "Microsoft YaHei UI", "Microsoft YaHei", "Segoe UI";
            font-size: 14px;
        }
        QLabel#titleLabel {
            color: #17231f;
            font-size: 22px;
            font-weight: 700;
        }
        QLabel#hintLabel {
            color: #42514b;
        }
        QTableWidget {
            color: #1f2933;
            background: #ffffff;
            alternate-background-color: #f6faf8;
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
            min-height: 34px;
            border-radius: 8px;
            padding: 6px 16px;
            font-weight: 700;
            color: #ffffff;
            background: #176b5b;
            border: none;
        }
        QPushButton:hover {
            background: #0f5749;
        }
    )QSS"));

    auto *rootLayout = new QVBoxLayout(this);
    rootLayout->setContentsMargins(24, 22, 24, 22);
    rootLayout->setSpacing(14);

    auto *titleLabel = new QLabel(QStringLiteral("我的订单"), this);
    titleLabel->setObjectName(QStringLiteral("titleLabel"));

    auto *hintLabel = new QLabel(QStringLiteral("查看当前账号已经创建的订单记录。"), this);
    hintLabel->setObjectName(QStringLiteral("hintLabel"));

    m_ordersTable = new QTableWidget(this);
    m_ordersTable->setColumnCount(5);
    m_ordersTable->setHorizontalHeaderLabels({
        QStringLiteral("订单号"),
        QStringLiteral("车次编号"),
        QStringLiteral("乘客姓名"),
        QStringLiteral("购票时间"),
        QStringLiteral("状态")
    });
    m_ordersTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_ordersTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_ordersTable->setSelectionMode(QAbstractItemView::SingleSelection);
    m_ordersTable->setAlternatingRowColors(true);
    m_ordersTable->verticalHeader()->setVisible(false);
    m_ordersTable->horizontalHeader()->setStretchLastSection(true);

    auto *closeButton = new QPushButton(QStringLiteral("关闭"), this);
    connect(closeButton, &QPushButton::clicked, this, &QDialog::accept);

    rootLayout->addWidget(titleLabel);
    rootLayout->addWidget(hintLabel);
    rootLayout->addWidget(m_ordersTable);
    rootLayout->addWidget(closeButton, 0, Qt::AlignRight);

    loadOrders();
}

void OrderHistoryDialog::loadOrders()
{
    if (m_ticketManager == nullptr) {
        return;
    }

    const auto orders = m_ticketManager->queryOrdersByUser(m_userId);
    m_ordersTable->setRowCount(orders.size());

    for (int row = 0; row < orders.size(); ++row) {
        const QVariantMap &order = orders[row];
        m_ordersTable->setItem(row, 0,
                               new QTableWidgetItem(QString::number(order.value(QStringLiteral("orderId")).toInt())));
        m_ordersTable->setItem(row, 1,
                               new QTableWidgetItem(QString::number(order.value(QStringLiteral("trainId")).toInt())));
        m_ordersTable->setItem(row, 2,
                               new QTableWidgetItem(order.value(QStringLiteral("passengerName")).toString()));
        m_ordersTable->setItem(row, 3,
                               new QTableWidgetItem(order.value(QStringLiteral("purchaseTime")).toString()));
        m_ordersTable->setItem(row, 4,
                               new QTableWidgetItem(statusText(order.value(QStringLiteral("status")).toInt())));
    }

    m_ordersTable->resizeColumnsToContents();
}

QString OrderHistoryDialog::statusText(int status) const
{
    switch (status) {
    case 0:
        return QStringLiteral("已预订");
    case 1:
        return QStringLiteral("已退票");
    case 2:
        return QStringLiteral("已改签");
    default:
        return QStringLiteral("未知状态");
    }
}
