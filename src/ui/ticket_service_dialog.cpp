#include "ticket_service_dialog.h"

#include "ticket_manager.h"
#include "database_manager.h"
#include "app_style.h"

#include <QAbstractItemView>
#include <QCheckBox>
#include <QDate>
#include <QDateEdit>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QGridLayout>
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
    UiStyle::prepareTable(table);
    return table;
}

QString durationText(int totalMinutes)
{
    if (totalMinutes <= 0) {
        return QStringLiteral("待确认");
    }

    const int hours = totalMinutes / 60;
    const int minutes = totalMinutes % 60;
    if (hours == 0) {
        return QStringLiteral("%1分钟").arg(minutes);
    }
    if (minutes == 0) {
        return QStringLiteral("%1小时").arg(hours);
    }
    return QStringLiteral("%1小时%2分钟").arg(hours).arg(minutes);
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
    resize(1220, 780);
    setMinimumSize(1080, 680);

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
        QGroupBox#compactSearchGroup {
            margin-top: 9px;
            padding: 7px 10px 9px 10px;
        }
        QGroupBox#compactSearchGroup QLineEdit,
        QGroupBox#compactSearchGroup QDateEdit {
            min-height: 28px;
            border: 1px solid #cbd8d2;
            border-radius: 7px;
            padding: 2px 8px;
            background: #ffffff;
        }
        QGroupBox#compactSearchGroup QDateEdit {
            padding: 2px 32px 2px 8px;
        }
        QGroupBox#compactSearchGroup QDateEdit::drop-down {
            subcontrol-origin: padding;
            subcontrol-position: top right;
            width: 28px;
            border-left: 1px solid #d8e0dc;
            background: #eef5f1;
            border-top-right-radius: 6px;
            border-bottom-right-radius: 6px;
        }
        QGroupBox#compactSearchGroup QDateEdit:disabled {
            color: #7b8983;
            background: #edf1ef;
        }
        QLabel#dateArrowLabel {
            color: #42514b;
            background: transparent;
            font-size: 11px;
        }
        QLabel#dateArrowLabel:disabled {
            color: #9aa6a1;
        }
        QGroupBox#compactSearchGroup QPushButton {
            min-height: 28px;
            padding: 3px 12px;
        }
    )QSS"));

    auto *rootLayout = new QVBoxLayout(this);
    rootLayout->setContentsMargins(24, 22, 24, 22);
    rootLayout->setSpacing(14);

    auto *titleLabel = new QLabel(QStringLiteral("票务服务中心"), this);
    titleLabel->setObjectName(QStringLiteral("titleLabel"));
    auto *hintLabel = new QLabel(QStringLiteral("查询列车时刻、余票和动态票价，办理订票退改签并查看订单。"), this);
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

    // 打开车票查询时先显示全部运营车次，乘客不输入条件也能直接看时刻表。
    searchTrips();

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

    auto *scheduleTitle = new QLabel(QStringLiteral("车次时刻表"), tab);
    scheduleTitle->setObjectName(QStringLiteral("titleLabel"));
    auto *scheduleDescription = new QLabel(
        QStringLiteral("可查看全部运营车次，也可以按站点、车次和日期筛选。"), tab);
    scheduleDescription->setObjectName(QStringLiteral("hintLabel"));

    auto *scheduleHeaderLayout = new QHBoxLayout;
    scheduleHeaderLayout->setSpacing(14);
    scheduleHeaderLayout->addWidget(scheduleTitle);
    scheduleHeaderLayout->addWidget(scheduleDescription);
    scheduleHeaderLayout->addStretch();

    auto *searchGroup = new QGroupBox(QStringLiteral("查询条件"), tab);
    searchGroup->setObjectName(QStringLiteral("compactSearchGroup"));
    auto *searchLayout = new QVBoxLayout(searchGroup);
    searchLayout->setContentsMargins(10, 7, 10, 8);
    auto *conditionLayout = new QGridLayout;
    conditionLayout->setHorizontalSpacing(10);
    conditionLayout->setVerticalSpacing(0);
    conditionLayout->setColumnStretch(1, 1);
    conditionLayout->setColumnStretch(3, 1);
    conditionLayout->setColumnStretch(5, 1);

    m_departureEdit = new QLineEdit(searchGroup);
    m_departureEdit->setPlaceholderText(QStringLiteral("例如：北京南"));
    m_arrivalEdit = new QLineEdit(searchGroup);
    m_arrivalEdit->setPlaceholderText(QStringLiteral("例如：上海虹桥"));
    m_trainNumberEdit = new QLineEdit(searchGroup);
    m_trainNumberEdit->setPlaceholderText(QStringLiteral("例如：G1001"));
    m_travelDateEdit = new QDateEdit(QDate::currentDate(), searchGroup);
    m_travelDateEdit->setDisplayFormat(QStringLiteral("yyyy-MM-dd"));
    m_travelDateEdit->setCalendarPopup(true);
    m_travelDateEdit->setFixedWidth(155);
    m_travelDateEdit->setToolTip(QStringLiteral("选择需要查询的出行日期"));

    auto *dateArrowLabel = new QLabel(QStringLiteral("▼"), m_travelDateEdit);
    dateArrowLabel->setObjectName(QStringLiteral("dateArrowLabel"));
    dateArrowLabel->setGeometry(126, 0, 28, 34);
    dateArrowLabel->setAlignment(Qt::AlignCenter);
    dateArrowLabel->setAttribute(Qt::WA_TransparentForMouseEvents);
    m_allDatesCheckBox = new QCheckBox(QStringLiteral("不限日期"), searchGroup);
    m_allDatesCheckBox->setChecked(true);
    m_travelDateEdit->setEnabled(false);

    dateArrowLabel->setEnabled(false);
    connect(m_allDatesCheckBox, &QCheckBox::toggled, this, [this, dateArrowLabel](bool checked) {
        m_travelDateEdit->setEnabled(!checked);
        dateArrowLabel->setEnabled(!checked);
    });

    auto *searchButton = new QPushButton(QStringLiteral("开始查询"), searchGroup);
    auto *showAllButton = new QPushButton(QStringLiteral("显示全部"), searchGroup);
    connect(searchButton, &QPushButton::clicked, this, [this]() { searchTrips(); });
    connect(showAllButton, &QPushButton::clicked, this, [this]() {
        m_departureEdit->clear();
        m_arrivalEdit->clear();
        m_trainNumberEdit->clear();
        m_allDatesCheckBox->setChecked(true);
        searchTrips();
    });

    conditionLayout->addWidget(new QLabel(QStringLiteral("出发站"), searchGroup), 0, 0);
    conditionLayout->addWidget(m_departureEdit, 0, 1);
    conditionLayout->addWidget(new QLabel(QStringLiteral("到达站"), searchGroup), 0, 2);
    conditionLayout->addWidget(m_arrivalEdit, 0, 3);
    conditionLayout->addWidget(new QLabel(QStringLiteral("车次编号"), searchGroup), 0, 4);
    conditionLayout->addWidget(m_trainNumberEdit, 0, 5);
    conditionLayout->addWidget(new QLabel(QStringLiteral("出行日期"), searchGroup), 0, 6);
    conditionLayout->addWidget(m_travelDateEdit, 0, 7);
    conditionLayout->addWidget(m_allDatesCheckBox, 0, 8);
    conditionLayout->addWidget(searchButton, 0, 9);
    conditionLayout->addWidget(showAllButton, 0, 10);
    searchLayout->addLayout(conditionLayout);

    m_searchResultsTable = createTable(tab, {
        QStringLiteral("班次ID"),
        QStringLiteral("车次编号"),
        QStringLiteral("出行日期"),
        QStringLiteral("出发站"),
        QStringLiteral("到达站"),
        QStringLiteral("出发时间"),
        QStringLiteral("到达时间"),
        QStringLiteral("行程时长"),
        QStringLiteral("余票"),
        QStringLiteral("动态票价")
    });
    m_searchResultsTable->setWordWrap(false);
    m_searchResultsTable->setMinimumHeight(400);
    m_searchResultsTable->setHorizontalScrollMode(QAbstractItemView::ScrollPerPixel);
    m_searchResultsTable->verticalHeader()->setDefaultSectionSize(36);

    // 站点名称需要多一点空间，其他列按实际内容排版，避免最后一列被无限拉长。
    QHeaderView *scheduleHeader = m_searchResultsTable->horizontalHeader();
    scheduleHeader->setStretchLastSection(false);
    scheduleHeader->setMinimumSectionSize(72);
    scheduleHeader->setSectionResizeMode(QHeaderView::ResizeToContents);
    scheduleHeader->setSectionResizeMode(2, QHeaderView::Stretch);
    scheduleHeader->setSectionResizeMode(3, QHeaderView::Stretch);

    auto *actionLayout = new QHBoxLayout;
    auto *scheduleHint = new QLabel(
        QStringLiteral("选择一班车次后，可继续办理订票。"), tab);
    auto *bookButton = new QPushButton(QStringLiteral("预订选中班次"), tab);
    connect(bookButton, &QPushButton::clicked, this, [this]() { bookSelectedTrain(); });

    const bool canBook = m_loginResult.role == UserRole::User
                         || m_loginResult.role == UserRole::Seller
                         || m_loginResult.role == UserRole::Admin;
    bookButton->setEnabled(canBook);
    if (!canBook) {
        scheduleHint->setText(QStringLiteral("游客可以查看时刻表，登录后可办理订票。"));
    }

    actionLayout->addWidget(scheduleHint);
    actionLayout->addStretch();
    actionLayout->addWidget(bookButton);

    layout->addLayout(scheduleHeaderLayout);
    layout->addWidget(searchGroup);
    layout->addWidget(m_searchResultsTable, 1);
    layout->addLayout(actionLayout);

    m_tabWidget->addTab(tab, QStringLiteral("车票查询"));
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
    m_changeTrainIdEdit->setPlaceholderText(QStringLiteral("请输入新班次ID"));
    changeForm->addRow(QStringLiteral("原订单号"), m_changeOrderIdEdit);
    changeForm->addRow(QStringLiteral("新班次ID"), m_changeTrainIdEdit);
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
    QHeaderView *queryHeader = m_queryResultsTable->horizontalHeader();
    queryHeader->setStretchLastSection(false);
    queryHeader->setSectionResizeMode(QHeaderView::ResizeToContents);
    queryHeader->setSectionResizeMode(3, QHeaderView::Stretch);
    queryHeader->setSectionResizeMode(4, QHeaderView::Stretch);

    if (m_loginResult.role == UserRole::User) {
        m_queryPassengerEdit->setEnabled(false);
    }

    layout->addWidget(queryGroup);
    layout->addWidget(m_queryResultsTable, 1);

    m_tabWidget->addTab(tab, QStringLiteral("订单查询"));
}

void TicketServiceDialog::searchTrips()
{
    m_searchResultsTable->setRowCount(0);

    const QString travelDate = m_allDatesCheckBox->isChecked()
                                   ? QString()
                                   : m_travelDateEdit->date().toString(QStringLiteral("yyyy-MM-dd"));
    QVector<QVariantMap> results;
    if (!m_trainNumberEdit->text().trimmed().isEmpty()) {
        results = m_ticketManager->searchByTrainNumber(m_trainNumberEdit->text().trimmed());
        if (!travelDate.isEmpty() && !results.isEmpty()) {
            QVector<QVariantMap> filteredResults;
            for (const QVariantMap &result : results) {
                if (result.value(QStringLiteral("travelDate")).toString() == travelDate) {
                    filteredResults.append(result);
                }
            }
            results = filteredResults;
        }
    } else {
        results = m_ticketManager->searchTrips(m_departureEdit->text().trimmed(),
                                               m_arrivalEdit->text().trimmed(),
                                               travelDate);
    }

    m_searchResultsTable->setRowCount(results.size());
    for (int row = 0; row < results.size(); ++row) {
        const QVariantMap &result = results[row];
        m_searchResultsTable->setItem(row, 0, new QTableWidgetItem(QString::number(result.value(QStringLiteral("tripId")).toInt())));
        m_searchResultsTable->setItem(row, 1, new QTableWidgetItem(result.value(QStringLiteral("trainNumber")).toString()));
        m_searchResultsTable->setItem(row, 2, new QTableWidgetItem(result.value(QStringLiteral("travelDate")).toString()));
        m_searchResultsTable->setItem(row, 3, new QTableWidgetItem(result.value(QStringLiteral("departureStation")).toString()));
        m_searchResultsTable->setItem(row, 4, new QTableWidgetItem(result.value(QStringLiteral("arrivalStation")).toString()));
        m_searchResultsTable->setItem(row, 5, new QTableWidgetItem(result.value(QStringLiteral("departureTime")).toString()));
        m_searchResultsTable->setItem(row, 6, new QTableWidgetItem(result.value(QStringLiteral("arrivalTime")).toString()));
        m_searchResultsTable->setItem(row, 7, new QTableWidgetItem(durationText(
            result.value(QStringLiteral("travelMinutes")).toInt())));
        m_searchResultsTable->setItem(row, 8, new QTableWidgetItem(QString::number(result.value(QStringLiteral("remainingSeats")).toInt())));
        const double price = result.value(QStringLiteral("dynamicPrice")).toDouble();
        m_searchResultsTable->setItem(row, 9, new QTableWidgetItem(
            price > 0.0
                ? QStringLiteral("￥%1").arg(price, 0, 'f', 0)
                : QStringLiteral("待确认")));

        for (int column : {0, 1, 2, 3, 4, 5, 6, 7, 8, 9}) {
            m_searchResultsTable->item(row, column)->setTextAlignment(Qt::AlignCenter);
        }
    }
    showMessage(true, results.isEmpty() ? QStringLiteral("没有匹配到车次。")
                                        : QStringLiteral("查询完成，共 %1 条结果。").arg(results.size()));
}

void TicketServiceDialog::bookSelectedTrain()
{
    if (m_loginResult.userId <= 0) {
        showMessage(false, QStringLiteral("请先登录真实账号后再订票。"));
        return;
    }

    const int tripId = selectedTripId();
    if (tripId <= 0) {
        showMessage(false, QStringLiteral("请先在查询结果中选择一个班次。"));
        return;
    }

    QDialog passengerDialog(this);
    passengerDialog.setWindowTitle(QStringLiteral("填写乘客信息"));
    passengerDialog.setModal(true);
    passengerDialog.resize(520, 250);
    passengerDialog.setMinimumSize(480, 230);
    passengerDialog.setStyleSheet(QStringLiteral(R"QSS(
        QDialog {
            background: #f4f7f5;
            color: #1f2933;
            font-family: "Microsoft YaHei UI", "Microsoft YaHei", "Segoe UI";
            font-size: 14px;
        }
        QLabel#passengerTitle {
            color: #173c34;
            font-size: 20px;
            font-weight: 700;
        }
        QLabel#passengerHint {
            color: #586861;
        }
        QLineEdit {
            min-height: 40px;
            border: 1px solid #b8cbc3;
            border-radius: 8px;
            padding: 4px 12px;
            background: #ffffff;
        }
        QLineEdit:focus {
            border: 2px solid #0f766e;
            padding: 3px 11px;
        }
        QPushButton {
            min-width: 90px;
            min-height: 34px;
            border: none;
            border-radius: 8px;
            padding: 4px 14px;
            color: #ffffff;
            background: #176b5b;
            font-weight: 700;
        }
        QPushButton:hover {
            background: #0f5749;
        }
    )QSS"));

    auto *dialogLayout = new QVBoxLayout(&passengerDialog);
    dialogLayout->setContentsMargins(28, 24, 28, 22);
    dialogLayout->setSpacing(12);

    auto *dialogTitle = new QLabel(QStringLiteral("乘客信息"), &passengerDialog);
    dialogTitle->setObjectName(QStringLiteral("passengerTitle"));
    auto *dialogHint = new QLabel(
        QStringLiteral("请输入本次乘车人的姓名，确认后将生成订单。"), &passengerDialog);
    dialogHint->setObjectName(QStringLiteral("passengerHint"));
    auto *passengerNameEdit = new QLineEdit(&passengerDialog);
    passengerNameEdit->setPlaceholderText(QStringLiteral("请输入乘客姓名"));
    passengerNameEdit->setFocus();

    auto *dialogButtons = new QDialogButtonBox(
        QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &passengerDialog);
    dialogButtons->button(QDialogButtonBox::Ok)->setText(QStringLiteral("确认订票"));
    dialogButtons->button(QDialogButtonBox::Cancel)->setText(QStringLiteral("取消"));
    connect(dialogButtons, &QDialogButtonBox::accepted, &passengerDialog, &QDialog::accept);
    connect(dialogButtons, &QDialogButtonBox::rejected, &passengerDialog, &QDialog::reject);

    dialogLayout->addWidget(dialogTitle);
    dialogLayout->addWidget(dialogHint);
    dialogLayout->addWidget(passengerNameEdit);
    dialogLayout->addStretch();
    dialogLayout->addWidget(dialogButtons);

    if (passengerDialog.exec() != QDialog::Accepted) {
        return;
    }
    const QString passengerName = passengerNameEdit->text().trimmed();
    if (passengerName.isEmpty()) {
        showMessage(false, QStringLiteral("请输入乘客姓名。"));
        return;
    }

    const int orderId = m_ticketManager->bookTicket(m_loginResult.userId,
                                                    tripId,
                                                    passengerName);
    if (orderId < 0) {
        showMessage(false, m_ticketManager->lastError());
        return;
    }

    showMessage(true, QStringLiteral("订票成功，订单号：%1").arg(orderId));
    writeOperationLog(QStringLiteral("订票"),
                      QStringLiteral("用户 %1 预订班次 %2，订单号 %3")
                          .arg(m_loginResult.username)
                          .arg(tripId)
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
    const int newTripId = m_changeTrainIdEdit->text().trimmed().toInt();
    if (orderId <= 0 || newTripId <= 0) {
        showMessage(false, QStringLiteral("请输入有效的订单号和新班次ID。"));
        return;
    }

    if (!m_ticketManager->changeTicket(orderId, newTripId)) {
        showMessage(false, m_ticketManager->lastError());
        return;
    }

    showMessage(true, QStringLiteral("改签成功。"));
    writeOperationLog(QStringLiteral("改签"),
                      QStringLiteral("用户 %1 改签，原订单 %2，新班次 %3")
                          .arg(m_loginResult.username)
                          .arg(orderId)
                          .arg(newTripId));
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

int TicketServiceDialog::selectedTripId() const
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
