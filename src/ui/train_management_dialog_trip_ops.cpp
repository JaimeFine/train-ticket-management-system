#include "train_management_dialog.h"

#include "database_manager.h"
#include "ticket_manager.h"
#include "train_management_dialog_support.h"

#include <QDate>
#include <QDateTime>
#include <QDialog>
#include <QDoubleSpinBox>
#include <QFormLayout>
#include <QInputDialog>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QModelIndex>
#include <QPushButton>
#include <QRegularExpression>
#include <QSpinBox>
#include <QTableWidget>
#include <QTableWidgetItem>

#include <algorithm>

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
