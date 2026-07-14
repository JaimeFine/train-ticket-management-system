#include "ticket_service_dialog.h"

#include "ticket_manager.h"
#include "database_manager.h"

#include <QAbstractItemView>
#include <QFormLayout>
#include <QGroupBox>
#include <QHeaderView>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QTabWidget>
#include <QVBoxLayout>

namespace {
QTableWidget *createTable(QWidget *parent, const QStringList &headers)
{
    auto *table = new QTableWidget(parent);
    table->setColumnCount(headers.size());
    table->setHorizontalHeaderLabels(headers);
    table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    table->setSelectionBehavior(QAbstractItemView::SelectRows);
    table->setSelectionMode(QAbstractItemView::SingleSelection);
    table->setAlternatingRowColors(true);
    table->verticalHeader()->setVisible(false);
    table->horizontalHeader()->setStretchLastSection(true);
    return table;
}
}

TicketServiceDialog::TicketServiceDialog(TicketManager &ticketManager,
                                         const LoginResult &loginResult,
                                         int initialTabIndex,
                                         QWidget *parent)
    : QDialog(parent)
    , m_ticketManager(&ticketManager)
    , m_loginResult(loginResult)
{
    setWindowTitle(QStringLiteral("票务服务中心"));
    setModal(true);
    resize(980, 680);

    setStyleSheet(QStringLiteral(R"QSS(
        QDialog {
            background: #eef2f3;
            color: #1f2933;
            font-family: "Microsoft YaHei UI", "Microsoft YaHei", "Segoe UI";
            font-size: 14px;
        }
        QLabel {
            color: #1f2933;
        }
        QLabel#titleLabel {
            color: #17231f;
            font-size: 22px;
            font-weight: 700;
        }
        QLabel#hintLabel {
            color: #42514b;
        }
        QLabel#messageLabel {
            border-radius: 8px;
            padding: 8px 10px;
        }
        QGroupBox {
            background: #fbfcfb;
            color: #33433d;
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
        QTabWidget::pane {
            border: 1px solid #d8e0dc;
            background: #fbfcfb;
            border-radius: 10px;
        }
        QTabBar::tab {
            background: #e5ece8;
            color: #33433d;
            padding: 8px 16px;
            margin-right: 6px;
            border-top-left-radius: 8px;
            border-top-right-radius: 8px;
        }
        QTabBar::tab:selected {
            background: #176b5b;
            color: #ffffff;
        }
        QLineEdit {
            min-height: 34px;
            border: 1px solid #cbd8d2;
            border-radius: 8px;
            padding: 4px 10px;
            background: #ffffff;
            color: #1f2933;
            selection-background-color: #0f766e;
            placeholder-text-color: #75857e;
        }
        QLineEdit:focus {
            border: 2px solid #0f766e;
            padding: 3px 9px;
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
        QPushButton#dangerButton {
            background: #b91c1c;
        }
        QPushButton#dangerButton:hover {
            background: #991b1b;
        }
    )QSS"));

    auto *rootLayout = new QVBoxLayout(this);
    rootLayout->setContentsMargins(24, 22, 24, 22);
    rootLayout->setSpacing(14);

    auto *titleLabel = new QLabel(QStringLiteral("票务服务中心"), this);
    titleLabel->setObjectName(QStringLiteral("titleLabel"));
    auto *hintLabel = new QLabel(QStringLiteral("查询车次、办理订票退改签，以及查看订单信息。"), this);
    hintLabel->setObjectName(QStringLiteral("hintLabel"));
    hintLabel->setWordWrap(true);

    m_messageLabel = new QLabel(QStringLiteral(" "), this);
    m_messageLabel->setObjectName(QStringLiteral("messageLabel"));
    m_messageLabel->setMinimumHeight(38);
    m_messageLabel->setWordWrap(true);

    m_tabWidget = new QTabWidget(this);

    setupSearchTab();
    setupManageTab();
    setupQueryTab();
    if (m_loginResult.role == UserRole::Guest) {
        m_tabWidget->setTabEnabled(1, false);
        m_tabWidget->setTabEnabled(2, false);
    }
    m_tabWidget->setCurrentIndex(initialTabIndex);

    auto *closeButton = new QPushButton(QStringLiteral("关闭"), this);
    connect(closeButton, &QPushButton::clicked, this, &QDialog::accept);

    rootLayout->addWidget(titleLabel);
    rootLayout->addWidget(hintLabel);
    rootLayout->addWidget(m_messageLabel);
    rootLayout->addWidget(m_tabWidget);
    rootLayout->addWidget(closeButton, 0, Qt::AlignRight);

    if (m_loginResult.role == UserRole::Guest) {
        showMessage(true, QStringLiteral("游客可先查询车次；如需订票、退改签或查看订单，请先注册并登录。"));
    } else if (m_loginResult.role == UserRole::User) {
        loadOwnOrders();
    }
}

void TicketServiceDialog::setupSearchTab()
{
    auto *tab = new QWidget(this);
    auto *layout = new QVBoxLayout(tab);
    layout->setContentsMargins(16, 16, 16, 16);
    layout->setSpacing(12);

    auto *searchGroup = new QGroupBox(QStringLiteral("车次查询"), tab);
    auto *searchLayout = new QVBoxLayout(searchGroup);
    auto *formLayout = new QFormLayout;
    formLayout->setHorizontalSpacing(14);
    formLayout->setVerticalSpacing(10);

    m_departureEdit = new QLineEdit(searchGroup);
    m_departureEdit->setPlaceholderText(QStringLiteral("例如：北京南"));
    m_arrivalEdit = new QLineEdit(searchGroup);
    m_arrivalEdit->setPlaceholderText(QStringLiteral("例如：上海虹桥"));
    m_trainNumberEdit = new QLineEdit(searchGroup);
    m_trainNumberEdit->setPlaceholderText(QStringLiteral("例如：G1001"));

    formLayout->addRow(QStringLiteral("出发站"), m_departureEdit);
    formLayout->addRow(QStringLiteral("到达站"), m_arrivalEdit);
    formLayout->addRow(QStringLiteral("车次编号"), m_trainNumberEdit);

    auto *searchButton = new QPushButton(QStringLiteral("开始查询"), searchGroup);
    connect(searchButton, &QPushButton::clicked, this, [this]() { searchTrains(); });

    searchLayout->addLayout(formLayout);
    searchLayout->addWidget(searchButton, 0, Qt::AlignRight);

    m_searchResultsTable = createTable(tab, {
        QStringLiteral("车次ID"),
        QStringLiteral("车次编号"),
        QStringLiteral("出发站"),
        QStringLiteral("到达站"),
        QStringLiteral("出发时间"),
        QStringLiteral("到达时间"),
        QStringLiteral("余票")
    });

    auto *bookingGroup = new QGroupBox(QStringLiteral("订票"), tab);
    auto *bookingLayout = new QVBoxLayout(bookingGroup);
    auto *bookingForm = new QFormLayout;
    m_passengerNameEdit = new QLineEdit(bookingGroup);
    m_passengerNameEdit->setPlaceholderText(QStringLiteral("请输入乘客姓名"));
    bookingForm->addRow(QStringLiteral("乘客姓名"), m_passengerNameEdit);

    auto *bookButton = new QPushButton(QStringLiteral("预订选中车次"), bookingGroup);
    connect(bookButton, &QPushButton::clicked, this, [this]() { bookSelectedTrain(); });

    bookingLayout->addLayout(bookingForm);
    bookingLayout->addWidget(bookButton, 0, Qt::AlignRight);

    const bool canBook = m_loginResult.role == UserRole::User
                         || m_loginResult.role == UserRole::Seller
                         || m_loginResult.role == UserRole::Admin;
    bookingGroup->setEnabled(canBook);

    layout->addWidget(searchGroup);
    layout->addWidget(m_searchResultsTable);
    layout->addWidget(bookingGroup);

    m_tabWidget->addTab(tab, QStringLiteral("查询与订票"));
}

void TicketServiceDialog::setupManageTab()
{
    auto *tab = new QWidget(this);
    auto *layout = new QVBoxLayout(tab);
    layout->setContentsMargins(16, 16, 16, 16);
    layout->setSpacing(12);

    auto *refundGroup = new QGroupBox(QStringLiteral("退票"), tab);
    auto *refundLayout = new QVBoxLayout(refundGroup);
    auto *refundForm = new QFormLayout;
    m_refundOrderIdEdit = new QLineEdit(refundGroup);
    m_refundOrderIdEdit->setPlaceholderText(QStringLiteral("请输入订单号"));
    refundForm->addRow(QStringLiteral("订单号"), m_refundOrderIdEdit);
    auto *refundButton = new QPushButton(QStringLiteral("办理退票"), refundGroup);
    refundButton->setObjectName(QStringLiteral("dangerButton"));
    connect(refundButton, &QPushButton::clicked, this, [this]() { refundOrder(); });
    refundLayout->addLayout(refundForm);
    refundLayout->addWidget(refundButton, 0, Qt::AlignRight);

    auto *changeGroup = new QGroupBox(QStringLiteral("改签"), tab);
    auto *changeLayout = new QVBoxLayout(changeGroup);
    auto *changeForm = new QFormLayout;
    m_changeOrderIdEdit = new QLineEdit(changeGroup);
    m_changeOrderIdEdit->setPlaceholderText(QStringLiteral("请输入原订单号"));
    m_changeTrainIdEdit = new QLineEdit(changeGroup);
    m_changeTrainIdEdit->setPlaceholderText(QStringLiteral("请输入新车次ID"));
    changeForm->addRow(QStringLiteral("原订单号"), m_changeOrderIdEdit);
    changeForm->addRow(QStringLiteral("新车次ID"), m_changeTrainIdEdit);
    auto *changeButton = new QPushButton(QStringLiteral("办理改签"), changeGroup);
    connect(changeButton, &QPushButton::clicked, this, [this]() { changeOrder(); });
    changeLayout->addLayout(changeForm);
    changeLayout->addWidget(changeButton, 0, Qt::AlignRight);

    const bool canManage = m_loginResult.role == UserRole::User
                           || m_loginResult.role == UserRole::Seller
                           || m_loginResult.role == UserRole::Admin;
    refundGroup->setEnabled(canManage);
    changeGroup->setEnabled(canManage);

    layout->addWidget(refundGroup);
    layout->addWidget(changeGroup);
    layout->addStretch();

    m_tabWidget->addTab(tab, QStringLiteral("退票与改签"));
}

void TicketServiceDialog::setupQueryTab()
{
    auto *tab = new QWidget(this);
    auto *layout = new QVBoxLayout(tab);
    layout->setContentsMargins(16, 16, 16, 16);
    layout->setSpacing(12);

    auto *queryGroup = new QGroupBox(QStringLiteral("订单查询"), tab);
    auto *queryLayout = new QVBoxLayout(queryGroup);
    auto *queryForm = new QFormLayout;

    m_queryPassengerEdit = new QLineEdit(queryGroup);
    m_queryPassengerEdit->setPlaceholderText(QStringLiteral("售票员/管理员可按乘客姓名查询"));
    m_queryOrderIdEdit = new QLineEdit(queryGroup);
    m_queryOrderIdEdit->setPlaceholderText(QStringLiteral("按订单号精确查询"));

    queryForm->addRow(QStringLiteral("乘客姓名"), m_queryPassengerEdit);
    queryForm->addRow(QStringLiteral("订单号"), m_queryOrderIdEdit);

    auto *buttonLayout = new QHBoxLayout;
    auto *queryButton = new QPushButton(QStringLiteral("开始查询"), queryGroup);
    auto *refreshOwnButton = new QPushButton(QStringLiteral("刷新我的订单"), queryGroup);
    connect(queryButton, &QPushButton::clicked, this, [this]() { runOrderQuery(); });
    connect(refreshOwnButton, &QPushButton::clicked, this, [this]() { loadOwnOrders(); });

    buttonLayout->addWidget(queryButton);
    buttonLayout->addWidget(refreshOwnButton);
    buttonLayout->addStretch();

    queryLayout->addLayout(queryForm);
    queryLayout->addLayout(buttonLayout);

    m_queryResultsTable = createTable(tab, {
        QStringLiteral("订单号"),
        QStringLiteral("用户ID"),
        QStringLiteral("车次ID"),
        QStringLiteral("乘客姓名"),
        QStringLiteral("购票时间"),
        QStringLiteral("状态")
    });

    if (m_loginResult.role == UserRole::User) {
        m_queryPassengerEdit->setEnabled(false);
    }

    layout->addWidget(queryGroup);
    layout->addWidget(m_queryResultsTable);

    m_tabWidget->addTab(tab, QStringLiteral("订单查询"));
}

void TicketServiceDialog::searchTrains()
{
    m_searchResultsTable->setRowCount(0);

    QVector<QVariantMap> results;
    if (!m_trainNumberEdit->text().trimmed().isEmpty()) {
        results = m_ticketManager->searchByTrainNumber(m_trainNumberEdit->text().trimmed());
    } else {
        results = m_ticketManager->searchTrains(m_departureEdit->text().trimmed(),
                                                m_arrivalEdit->text().trimmed());
    }

    m_searchResultsTable->setRowCount(results.size());
    for (int row = 0; row < results.size(); ++row) {
        const QVariantMap &result = results[row];
        m_searchResultsTable->setItem(row, 0, new QTableWidgetItem(QString::number(result.value(QStringLiteral("trainId")).toInt())));
        m_searchResultsTable->setItem(row, 1, new QTableWidgetItem(result.value(QStringLiteral("trainNumber")).toString()));
        m_searchResultsTable->setItem(row, 2, new QTableWidgetItem(result.value(QStringLiteral("departureStation")).toString()));
        m_searchResultsTable->setItem(row, 3, new QTableWidgetItem(result.value(QStringLiteral("arrivalStation")).toString()));
        m_searchResultsTable->setItem(row, 4, new QTableWidgetItem(result.value(QStringLiteral("departureTime")).toString()));
        m_searchResultsTable->setItem(row, 5, new QTableWidgetItem(result.value(QStringLiteral("arrivalTime")).toString()));
        m_searchResultsTable->setItem(row, 6, new QTableWidgetItem(QString::number(result.value(QStringLiteral("remainingSeats")).toInt())));
    }
    m_searchResultsTable->resizeColumnsToContents();
    showMessage(true, results.isEmpty() ? QStringLiteral("没有匹配到车次。")
                                        : QStringLiteral("查询完成，共 %1 条结果。").arg(results.size()));
}

void TicketServiceDialog::bookSelectedTrain()
{
    if (m_loginResult.userId <= 0) {
        showMessage(false, QStringLiteral("请先登录真实账号后再订票。"));
        return;
    }

    const int trainId = selectedTrainId();
    if (trainId <= 0) {
        showMessage(false, QStringLiteral("请先在查询结果中选择一个车次。"));
        return;
    }
    if (m_passengerNameEdit->text().trimmed().isEmpty()) {
        showMessage(false, QStringLiteral("请输入乘客姓名。"));
        return;
    }

    const int orderId = m_ticketManager->bookTicket(m_loginResult.userId,
                                                    trainId,
                                                    m_passengerNameEdit->text().trimmed());
    if (orderId < 0) {
        showMessage(false, m_ticketManager->lastError());
        return;
    }

    showMessage(true, QStringLiteral("订票成功，订单号：%1").arg(orderId));
    writeOperationLog(QStringLiteral("订票"),
                      QStringLiteral("用户 %1 预订车次 %2，订单号 %3")
                          .arg(m_loginResult.username)
                          .arg(trainId)
                          .arg(orderId));
    loadOwnOrders();
}

void TicketServiceDialog::refundOrder()
{
    const int orderId = m_refundOrderIdEdit->text().trimmed().toInt();
    if (orderId <= 0) {
        showMessage(false, QStringLiteral("请输入有效订单号。"));
        return;
    }

    if (!m_ticketManager->refundTicket(orderId)) {
        showMessage(false, m_ticketManager->lastError());
        return;
    }

    showMessage(true, QStringLiteral("退票成功。"));
    writeOperationLog(QStringLiteral("退票"),
                      QStringLiteral("用户 %1 退票，订单号 %2")
                          .arg(m_loginResult.username)
                          .arg(orderId));
    loadOwnOrders();
}

void TicketServiceDialog::changeOrder()
{
    const int orderId = m_changeOrderIdEdit->text().trimmed().toInt();
    const int newTrainId = m_changeTrainIdEdit->text().trimmed().toInt();
    if (orderId <= 0 || newTrainId <= 0) {
        showMessage(false, QStringLiteral("请输入有效的订单号和新车次ID。"));
        return;
    }

    if (!m_ticketManager->changeTicket(orderId, newTrainId)) {
        showMessage(false, m_ticketManager->lastError());
        return;
    }

    showMessage(true, QStringLiteral("改签成功。"));
    writeOperationLog(QStringLiteral("改签"),
                      QStringLiteral("用户 %1 改签，原订单 %2，新车次 %3")
                          .arg(m_loginResult.username)
                          .arg(orderId)
                          .arg(newTrainId));
    loadOwnOrders();
}

void TicketServiceDialog::runOrderQuery()
{
    QVector<QVariantMap> results;
    const QString orderIdText = m_queryOrderIdEdit->text().trimmed();
    const QString passengerName = m_queryPassengerEdit->text().trimmed();

    if (m_loginResult.role == UserRole::Guest) {
        showMessage(false, QStringLiteral("游客不能查询订单，请先登录。"));
        return;
    } else if (m_loginResult.role == UserRole::User) {
        results = m_ticketManager->queryOrdersByUser(m_loginResult.userId);
    } else if (!orderIdText.isEmpty()) {
        results = m_ticketManager->queryOrderByOrderId(orderIdText.toInt());
    } else if (!passengerName.isEmpty()) {
        results = m_ticketManager->queryOrdersByPassenger(passengerName);
    } else {
        results = m_ticketManager->queryAllOrders();
    }

    m_queryResultsTable->setRowCount(results.size());
    for (int row = 0; row < results.size(); ++row) {
        const QVariantMap &order = results[row];
        m_queryResultsTable->setItem(row, 0, new QTableWidgetItem(QString::number(order.value(QStringLiteral("orderId")).toInt())));
        m_queryResultsTable->setItem(row, 1, new QTableWidgetItem(QString::number(order.value(QStringLiteral("userId")).toInt())));
        m_queryResultsTable->setItem(row, 2, new QTableWidgetItem(QString::number(order.value(QStringLiteral("trainId")).toInt())));
        m_queryResultsTable->setItem(row, 3, new QTableWidgetItem(order.value(QStringLiteral("passengerName")).toString()));
        m_queryResultsTable->setItem(row, 4, new QTableWidgetItem(order.value(QStringLiteral("purchaseTime")).toString()));
        m_queryResultsTable->setItem(row, 5, new QTableWidgetItem(statusText(order.value(QStringLiteral("status")).toInt())));
    }
    m_queryResultsTable->resizeColumnsToContents();

    showMessage(true, results.isEmpty() ? QStringLiteral("没有查询到订单。")
                                        : QStringLiteral("订单查询完成，共 %1 条结果。").arg(results.size()));
}

void TicketServiceDialog::loadOwnOrders()
{
    if (m_loginResult.userId <= 0 || m_loginResult.role == UserRole::Guest) {
        return;
    }

    if (m_tabWidget->count() > 2 && m_loginResult.role == UserRole::User) {
        m_tabWidget->setCurrentIndex(m_tabWidget->currentIndex());
    }
    runOrderQuery();
}

void TicketServiceDialog::showMessage(bool success, const QString &message)
{
    if (success) {
        m_messageLabel->setStyleSheet(QStringLiteral(
            "color: #14532d; background: #dcfce7; border: 1px solid #86efac;"));
    } else {
        m_messageLabel->setStyleSheet(QStringLiteral(
            "color: #9f1239; background: #fff1f2; border: 1px solid #fecdd3;"));
    }
    m_messageLabel->setText(message);
}

QString TicketServiceDialog::statusText(int status) const
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

int TicketServiceDialog::selectedTrainId() const
{
    const int row = m_searchResultsTable->currentRow();
    if (row < 0) {
        return 0;
    }

    const QTableWidgetItem *item = m_searchResultsTable->item(row, 0);
    return item == nullptr ? 0 : item->text().toInt();
}

void TicketServiceDialog::writeOperationLog(const QString &action, const QString &detail)
{
    if (m_ticketManager == nullptr) {
        return;
    }
    m_ticketManager->addOperationLog(m_loginResult.username, action, detail);
}
