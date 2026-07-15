#include "train_management_dialog.h"

#include "database_manager.h"
#include "train_management_dialog_support.h"

#include <QComboBox>
#include <QDialog>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QInputDialog>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QRegularExpression>
#include <QSpinBox>
#include <QTime>

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
