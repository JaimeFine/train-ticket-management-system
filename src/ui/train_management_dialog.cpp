#include "train_management_dialog.h"
#include "database_manager.h"
#include "station_management_dialog.h"
#include "app_style.h"

#include <QComboBox>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QHeaderView>
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

namespace {
class StationComboBox : public QComboBox
{
protected:
    void paintEvent(QPaintEvent *event) override
    {
        QComboBox::paintEvent(event);

        // 右侧区域和外框一起绘制，获得焦点时边框不会在中间断开。
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
}

TrainManagementDialog::TrainManagementDialog(TrainManager* manager, QWidget *parent)
    : QDialog(parent)
    , m_manager(manager)
{
    setStyleSheet(UiStyle::dialogStyleSheet());
    setupUI();
    loadStations();
    loadData();
    setWindowTitle("车次管理");
    resize(1000, 550);
}

void TrainManagementDialog::setupUI()
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);

    // -------- 搜索栏 --------
    QHBoxLayout *searchLayout = new QHBoxLayout();

    m_searchInput = new QLineEdit();
    m_searchInput->setPlaceholderText("输入车次号或站点名称...");
    m_searchInput->setMinimumWidth(250);
    m_searchBtn = new QPushButton("搜索");
    m_refreshBtn = new QPushButton("刷新");

    m_stationCombo = new StationComboBox();
    m_stationCombo->setObjectName(QStringLiteral("stationSearchCombo"));
    m_stationCombo->addItem("-- 选择出发站 --");
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
        "出发时间", "到达时间", "余票/总座", "状态"
    });
    UiStyle::prepareTable(m_table);
    m_table->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    mainLayout->addWidget(m_table);

    // -------- 操作按钮 --------
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

    m_seatBtn = new QPushButton("Trip座位说明");
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
    actionLayout->addSpacing(20);
    actionLayout->addWidget(m_messageLabel);
    mainLayout->addLayout(actionLayout);

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
        m_table->setItem(i, 6, new QTableWidgetItem(
                                   QString::number(0) + "/" + QString::number(t.totalSeats)
                                   ));
        m_table->setItem(i, 7, new QTableWidgetItem(t.enabled ? "运营中" : "已停运"));
    }

    m_countLabel->setText("共 " + QString::number(trains.size()) + " 条记录");
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
        m_table->setItem(i, 6, new QTableWidgetItem(
                                   QString::number(0) + "/" + QString::number(t.totalSeats)
                                   ));
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
        m_table->setItem(i, 6, new QTableWidgetItem(
                                   QString::number(0) + "/" + QString::number(t.totalSeats)
                                   ));
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
    departTimeEdit->setPlaceholderText("如 2026-07-10 08:00");
    QLineEdit *arriveTimeEdit = new QLineEdit();
    arriveTimeEdit->setPlaceholderText("如 2026-07-10 12:30");
    QSpinBox *totalSeatsSpin = new QSpinBox();
    totalSeatsSpin->setRange(1, 999);
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
        // V2: seats are per Trip now;
        train.enabled = true;

        if (train.trainNumber.isEmpty()) {
            showMessage("车次号不能为空", false);
            return;
        }

        if (m_manager->addTrain(train)) {
            showMessage(m_manager->statusMessage(), true);
            loadData();
        } else {
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
    totalSeatsSpin->setRange(1, 999);
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
        // V2: seats per Trip;
        train.enabled = current.enabled;

        if (train.trainNumber.isEmpty()) {
            showMessage("车次号不能为空", false);
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
    showMessage(QStringLiteral("当前版本的座位余量由 Trip 数据维护。"), false);
}

void TrainManagementDialog::onTableRowClicked()
{
    int row = m_table->currentRow();
    bool hasSelection = (row >= 0 && row < m_table->rowCount());
    m_editBtn->setEnabled(hasSelection);
    m_deleteBtn->setEnabled(hasSelection);
    m_resumeBtn->setEnabled(hasSelection);
    m_purgeBtn->setEnabled(hasSelection);
    m_seatBtn->setEnabled(false);
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
