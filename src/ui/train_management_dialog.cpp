#include "train_management_dialog.h"
#include "station_management_dialog.h"

#include <QComboBox>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QInputDialog>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QSpinBox>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QVBoxLayout>
#include <QSqlDatabase>
#include <QSqlQuery>

TrainManagementDialog::TrainManagementDialog(TrainManager* manager, QWidget *parent)
    : QDialog(parent)
    , m_manager(manager)
{
    setStyleSheet(
        "QDialog {"
        "    background: #eef2f3;"
        "    color: #1f2933;"
        "    font-family: \"Microsoft YaHei UI\", \"Microsoft YaHei\", \"Segoe UI\";"
        "    font-size: 14px;"
        "}"
        );
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
    m_searchInput->setStyleSheet(
        "QLineEdit {"
        "    color: #1f2933;"
        "    background-color: #ffffff;"
        "    border: 1px solid #cbd8d2;"
        "    border-radius: 6px;"
        "    padding: 4px 8px;"
        "    font-size: 13px;"
        "}"
        "QLineEdit:focus {"
        "    border: 2px solid #0f766e;"
        "}"
        "QLineEdit::placeholder {"
        "    color: #8b9490;"
        "}"
        );
    m_searchBtn = new QPushButton("搜索");
    m_refreshBtn = new QPushButton("刷新");

    m_stationCombo = new QComboBox();
    m_stationCombo->addItem("-- 选择出发站 --");
    m_stationCombo->setStyleSheet(
        "QComboBox {"
        "    color: #1f2933;"
        "    background-color: #ffffff;"
        "    border: 1px solid #cbd8d2;"
        "    border-radius: 6px;"
        "    padding: 4px 8px;"
        "    min-height: 26px;"
        "}"
        "QComboBox:hover {"
        "    border: 1px solid #0f766e;"
        "}"
        "QComboBox::drop-down {"
        "    border: none;"
        "}"
        "QComboBox QAbstractItemView {"
        "    color: #1f2933;"
        "    background-color: #ffffff;"
        "    border: 1px solid #cbd8d2;"
        "    border-radius: 6px;"
        "    selection-background-color: #d9f99d;"
        "    selection-color: #153832;"
        "    outline: none;"
        "}"
        "QComboBox QAbstractItemView::item {"
        "    padding: 6px 10px;"
        "    min-height: 26px;"
        "}"
        "QComboBox QAbstractItemView::item:hover {"
        "    background-color: #f0f5f3;"
        "}"
        );
    m_searchStationBtn = new QPushButton("按出发站查询");
    m_searchStationBtn->setStyleSheet("background-color: #176b5b; color: white; border-radius: 6px; padding: 6px 12px;");
    // 站点管理按钮 - 正确文本和功能
    QPushButton *stationBtn = new QPushButton("站点管理");
    stationBtn->setStyleSheet("background-color: #17a2b8; color: white; border-radius: 6px; padding: 6px 12px;");
    connect(stationBtn, &QPushButton::clicked, [this]() {
        StationManagementDialog dialog(this);
        dialog.exec();
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
    m_table->setColumnCount(7);
    m_table->setHorizontalHeaderLabels({
        "ID", "车次号", "出发站", "到达站",
        "出发时间", "到达时间", "余票/总座"
    });
    m_table->verticalHeader()->setVisible(false);

    m_table->setStyleSheet(
        "QTableWidget {"
        "    background-color: #ffffff;"
        "    alternate-background-color: #f5f8f7;"
        "    color: #1f2933;"
        "    border: 1px solid #d8e0dc;"
        "    border-radius: 8px;"
        "    gridline-color: #e5ece8;"
        "}"
        "QHeaderView::section {"
        "    background-color: #eef5f1;"
        "    color: #33433d;"
        "    padding: 6px;"
        "    border: none;"
        "    font-weight: bold;"
        "    font-size: 13px;"
        "}"
        "QTableWidget::item {"
        "    color: #1f2933;"
        "    padding: 4px;"
        "}"
        );
    m_table->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->setSelectionMode(QAbstractItemView::SingleSelection);
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_table->setAlternatingRowColors(true);
    mainLayout->addWidget(m_table);

    // -------- 操作按钮 --------
    QHBoxLayout *actionLayout = new QHBoxLayout();

    m_addBtn = new QPushButton("添加车次");
    m_addBtn->setStyleSheet("background-color: #28a745; color: white; font-weight: bold;");

    m_editBtn = new QPushButton("编辑");
    m_editBtn->setEnabled(false);

    m_deleteBtn = new QPushButton("停运");
    m_deleteBtn->setEnabled(false);
    m_deleteBtn->setStyleSheet("background-color: #dc3545; color: white;");

    m_seatBtn = new QPushButton("座位管理");
    m_seatBtn->setEnabled(false);

    m_countLabel = new QLabel("共 0 条记录");
    m_countLabel->setStyleSheet("color: #65716c; font-size: 13px;");

    m_messageLabel = new QLabel("就绪");
    m_messageLabel->setStyleSheet("color: #28a745; font-size: 13px;");

    actionLayout->addWidget(m_addBtn);
    actionLayout->addWidget(m_editBtn);
    actionLayout->addWidget(m_deleteBtn);
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

    m_stationNameMap.clear();

    QSqlDatabase db = QSqlDatabase::database("train_ticket_connection");
    if (!db.isOpen()) return;

    QSqlQuery query(db);
    query.exec("SELECT stationId, stationName FROM Station ORDER BY stationName");
    while (query.next()) {
        int id = query.value("stationId").toInt();
        QString name = query.value("stationName").toString();
        m_stationCombo->addItem(name, id);
        m_stationNameMap[id] = name;
    }
    int count = 0;
    while (query.next()) {
        // ...
        count++;
    }
     qDebug() << "加载了" << count << "个站点";

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
                                   QString::number(t.remainingSeats) + "/" + QString::number(t.totalSeats)
                                   ));
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
        m_table->setItem(i, 2, new QTableWidgetItem(QString::number(t.departureStationId)));
        m_table->setItem(i, 3, new QTableWidgetItem(QString::number(t.arrivalStationId)));
        m_table->setItem(i, 4, new QTableWidgetItem(t.departureTime));
        m_table->setItem(i, 5, new QTableWidgetItem(t.arrivalTime));
        m_table->setItem(i, 6, new QTableWidgetItem(
                                   QString::number(t.remainingSeats) + "/" + QString::number(t.totalSeats)
                                   ));
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
        m_table->setItem(i, 2, new QTableWidgetItem(QString::number(t.departureStationId)));
        m_table->setItem(i, 3, new QTableWidgetItem(QString::number(t.arrivalStationId)));
        m_table->setItem(i, 4, new QTableWidgetItem(t.departureTime));
        m_table->setItem(i, 5, new QTableWidgetItem(t.arrivalTime));
        m_table->setItem(i, 6, new QTableWidgetItem(
                                   QString::number(t.remainingSeats) + "/" + QString::number(t.totalSeats)
                                   ));
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

    QSqlDatabase db = QSqlDatabase::database("train_ticket_connection");
    if (db.isOpen()) {
        QSqlQuery query(db);
        query.exec("SELECT stationId, stationName FROM Station ORDER BY stationName");
        while (query.next()) {
            QString name = query.value("stationName").toString();
            int id = query.value("stationId").toInt();
            departCombo->addItem(name, id);
            arriveCombo->addItem(name, id);
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
        train.remainingSeats = train.totalSeats;
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

    QFormLayout *form = new QFormLayout(&dialog);

    QLineEdit *trainNumberEdit = new QLineEdit(current.trainNumber);
    QComboBox *departCombo = new QComboBox();
    QComboBox *arriveCombo = new QComboBox();
    QLineEdit *departTimeEdit = new QLineEdit(current.departureTime);
    QLineEdit *arriveTimeEdit = new QLineEdit(current.arrivalTime);
    QSpinBox *totalSeatsSpin = new QSpinBox();
    totalSeatsSpin->setRange(1, 999);
    totalSeatsSpin->setValue(current.totalSeats);
    QSpinBox *remainingSeatsSpin = new QSpinBox();
    remainingSeatsSpin->setRange(0, 999);
    remainingSeatsSpin->setValue(current.remainingSeats);

    QSqlDatabase db = QSqlDatabase::database("train_ticket_connection");
    if (db.isOpen()) {
        QSqlQuery query(db);
        query.exec("SELECT stationId, stationName FROM Station ORDER BY stationName");
        int departIndex = 0, arriveIndex = 0;
        int idx = 0;
        while (query.next()) {
            QString name = query.value("stationName").toString();
            int id = query.value("stationId").toInt();
            departCombo->addItem(name, id);
            arriveCombo->addItem(name, id);
            if (id == current.departureStationId) departIndex = idx;
            if (id == current.arrivalStationId) arriveIndex = idx;
            idx++;
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
    form->addRow("剩余座位:", remainingSeatsSpin);

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
        train.remainingSeats = remainingSeatsSpin->value();
        train.enabled = current.enabled;

        if (train.trainNumber.isEmpty()) {
            showMessage("车次号不能为空", false);
            return;
        }
        if (train.remainingSeats > train.totalSeats) {
            showMessage("剩余座位不能超过总座位", false);
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

void TrainManagementDialog::updateSeats()
{
    int trainId = getSelectedTrainId();
    if (trainId == 0) return;

    auto all = m_manager->getAllTrains(false);
    int currentSeats = 0;
    QString trainNumber;
    for (const auto& t : all) {
        if (t.trainId == trainId) {
            currentSeats = t.remainingSeats;
            trainNumber = t.trainNumber;
            break;
        }
    }

    bool ok;
    int delta = QInputDialog::getInt(
        this, "座位管理 - " + trainNumber,
        "当前余票: " + QString::number(currentSeats) + "\n\n"
                                                       "输入变动数量（负数=售票，正数=退票）:",
        0, -999, 999, 1, &ok
        );

    if (ok && delta != 0) {
        if (m_manager->updateRemainingSeats(trainId, delta)) {
            showMessage(m_manager->statusMessage(), true);
            loadData();
        } else {
            showMessage(m_manager->statusMessage(), false);
        }
    }
}

void TrainManagementDialog::onTableRowClicked()
{
    int row = m_table->currentRow();
    bool hasSelection = (row >= 0 && row < m_table->rowCount());
    m_editBtn->setEnabled(hasSelection);
    m_deleteBtn->setEnabled(hasSelection);
    m_seatBtn->setEnabled(hasSelection);
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
    m_seatBtn->setEnabled(false);
    m_table->clearSelection();
}

void TrainManagementDialog::showMessage(const QString &msg, bool success)
{
    m_messageLabel->setText(msg);
    m_messageLabel->setStyleSheet(success ? "color: #28a745;" : "color: #dc3545;");
}