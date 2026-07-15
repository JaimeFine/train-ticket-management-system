#include "train_management_dialog.h"
#include "database_manager.h"
#include "station_management_dialog.h"
#include "ticket_manager.h"
#include "app_style.h"

#include <QComboBox>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QInputDialog>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPainter>
#include <QPaintEvent>
#include <QPainterPath>
#include <QPushButton>
#include <QSpinBox>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QVBoxLayout>
#include <QTime>
#include <QDoubleSpinBox>
#include <QDebug>

namespace {
// 系统下拉箭头在 Windows 样式下容易偏移，这里只重画右侧箭头区域。
class StationComboBox : public QComboBox
{
protected:
    void paintEvent(QPaintEvent *event) override
    {
        QComboBox::paintEvent(event);

        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing);
        const qreal arrowAreaWidth = 36.0;

        QPainterPath clipPath;
        clipPath.addRoundedRect(QRectF(0.5, 0.5, width() - 1.0, height() - 1.0),
                                8.0, 8.0);
        painter.setClipPath(clipPath);
        painter.fillRect(QRectF(width() - arrowAreaWidth, 0.0,
                                arrowAreaWidth, height()),
                         QColor(QStringLiteral("#eef5f1")));
        painter.setClipping(false);

        painter.setPen(QPen(QColor(QStringLiteral("#d8e0dc")), 1.0));
        painter.drawLine(QPointF(width() - arrowAreaWidth, 1.0),
                         QPointF(width() - arrowAreaWidth, height() - 1.0));

        painter.setPen(Qt::NoPen);
        painter.setBrush(isEnabled() ? QColor(QStringLiteral("#8fa5a0"))
                                     : QColor(QStringLiteral("#bdc8c4")));

        const qreal centerX = width() - arrowAreaWidth / 2.0;
        const qreal centerY = height() / 2.0;
        QPolygonF arrow;
        arrow << QPointF(centerX - 4.5, centerY - 2.5)
              << QPointF(centerX + 4.5, centerY - 2.5)
              << QPointF(centerX, centerY + 4.0);
        painter.drawPolygon(arrow);

        const qreal borderWidth = hasFocus() ? 2.0 : 1.0;
        const qreal borderOffset = borderWidth / 2.0;
        painter.setBrush(Qt::NoBrush);
        painter.setPen(QPen(hasFocus() ? QColor(QStringLiteral("#0f766e"))
                                       : QColor(QStringLiteral("#cbd8d2")),
                            borderWidth));
        painter.drawRoundedRect(QRectF(borderOffset, borderOffset,
                                       width() - borderWidth,
                                       height() - borderWidth),
                                8.0, 8.0);
    }
};

// 添加和编辑窗口使用同一套输入框、按钮样式。
void styleTrainEditDialog(QDialog &dialog)
{
    dialog.setStyleSheet(
        "QDialog {"
        "    background: #eef2f3;"
        "    color: #1f2933;"
        "    font-family: \"Microsoft YaHei UI\", \"Microsoft YaHei\", \"Segoe UI\";"
        "    font-size: 14px;"
        "}"
        "QLabel {"
        "    color: #33433d;"
        "}"
        "QLineEdit, QComboBox, QSpinBox {"
        "    color: #1f2933;"
        "    background-color: #ffffff;"
        "    border: 1px solid #cbd8d2;"
        "    border-radius: 6px;"
        "    padding: 4px 8px;"
        "    min-height: 28px;"
        "}"
        "QLineEdit::placeholder {"
        "    color: #8b9490;"
        "}"
        "QComboBox::drop-down, QSpinBox::up-button, QSpinBox::down-button {"
        "    border: none;"
        "    background: #eef5f1;"
        "}"
        "QComboBox QAbstractItemView {"
        "    color: #1f2933;"
        "    background-color: #ffffff;"
        "    border: 1px solid #cbd8d2;"
        "    selection-background-color: #d9f99d;"
        "    selection-color: #153832;"
        "}"
        "QPushButton {"
        "    background-color: #176b5b;"
        "    color: white;"
        "    border: none;"
        "    border-radius: 8px;"
        "    padding: 8px 14px;"
        "    font-weight: 700;"
        "}"
        "QPushButton:hover {"
        "    background-color: #0f5749;"
        "}"
        );
}
} // namespace

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

void TrainManagementDialog::addTrain()
{
    QDialog dialog(this);
    dialog.setWindowTitle("添加车次");
    dialog.resize(400, 350);
    styleTrainEditDialog(dialog);

    QFormLayout *form = new QFormLayout(&dialog);

    QLineEdit *trainNumberEdit = new QLineEdit();
    trainNumberEdit->setPlaceholderText("如 G1234");
    QComboBox *departCombo = new QComboBox();
    QComboBox *arriveCombo = new QComboBox();
    QLineEdit *departTimeEdit = new QLineEdit();
    departTimeEdit->setPlaceholderText("如 08:00");
    QLineEdit *arriveTimeEdit = new QLineEdit();
    arriveTimeEdit->setPlaceholderText("如 12:30");
    QSpinBox *totalSeatsSpin = new QSpinBox();
    totalSeatsSpin->setRange(1, 99999);
    totalSeatsSpin->setValue(100);

    DatabaseManager *dbManager = m_manager->databaseManager();
    if (dbManager != nullptr) {
        const QList<StationRecord> stations = dbManager->getAllStations();
        for (const StationRecord &station : stations) {
            departCombo->addItem(station.stationName, station.stationId);
            arriveCombo->addItem(station.stationName, station.stationId);
        }
    }

    form->addRow("车次号:", trainNumberEdit);
    form->addRow("出发站:", departCombo);
    form->addRow("到达站:", arriveCombo);
    form->addRow("出发时间:", departTimeEdit);
    form->addRow("到达时间:", arriveTimeEdit);
    form->addRow("总座位数:", totalSeatsSpin);

    QHBoxLayout *btnLayout = new QHBoxLayout();
    QPushButton *okBtn = new QPushButton("确定");
    QPushButton *cancelBtn = new QPushButton("取消");
    btnLayout->addStretch();
    btnLayout->addWidget(okBtn);
    btnLayout->addWidget(cancelBtn);
    form->addRow(btnLayout);

    connect(okBtn, &QPushButton::clicked, &dialog, &QDialog::accept);
    connect(cancelBtn, &QPushButton::clicked, &dialog, &QDialog::reject);

    if (dialog.exec() == QDialog::Accepted) {
        Train train;
        train.trainNumber = trainNumberEdit->text().trimmed();
        train.departureStationId = departCombo->currentData().toInt();
        train.arrivalStationId = arriveCombo->currentData().toInt();
        train.departureTime = departTimeEdit->text().trimmed();
        train.arrivalTime = arriveTimeEdit->text().trimmed();
        train.totalSeats = totalSeatsSpin->value();
        train.enabled = true;

        // ---- 参数校验 ----
        if (train.trainNumber.isEmpty()) {
            showMessage("车次号不能为空", false);
            return;
        }

        if (train.departureStationId == train.arrivalStationId) {
            showMessage("出发站和到达站不能相同", false);
            return;
        }

        QRegularExpression timeRegex(R"(^([0-1][0-9]|2[0-3]):[0-5][0-9]$)");
        if (!timeRegex.match(train.departureTime).hasMatch()) {
            showMessage("出发时间格式无效，请使用 HH:mm 格式", false);
            return;
        }
        if (!timeRegex.match(train.arrivalTime).hasMatch()) {
            showMessage("到达时间格式无效，请使用 HH:mm 格式", false);
            return;
        }

        QTime dep = QTime::fromString(train.departureTime, "HH:mm");
        QTime arr = QTime::fromString(train.arrivalTime, "HH:mm");
        if (!dep.isValid() || !arr.isValid()) {
            showMessage("时间格式无效", false);
            return;
        }

        if (dep == arr) {
            showMessage("出发时间和到达时间不能相同", false);
            return;
        }

        if (train.totalSeats <= 0) {
            showMessage("总座位数必须大于 0", false);
            return;
        }

        if (m_manager->addTrain(train)) {
            showMessage(m_manager->statusMessage(), true);
            loadData();

            QMessageBox msgBox(this);
            msgBox.setWindowTitle("车次添加成功");
            msgBox.setText("车次 " + train.trainNumber + " 添加成功！\n\n是否为该车次生成班次？");
            msgBox.addButton("生成班次", QMessageBox::AcceptRole);
            msgBox.addButton("稍后添加", QMessageBox::RejectRole);

            if (msgBox.exec() == QMessageBox::Accepted) {
                selectTrainByNumber(train.trainNumber);
                generateTrips();
            }
        }
        else {
            showMessage(m_manager->statusMessage(), false);
        }
    }
}

void TrainManagementDialog::editTrain()
{
    int trainId = getSelectedTrainId();
    if (trainId == 0) return;

    auto all = m_manager->getAllTrains(false);
    Train current;
    for (const auto& t : all) {
        if (t.trainId == trainId) {
            current = t;
            break;
        }
    }
    if (current.trainId == 0) {
        showMessage("未找到该车次", false);
        return;
    }

    QDialog dialog(this);
    dialog.setWindowTitle("编辑车次 - " + current.trainNumber);
    dialog.resize(400, 380);
    styleTrainEditDialog(dialog);

    QFormLayout *form = new QFormLayout(&dialog);

    QLineEdit *trainNumberEdit = new QLineEdit(current.trainNumber);
    QComboBox *departCombo = new QComboBox();
    QComboBox *arriveCombo = new QComboBox();
    QLineEdit *departTimeEdit = new QLineEdit(current.departureTime);
    QLineEdit *arriveTimeEdit = new QLineEdit(current.arrivalTime);
    QSpinBox *totalSeatsSpin = new QSpinBox();
    totalSeatsSpin->setRange(1, 99999);
    totalSeatsSpin->setValue(current.totalSeats);

    DatabaseManager *dbManager = m_manager->databaseManager();
    if (dbManager != nullptr) {
        const QList<StationRecord> stations = dbManager->getAllStations();
        int departIndex = 0;
        int arriveIndex = 0;
        for (int idx = 0; idx < stations.size(); ++idx) {
            const StationRecord &station = stations[idx];
            departCombo->addItem(station.stationName, station.stationId);
            arriveCombo->addItem(station.stationName, station.stationId);
            if (station.stationId == current.departureStationId) departIndex = idx;
            if (station.stationId == current.arrivalStationId) arriveIndex = idx;
        }
        departCombo->setCurrentIndex(departIndex);
        arriveCombo->setCurrentIndex(arriveIndex);
    }

    form->addRow("车次号:", trainNumberEdit);
    form->addRow("出发站:", departCombo);
    form->addRow("到达站:", arriveCombo);
    form->addRow("出发时间:", departTimeEdit);
    form->addRow("到达时间:", arriveTimeEdit);
    form->addRow("总座位数:", totalSeatsSpin);

    QHBoxLayout *btnLayout = new QHBoxLayout();
    QPushButton *okBtn = new QPushButton("确定");
    QPushButton *cancelBtn = new QPushButton("取消");
    btnLayout->addStretch();
    btnLayout->addWidget(okBtn);
    btnLayout->addWidget(cancelBtn);
    form->addRow(btnLayout);

    connect(okBtn, &QPushButton::clicked, &dialog, &QDialog::accept);
    connect(cancelBtn, &QPushButton::clicked, &dialog, &QDialog::reject);

    if (dialog.exec() == QDialog::Accepted) {
        Train train;
        train.trainId = current.trainId;
        train.trainNumber = trainNumberEdit->text().trimmed();
        train.departureStationId = departCombo->currentData().toInt();
        train.arrivalStationId = arriveCombo->currentData().toInt();
        train.departureTime = departTimeEdit->text().trimmed();
        train.arrivalTime = arriveTimeEdit->text().trimmed();
        train.totalSeats = totalSeatsSpin->value();
        train.enabled = current.enabled;

        if (train.trainNumber.isEmpty()) {
            showMessage("车次号不能为空", false);
            return;
        }

        if (train.departureStationId == train.arrivalStationId) {
            showMessage("出发站和到达站不能相同", false);
            return;
        }

        QRegularExpression timeRegex(R"(^([0-1][0-9]|2[0-3]):[0-5][0-9]$)");
        if (!timeRegex.match(train.departureTime).hasMatch()) {
            showMessage("出发时间格式无效，请使用 HH:mm 格式", false);
            return;
        }
        if (!timeRegex.match(train.arrivalTime).hasMatch()) {
            showMessage("到达时间格式无效，请使用 HH:mm 格式", false);
            return;
        }

        QTime dep = QTime::fromString(train.departureTime, "HH:mm");
        QTime arr = QTime::fromString(train.arrivalTime, "HH:mm");
        if (!dep.isValid() || !arr.isValid()) {
            showMessage("时间格式无效", false);
            return;
        }

        if (dep == arr) {
            showMessage("出发时间和到达时间不能相同", false);
            return;
        }

        if (train.totalSeats <= 0) {
            showMessage("总座位数必须大于 0", false);
            return;
        }

        if (m_manager->updateTrain(train)) {
            showMessage(m_manager->statusMessage(), true);
            loadData();
        } else {
            showMessage(m_manager->statusMessage(), false);
        }
    }
}

void TrainManagementDialog::deleteTrain()
{
    int trainId = getSelectedTrainId();
    if (trainId == 0) return;

    auto all = m_manager->getAllTrains(false);
    QString trainNumber;
    for (const auto& t : all) {
        if (t.trainId == trainId) {
            trainNumber = t.trainNumber;
            break;
        }
    }

    QMessageBox::StandardButton reply = QMessageBox::question(
        this, "确认停运",
        "确定要停运车次 " + trainNumber + " 吗？",
        QMessageBox::Yes | QMessageBox::No
        );

    if (reply == QMessageBox::Yes) {
        if (m_manager->deleteTrain(trainId)) {
            showMessage(m_manager->statusMessage(), true);
            loadData();
        } else {
            showMessage(m_manager->statusMessage(), false);
        }
    }
}

void TrainManagementDialog::resumeTrain()
{
    int trainId = getSelectedTrainId();
    if (trainId == 0) return;

    auto all = m_manager->getAllTrains(false);
    QString trainNumber;
    for (const auto& t : all) {
        if (t.trainId == trainId) {
            trainNumber = t.trainNumber;
            break;
        }
    }

    QMessageBox::StandardButton reply = QMessageBox::question(
        this, "确认恢复运营",
        "确定要恢复车次 " + trainNumber + " 的运营吗？",
        QMessageBox::Yes | QMessageBox::No
        );

    if (reply == QMessageBox::Yes) {
        if (m_manager->resumeTrain(trainId)) {
            showMessage(m_manager->statusMessage(), true);
            loadData();
        } else {
            showMessage(m_manager->statusMessage(), false);
        }
    }
}

void TrainManagementDialog::deleteTrainPermanently()
{
    int trainId = getSelectedTrainId();
    if (trainId == 0) return;

    auto all = m_manager->getAllTrains(false);
    QString trainNumber;
    for (const auto& t : all) {
        if (t.trainId == trainId) {
            trainNumber = t.trainNumber;
            break;
        }
    }

    QMessageBox::StandardButton reply = QMessageBox::warning(
        this, "确认物理删除",
        "物理删除会彻底移除车次 " + trainNumber + "。\n"
                                                  "只有没有关联订单的车次才能删除。\n\n确定继续吗？",
        QMessageBox::Yes | QMessageBox::No
        );

    if (reply == QMessageBox::Yes) {
        if (m_manager->deleteTrainPermanently(trainId)) {
            showMessage(m_manager->statusMessage(), true);
            loadData();
        } else {
            showMessage(m_manager->statusMessage(), false);
        }
    }
}

void TrainManagementDialog::updateSeats()
{
    int trainId = getSelectedTrainId();
    if (trainId == 0) return;

    auto all = m_manager->getAllTrains(false);
    int currentSeats = 0;
    QString trainNumber;
    for (const auto& t : all) {
        if (t.trainId == trainId) {
            currentSeats = 0;
            trainNumber = t.trainNumber;
            break;
        }
    }

    bool ok;
    int delta = QInputDialog::getInt(
        this, "座位管理 - " + trainNumber,
        "当前余票: " + QString::number(currentSeats) + "\n\n"
                                                       "输入变动数量（负数=售票，正数=退票）:",
        0, -99999, 99999, 1, &ok
        );

    if (ok && delta != 0) {
        showMessage(QStringLiteral("V2: 座位管理请通过Trip表操作"), false);
    }
}

void TrainManagementDialog::onTableRowClicked()
{
    int row = m_table->currentRow();
    bool hasSelection = (row >= 0 && row < m_table->rowCount());
    m_editBtn->setEnabled(hasSelection);
    m_deleteBtn->setEnabled(hasSelection);
    m_resumeBtn->setEnabled(hasSelection);
    m_purgeBtn->setEnabled(hasSelection);
    m_seatBtn->setEnabled(hasSelection);

    int trainId = getSelectedTrainId();
    if (trainId > 0) {
        loadTripsForTrain(trainId);
    }
}

void TrainManagementDialog::toggleHistoryMode()
{
    m_showHistory = !m_showHistory;
    if (m_showHistory) {
        m_historyToggleBtn->setText("查看未发车班次");
        showMessage("切换到历史班次模式（按时间倒序）", true);
    } else {
        m_historyToggleBtn->setText("查看历史班次");
        showMessage("切换到未发车班次模式", true);
    }
    int trainId = getSelectedTrainId();
    if (trainId > 0) {
        loadTripsForTrain(trainId);
    }
}

void TrainManagementDialog::loadTripsForTrain(int trainId)
{
    if (m_manager == nullptr) return;
    auto dbManager = m_manager->databaseManager();
    if (dbManager == nullptr) return;

    auto trainOpt = dbManager->findTrainById(trainId);
    if (!trainOpt.has_value()) {
        showMessage("未找到车次", false);
        return;
    }

    auto depStation = dbManager->findStationById(trainOpt->departureStationId);
    auto arrStation = dbManager->findStationById(trainOpt->arrivalStationId);
    QString depName = depStation.has_value() ? depStation->stationName : "未知";
    QString arrName = arrStation.has_value() ? arrStation->stationName : "未知";

    // 更新标签
    if (m_trainInfoLabel) {
        m_trainInfoLabel->setText(
            QString("当前车次：%1  |  %2 → %3  |  %4 → %5  |  总座位：%6")
                .arg(trainOpt->trainNumber)
                .arg(depName)
                .arg(arrName)
                .arg(trainOpt->departureTime)
                .arg(trainOpt->arrivalTime)
                .arg(trainOpt->totalSeats)
            );
    }

    auto trips = dbManager->findTripsByTrain(trainId);

    QList<TripRecord> filteredTrips;
    QDateTime now = QDateTime::currentDateTime();
    for (const TripRecord& trip : trips) {
        QDateTime depDateTime = QDateTime::fromString(
            trip.travelDate + " " + trip.departureTime,
            "yyyy-MM-dd HH:mm"
            );
        bool isDeparted = depDateTime.isValid() && depDateTime < now;
        if (m_showHistory) {
            if (isDeparted) filteredTrips.append(trip);
        } else {
            if (!isDeparted) filteredTrips.append(trip);
        }
    }

    if (m_showHistory) {
        std::sort(filteredTrips.begin(), filteredTrips.end(),
                  [](const TripRecord& a, const TripRecord& b) {
                      return (a.travelDate + a.departureTime) > (b.travelDate + b.departureTime);
                  });
    }

    m_tripTable->setRowCount(filteredTrips.size());

    TicketManager ticketManager(*dbManager);

    for (int i = 0; i < filteredTrips.size(); ++i) {
        const TripRecord& trip = filteredTrips[i];

        double dynamicPrice = 0.0;
        QVector<QVariantMap> results = ticketManager.searchTrips(depName, arrName, trip.travelDate);
        for (const QVariantMap& result : results) {
            if (result.value("tripId").toInt() == trip.tripId) {
                dynamicPrice = result.value("dynamicPrice").toDouble();
                break;
            }
        }

        m_tripTable->setItem(i, 0, new QTableWidgetItem(QString::number(trip.tripId)));
        m_tripTable->setItem(i, 1, new QTableWidgetItem(trip.travelDate));
        m_tripTable->setItem(i, 2, new QTableWidgetItem(trip.departureTime));
        m_tripTable->setItem(i, 3, new QTableWidgetItem(trip.arrivalTime));
        m_tripTable->setItem(i, 4, new QTableWidgetItem(
                                       QString::number(trip.remainingSeats) + "/" + QString::number(trip.totalSeats)
                                       ));
        m_tripTable->setItem(i, 5, new QTableWidgetItem(
                                       trip.basePrice > 0
                                           ? QString::number(trip.basePrice, 'f', 0) + " 元"
                                           : "未设置"
                                       ));
        m_tripTable->setItem(i, 6, new QTableWidgetItem(
                                       dynamicPrice > 0
                                           ? QString::number(dynamicPrice, 'f', 0) + " 元"
                                           : "计算中"
                                       ));

        for (int column = 0; column < m_tripTable->columnCount(); ++column) {
            if (QTableWidgetItem *item = m_tripTable->item(i, column)) {
                item->setTextAlignment(Qt::AlignCenter);
            }
        }
    }

    QString modeText = m_showHistory ? "历史班次" : "未发车班次";
    showMessage(QString("%1 共 %2 条记录").arg(modeText).arg(filteredTrips.size()), true);
}

void TrainManagementDialog::generateTrips()
{
    int trainId = getSelectedTrainId();
    if (trainId == 0) {
        showMessage("请先选择车次", false);
        return;
    }

    auto dbManager = m_manager->databaseManager();
    if (dbManager == nullptr) {
        showMessage("数据库管理器未初始化", false);
        return;
    }

    const auto train = dbManager->findTrainById(trainId);
    if (!train.has_value()) {
        showMessage("未找到车次", false);
        return;
    }

    bool ok;

    int days = QInputDialog::getInt(this, "生成班次",
                                    "添加未来多少天的班次？", 7, 1, 30, 1, &ok);
    if (!ok) return;

    auto existingTrips = dbManager->findTripsByTrain(trainId);
    QDate startDate = QDate::currentDate();

    if (!existingTrips.isEmpty()) {
        QDate maxDate;
        for (const auto& trip : existingTrips) {
            QDate tripDate = QDate::fromString(trip.travelDate, "yyyy-MM-dd");
            if (tripDate.isValid() && (!maxDate.isValid() || tripDate > maxDate)) {
                maxDate = tripDate;
            }
        }
        if (maxDate.isValid()) {
            startDate = maxDate.addDays(1);
        }
    }

    double existingPrice = 0.0;
    for (const auto& trip : existingTrips) {
        if (trip.basePrice > 0) {
            existingPrice = trip.basePrice;
            break;
        }
    }

    double defaultPrice = 0.0;

    if (existingPrice > 0) {
        QMessageBox msgBox(this);
        msgBox.setWindowTitle("生成班次");
        msgBox.setText(QString("已有班次的基础票价为 ¥%1\n\n是否继承该票价？")
                           .arg(existingPrice, 0, 'f', 0));

        QPushButton *inheritBtn = msgBox.addButton("继承已有票价", QMessageBox::AcceptRole);
        QPushButton *manualBtn = msgBox.addButton("手动修改", QMessageBox::RejectRole);
        QPushButton *cancelBtn = msgBox.addButton("取消", QMessageBox::RejectRole);

        msgBox.exec();

        if (msgBox.clickedButton() == cancelBtn) {
            return;
        }

        if (msgBox.clickedButton() == inheritBtn) {
            defaultPrice = existingPrice;
        } else {
            double inputPrice = QInputDialog::getDouble(
                this, "生成班次",
                "请输入基础票价（元）:",
                existingPrice, 0, 9999, 1, &ok
                );
            if (!ok) return;
            defaultPrice = inputPrice;
        }
    } else {
        double inputPrice = QInputDialog::getDouble(
            this, "生成班次",
            "该车次暂无班次，请输入基础票价（元）:",
            100, 0, 9999, 1, &ok
            );
        if (!ok) return;
        defaultPrice = inputPrice;
    }

    int created = 0;
    for (int i = 0; i < days; ++i) {
        QDate date = startDate.addDays(i);
        QString dateStr = date.toString("yyyy-MM-dd");

        auto existing = dbManager->findOrCreateTrip(trainId, dateStr, train->totalSeats);
        if (existing.has_value()) {
            TripRecord toUpdate = existing.value();
            toUpdate.basePrice = defaultPrice;
            if (dbManager->updateTrip(toUpdate)) {
                created++;
            }
        }
    }

    showMessage(QString("成功添加 %1 个班次，基础票价 ¥%2")
                    .arg(created)
                    .arg(defaultPrice, 0, 'f', 0), true);
    loadTripsForTrain(trainId);
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

void TrainManagementDialog::editTrip()
{
    int row = m_tripTable->currentRow();
    if (row < 0) {
        showMessage("请先选择一个班次", false);
        return;
    }

    int tripId = m_tripTable->item(row, 0)->text().toInt();
    auto dbManager = m_manager->databaseManager();
    if (dbManager == nullptr) {
        showMessage("数据库管理器未初始化", false);
        return;
    }

    auto tripOpt = dbManager->findTripById(tripId);
    if (!tripOpt.has_value()) {
        showMessage("未找到该班次", false);
        return;
    }

    TripRecord trip = tripOpt.value();

    QDialog dialog(this);
    dialog.setWindowTitle("编辑班次 - " + trip.travelDate);
    dialog.resize(400, 350);
    styleTrainEditDialog(dialog);

    QFormLayout *form = new QFormLayout(&dialog);

    QLineEdit *travelDateEdit = new QLineEdit(trip.travelDate);
    travelDateEdit->setPlaceholderText("如 2026-07-16");
    QLineEdit *departTimeEdit = new QLineEdit(trip.departureTime);
    departTimeEdit->setPlaceholderText("如 08:00");
    QLineEdit *arriveTimeEdit = new QLineEdit(trip.arrivalTime);
    arriveTimeEdit->setPlaceholderText("如 12:38");
    QSpinBox *totalSeatsSpin = new QSpinBox();
    totalSeatsSpin->setRange(1, 99999);
    totalSeatsSpin->setValue(trip.totalSeats);
    QSpinBox *remainingSeatsSpin = new QSpinBox();
    remainingSeatsSpin->setRange(0, 99999);
    remainingSeatsSpin->setValue(trip.remainingSeats);
    QDoubleSpinBox *priceSpin = new QDoubleSpinBox();
    priceSpin->setRange(0, 9999);
    priceSpin->setValue(trip.basePrice);
    priceSpin->setPrefix("¥");
    priceSpin->setDecimals(0);

    form->addRow("出行日期:", travelDateEdit);
    form->addRow("出发时间:", departTimeEdit);
    form->addRow("到达时间:", arriveTimeEdit);
    form->addRow("总座位数:", totalSeatsSpin);
    form->addRow("剩余座位:", remainingSeatsSpin);
    form->addRow("票价:", priceSpin);

    QHBoxLayout *btnLayout = new QHBoxLayout();
    QPushButton *okBtn = new QPushButton("确定");
    QPushButton *cancelBtn = new QPushButton("取消");
    btnLayout->addStretch();
    btnLayout->addWidget(okBtn);
    btnLayout->addWidget(cancelBtn);
    form->addRow(btnLayout);

    connect(okBtn, &QPushButton::clicked, &dialog, &QDialog::accept);
    connect(cancelBtn, &QPushButton::clicked, &dialog, &QDialog::reject);

    if (dialog.exec() == QDialog::Accepted) {
        QString travelDate = travelDateEdit->text().trimmed();
        QString depTime = departTimeEdit->text().trimmed();
        QString arrTime = arriveTimeEdit->text().trimmed();
        int totalSeats = totalSeatsSpin->value();
        int remainingSeats = remainingSeatsSpin->value();
        double price = priceSpin->value();

        QRegularExpression dateRegex(R"(^\d{4}-\d{2}-\d{2}$)");
        if (!dateRegex.match(travelDate).hasMatch()) {
            showMessage("日期格式无效，请使用 yyyy-MM-dd", false);
            return;
        }

        QRegularExpression timeRegex(R"(^([0-1][0-9]|2[0-3]):[0-5][0-9]$)");
        if (!timeRegex.match(depTime).hasMatch()) {
            showMessage("出发时间格式无效，请使用 HH:mm", false);
            return;
        }
        if (!timeRegex.match(arrTime).hasMatch()) {
            showMessage("到达时间格式无效，请使用 HH:mm", false);
            return;
        }

        if (remainingSeats < 0 || remainingSeats > totalSeats) {
            showMessage("剩余座位数必须在 0 到总座位数之间", false);
            return;
        }

        trip.travelDate = travelDate;
        trip.departureTime = depTime;
        trip.arrivalTime = arrTime;
        trip.totalSeats = totalSeats;
        trip.remainingSeats = remainingSeats;
        trip.basePrice = price;

        if (!dbManager->updateTrip(trip)) {
            showMessage("更新班次失败: " + dbManager->lastError(), false);
            return;
        }

        showMessage("班次更新成功", true);
        loadTripsForTrain(getSelectedTrainId());
    }
}

void TrainManagementDialog::disableTrip()
{
    int row = m_tripTable->currentRow();
    if (row < 0) {
        showMessage("请先选择一个班次", false);
        return;
    }

    int tripId = m_tripTable->item(row, 0)->text().toInt();
    auto dbManager = m_manager->databaseManager();
    if (dbManager == nullptr) {
        showMessage("数据库管理器未初始化", false);
        return;
    }

    auto tripOpt = dbManager->findTripById(tripId);
    if (!tripOpt.has_value()) {
        showMessage("未找到该班次", false);
        return;
    }

    TripRecord trip = tripOpt.value();
    bool currentlyEnabled = trip.enabled;

    QString action = currentlyEnabled ? "停运" : "恢复运营";
    QString confirmMsg = currentlyEnabled
                             ? QString("确定要停运 %1 的班次吗？").arg(trip.travelDate)
                             : QString("确定要恢复 %1 的班次吗？").arg(trip.travelDate);

    QMessageBox::StandardButton reply = QMessageBox::question(
        this, "确认" + action,
        confirmMsg,
        QMessageBox::Yes | QMessageBox::No
        );

    if (reply != QMessageBox::Yes) return;

    trip.enabled = !currentlyEnabled;
    if (!dbManager->updateTrip(trip)) {
        showMessage(action + "失败: " + dbManager->lastError(), false);
        return;
    }

    showMessage(action + "成功", true);
    loadTripsForTrain(getSelectedTrainId());
}

void TrainManagementDialog::selectTrainByNumber(const QString& trainNumber)
{
    for (int row = 0; row < m_table->rowCount(); ++row) {
        QTableWidgetItem* item = m_table->item(row, 1);
        if (item && item->text() == trainNumber) {
            m_table->selectRow(row);
            m_tabWidget->setCurrentIndex(1);
            int trainId = m_table->item(row, 0)->text().toInt();
            loadTripsForTrain(trainId);
            return;
        }
    }
}

void TrainManagementDialog::onTrainTableDoubleClicked(const QModelIndex &index)
{
    if (!index.isValid()) return;
    int row = index.row();
    if (row < 0 || row >= m_table->rowCount()) return;

    // 获取当前选中行的车次ID（调用现有函数）
    int trainId = getSelectedTrainId();
    if (trainId <= 0) return;

    // 切换到“班次”标签页（索引1）
    m_tabWidget->setCurrentIndex(1);

    // 确保班次数据已加载（如果单击时已经加载，可省略，但为保险保留）
    loadTripsForTrain(trainId);
}

void TrainManagementDialog::goBackToTrainList()
{
    // 切换到车次信息 Tab（索引0）
    m_tabWidget->setCurrentIndex(0);

    // 清除车次表格的选中状态
    m_table->clearSelection();
    m_table->setCurrentIndex(QModelIndex());

    // 清空班次表格
    m_tripTable->setRowCount(0);

    // 重置右上角车次信息标签
    if (m_trainInfoLabel) {
        m_trainInfoLabel->setText("请选择车次");
    }

    // 如果当前处于历史模式，则重置为普通模式
    if (m_showHistory) {
        m_showHistory = false;
        if (m_historyToggleBtn) {
            m_historyToggleBtn->setText("📅 查看历史班次");
        }
    }

    showMessage("已返回车次列表", true);
}
