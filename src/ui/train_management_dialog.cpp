#include "train_management_dialog.h"
#include "database_manager.h"
#include "station_management_dialog.h"
#include "app_style.h"
#include "train_management_dialog_support.h"

#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QVBoxLayout>

TrainManagementDialog::TrainManagementDialog(TrainManager* manager, QWidget *parent)
    : QDialog(parent)
    , m_manager(manager)
{
    setStyleSheet(UiStyle::dialogStyleSheet());
    setupUI();
    loadStations();
    loadData();
    setWindowTitle("车次管理");
    resize(1280, 760);
    setMinimumSize(1100, 680);
}

void TrainManagementDialog::setupUI()
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(18, 16, 18, 14);
    mainLayout->setSpacing(12);

    // -------- 搜索栏 --------
    QHBoxLayout *searchLayout = new QHBoxLayout();

    m_searchInput = new QLineEdit();
    m_searchInput->setPlaceholderText("输入车次号或站点名称...");
    m_searchInput->setMinimumWidth(300);

    m_searchBtn = new QPushButton("搜索");
    m_refreshBtn = new QPushButton("刷新");

    m_stationCombo = new StationComboBox();
    m_stationCombo->setObjectName(QStringLiteral("stationSearchCombo"));
    m_stationCombo->addItem("-- 选择出发站 --");
    m_stationCombo->setMinimumWidth(210);

    m_searchStationBtn = new QPushButton("按出发站查询");
    QPushButton *stationBtn = new QPushButton("站点管理");
    connect(stationBtn, &QPushButton::clicked, [this]() {
        StationManagementDialog dialog(m_manager->databaseManager(), this);
        dialog.exec();
        loadStations();
        loadData();
    });

    searchLayout->addWidget(m_searchInput);
    searchLayout->addWidget(m_searchBtn);
    searchLayout->addSpacing(10);
    searchLayout->addWidget(m_stationCombo);
    searchLayout->addWidget(m_searchStationBtn);
    searchLayout->addStretch();
    searchLayout->addWidget(stationBtn);
    searchLayout->addWidget(m_refreshBtn);
    mainLayout->addLayout(searchLayout);

    // -------- 车次列表 --------
    m_table = new QTableWidget();
    m_table->setColumnCount(8);
    m_table->setHorizontalHeaderLabels({
        "ID", "车次号", "出发站", "到达站",
        "出发时间", "到达时间", "总座位", "状态"
    });
    UiStyle::prepareTable(m_table);
    m_table->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);

    // -------- 主内容区域（TabWidget） --------
    m_tabWidget = new QTabWidget(this);

    // Tab 1：车次信息
    auto *trainTab = new QWidget();
    auto *trainLayout = new QVBoxLayout(trainTab);
    trainLayout->setContentsMargins(16, 16, 16, 14);
    trainLayout->setSpacing(12);
    trainLayout->addWidget(m_table);
    m_tabWidget->addTab(trainTab, "车次信息");

    // Tab 2：班次管理
    auto *tripTab = new QWidget();
    auto *tripLayout = new QVBoxLayout(tripTab);
    tripLayout->setContentsMargins(16, 16, 16, 14);
    tripLayout->setSpacing(12);

    // 信息栏（车次信息 + 返回按钮）
    QHBoxLayout *infoLayout = new QHBoxLayout();
    infoLayout->setContentsMargins(0, 0, 0, 0);

    m_trainInfoLabel = new QLabel("请选择车次", tripTab);
    m_trainInfoLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    m_trainInfoLabel->setStyleSheet(
        "QLabel {"
        "    color: #153832;"
        "    font-size: 14px;"
        "    font-weight: bold;"
        "    background-color: #eef5f1;"
        "    border: 1px solid #cbd8d2;"
        "    border-radius: 8px;"
        "    padding: 10px 16px;"
        "}"
        );
    m_trainInfoLabel->setAlignment(Qt::AlignCenter);

    m_backToTrainBtn = new QPushButton("← 返回车次列表", tripTab);
    m_backToTrainBtn->setObjectName(QStringLiteral("secondaryButton"));
    connect(m_backToTrainBtn, &QPushButton::clicked, this, &TrainManagementDialog::goBackToTrainList);

    infoLayout->addWidget(m_trainInfoLabel, 1);
    infoLayout->addWidget(m_backToTrainBtn);
    tripLayout->addLayout(infoLayout);

    // 班次表格
    m_tripTable = new QTableWidget(tripTab);
    m_tripTable->setColumnCount(7);
    m_tripTable->setHorizontalHeaderLabels({
        "班次ID", "日期", "出发时间", "到达时间",
        "余票/总座", "基础票价", "动态票价"
    });
    UiStyle::prepareTable(m_tripTable);
    m_tripTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    tripLayout->addWidget(m_tripTable, 1);

    // 班次操作按钮
    auto *tripActionLayout = new QHBoxLayout();

    m_generateTripBtn = new QPushButton("生成班次");
    m_editTripBtn = new QPushButton("编辑");
    m_disableTripBtn = new QPushButton("停运");
    m_disableTripBtn->setObjectName(QStringLiteral("dangerButton"));
    m_historyToggleBtn = new QPushButton("查看历史班次");
    m_historyToggleBtn->setObjectName(QStringLiteral("secondaryButton"));
    m_historyToggleBtn->setCheckable(false);
    connect(m_historyToggleBtn, &QPushButton::clicked, this, &TrainManagementDialog::toggleHistoryMode);

    tripActionLayout->addWidget(m_generateTripBtn);
    tripActionLayout->addWidget(m_editTripBtn);
    tripActionLayout->addWidget(m_disableTripBtn);
    tripActionLayout->addStretch();
    tripActionLayout->addWidget(m_historyToggleBtn);

    tripLayout->addLayout(tripActionLayout);

    m_tabWidget->addTab(tripTab, "班次");
    mainLayout->addWidget(m_tabWidget, 1);

    // -------- 操作按钮（车次列表下方） --------
    QHBoxLayout *actionLayout = new QHBoxLayout();

    m_addBtn = new QPushButton("添加车次");

    m_editBtn = new QPushButton("编辑");
    m_editBtn->setEnabled(false);

    m_deleteBtn = new QPushButton("停运");
    m_deleteBtn->setEnabled(false);
    m_deleteBtn->setObjectName(QStringLiteral("dangerButton"));

    m_resumeBtn = new QPushButton("恢复运营");
    m_resumeBtn->setEnabled(false);

    m_purgeBtn = new QPushButton("物理删除");
    m_purgeBtn->setEnabled(false);
    m_purgeBtn->setObjectName(QStringLiteral("dangerButton"));

    m_seatBtn = new QPushButton("座位管理");
    m_seatBtn->setEnabled(false);

    m_countLabel = new QLabel("共 0 条记录");
    m_countLabel->setStyleSheet("color: #42514b; font-size: 13px;");

    m_messageLabel = new QLabel("就绪");
    m_messageLabel->setStyleSheet("color: #28a745; font-size: 13px;");

    actionLayout->addWidget(m_addBtn);
    actionLayout->addWidget(m_editBtn);
    actionLayout->addWidget(m_deleteBtn);
    actionLayout->addWidget(m_resumeBtn);
    actionLayout->addWidget(m_purgeBtn);
    actionLayout->addWidget(m_seatBtn);
    actionLayout->addStretch();
    actionLayout->addWidget(m_countLabel);
    trainLayout->addLayout(actionLayout);
    mainLayout->addWidget(m_messageLabel, 0, Qt::AlignRight);

    // -------- 信号连接 --------
    connect(m_refreshBtn, &QPushButton::clicked, this, &TrainManagementDialog::refreshList);
    connect(m_searchBtn, &QPushButton::clicked, this, &TrainManagementDialog::searchTrain);
    connect(m_searchStationBtn, &QPushButton::clicked, this, &TrainManagementDialog::searchByStation);
    connect(m_addBtn, &QPushButton::clicked, this, &TrainManagementDialog::addTrain);
    connect(m_editBtn, &QPushButton::clicked, this, &TrainManagementDialog::editTrain);
    connect(m_deleteBtn, &QPushButton::clicked, this, &TrainManagementDialog::deleteTrain);
    connect(m_resumeBtn, &QPushButton::clicked, this, &TrainManagementDialog::resumeTrain);
    connect(m_purgeBtn, &QPushButton::clicked, this, &TrainManagementDialog::deleteTrainPermanently);
    connect(m_seatBtn, &QPushButton::clicked, this, &TrainManagementDialog::updateSeats);
    connect(m_searchInput, &QLineEdit::returnPressed, this, &TrainManagementDialog::searchTrain);
    connect(m_table, &QTableWidget::itemSelectionChanged, this, &TrainManagementDialog::onTableRowClicked);
    connect(m_editTripBtn, &QPushButton::clicked, this, &TrainManagementDialog::editTrip);
    connect(m_disableTripBtn, &QPushButton::clicked, this, &TrainManagementDialog::disableTrip);
    connect(m_generateTripBtn, &QPushButton::clicked, this, &TrainManagementDialog::generateTrips);
    connect(m_table, &QTableWidget::doubleClicked,
            this, &TrainManagementDialog::onTrainTableDoubleClicked);
}

void TrainManagementDialog::loadStations()
{
    if (m_manager == nullptr) {
        showMessage("车次管理服务未初始化", false);
        return;
    }

    DatabaseManager *dbManager = m_manager->databaseManager();
    if (dbManager == nullptr) {
        showMessage("数据库管理器未初始化", false);
        return;
    }

    m_stationNameMap.clear();
    m_stationCombo->clear();
    m_stationCombo->addItem("-- 选择出发站 --");

    const QList<StationRecord> stations = dbManager->getAllStations();
    for (const StationRecord &station : stations) {
        m_stationCombo->addItem(station.stationName, station.stationId);
        m_stationNameMap[station.stationId] = station.stationName;
    }
}

void TrainManagementDialog::loadData()
{
    if (m_manager == nullptr) {
        showMessage("车次管理服务未初始化", false);
        return;
    }

    auto trains = m_manager->getAllTrains(false);
    m_table->setRowCount(trains.size());

    for (int i = 0; i < trains.size(); ++i) {
        const Train& t = trains[i];
        m_table->setItem(i, 0, new QTableWidgetItem(QString::number(t.trainId)));
        m_table->setItem(i, 1, new QTableWidgetItem(t.trainNumber));
        m_table->setItem(i, 2, new QTableWidgetItem(
                                   m_stationNameMap.value(t.departureStationId, QString::number(t.departureStationId))
                                   ));
        m_table->setItem(i, 3, new QTableWidgetItem(
                                   m_stationNameMap.value(t.arrivalStationId, QString::number(t.arrivalStationId))
                                   ));
        m_table->setItem(i, 4, new QTableWidgetItem(t.departureTime));
        m_table->setItem(i, 5, new QTableWidgetItem(t.arrivalTime));
        m_table->setItem(i, 6, new QTableWidgetItem(QString::number(t.totalSeats)));
        m_table->setItem(i, 7, new QTableWidgetItem(t.enabled ? "运营中" : "已停运"));
    }

    m_countLabel->setText("总车次 共 " + QString::number(trains.size()) + " 条记录");
    clearSelection();
    showMessage("列表已刷新", true);
}

void TrainManagementDialog::refreshList()
{
    loadData();
}

void TrainManagementDialog::searchTrain()
{
    QString keyword = m_searchInput->text().trimmed();
    if (keyword.isEmpty()) {
        showMessage("请输入搜索关键字", false);
        return;
    }

    auto results = m_manager->searchTrains(keyword);
    m_table->setRowCount(results.size());

    for (int i = 0; i < results.size(); ++i) {
        const Train& t = results[i];
        m_table->setItem(i, 0, new QTableWidgetItem(QString::number(t.trainId)));
        m_table->setItem(i, 1, new QTableWidgetItem(t.trainNumber));
        m_table->setItem(i, 2, new QTableWidgetItem(
                                   m_stationNameMap.value(t.departureStationId, QString::number(t.departureStationId))
                                   ));
        m_table->setItem(i, 3, new QTableWidgetItem(
                                   m_stationNameMap.value(t.arrivalStationId, QString::number(t.arrivalStationId))
                                   ));
        m_table->setItem(i, 4, new QTableWidgetItem(t.departureTime));
        m_table->setItem(i, 5, new QTableWidgetItem(t.arrivalTime));
        m_table->setItem(i, 6, new QTableWidgetItem(QString::number(t.totalSeats)));
        m_table->setItem(i, 7, new QTableWidgetItem(t.enabled ? "运营中" : "已停运"));
    }

    m_countLabel->setText("找到 " + QString::number(results.size()) + " 条记录");
    clearSelection();
    showMessage("搜索完成", true);
}

void TrainManagementDialog::searchByStation()
{
    int index = m_stationCombo->currentIndex();
    if (index <= 0) {
        showMessage("请先选择出发站", false);
        return;
    }

    int stationId = m_stationCombo->currentData().toInt();
    auto results = m_manager->searchByStation(stationId, true);

    m_table->setRowCount(results.size());
    for (int i = 0; i < results.size(); ++i) {
        const Train& t = results[i];
        m_table->setItem(i, 0, new QTableWidgetItem(QString::number(t.trainId)));
        m_table->setItem(i, 1, new QTableWidgetItem(t.trainNumber));
        m_table->setItem(i, 2, new QTableWidgetItem(
                                   m_stationNameMap.value(t.departureStationId, QString::number(t.departureStationId))
                                   ));
        m_table->setItem(i, 3, new QTableWidgetItem(
                                   m_stationNameMap.value(t.arrivalStationId, QString::number(t.arrivalStationId))
                                   ));
        m_table->setItem(i, 4, new QTableWidgetItem(t.departureTime));
        m_table->setItem(i, 5, new QTableWidgetItem(t.arrivalTime));
        m_table->setItem(i, 6, new QTableWidgetItem(QString::number(t.totalSeats)));
        m_table->setItem(i, 7, new QTableWidgetItem(t.enabled ? "运营中" : "已停运"));
    }

    m_countLabel->setText("从该站出发共 " + QString::number(results.size()) + " 条记录");
    clearSelection();
    showMessage("按站查询完成", true);
}

int TrainManagementDialog::getSelectedTrainId()
{
    int row = m_table->currentRow();
    if (row < 0 || row >= m_table->rowCount()) {
        showMessage("请先选择一辆车次", false);
        return 0;
    }
    QTableWidgetItem *idItem = m_table->item(row, 0);
    if (!idItem) return 0;
    return idItem->text().toInt();
}

void TrainManagementDialog::clearSelection()
{
    m_editBtn->setEnabled(false);
    m_deleteBtn->setEnabled(false);
    m_resumeBtn->setEnabled(false);
    m_purgeBtn->setEnabled(false);
    m_seatBtn->setEnabled(false);
    m_table->clearSelection();
}

void TrainManagementDialog::showMessage(const QString &msg, bool success)
{
    m_messageLabel->setText(msg);
    m_messageLabel->setStyleSheet(success ? "color: #28a745;" : "color: #dc3545;");
}

