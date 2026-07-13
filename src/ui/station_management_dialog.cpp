#include "station_management_dialog.h"
#include "database_manager.h"

#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QHeaderView>
#include <QMessageBox>

StationManagementDialog::StationManagementDialog(
    DatabaseManager *dbManager, QWidget *parent
)
    : QDialog(parent)
    , m_dbManager(dbManager)
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
    loadData();
    setWindowTitle("站点管理");
    resize(500, 400);
}

void StationManagementDialog::setupUI()
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);

    // -------- 添加站点区域 --------
    QHBoxLayout *addLayout = new QHBoxLayout();
    QLabel *label = new QLabel("站点名称:");
    label->setStyleSheet("color: #33433d; font-weight: 600;");

    m_nameInput = new QLineEdit();
    m_nameInput->setPlaceholderText("请输入站点名称...");
    m_nameInput->setStyleSheet(
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

    m_addBtn = new QPushButton("添加");
    m_addBtn->setStyleSheet("background-color: #28a745; color: white;");

    addLayout->addWidget(label);
    addLayout->addWidget(m_nameInput, 1);
    addLayout->addWidget(m_addBtn);
    mainLayout->addLayout(addLayout);

    // -------- 站点列表 --------
    m_table = new QTableWidget();
    m_table->setColumnCount(2);
    m_table->setHorizontalHeaderLabels({"ID", "站点名称"});
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

    m_table->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_table->setAlternatingRowColors(true);
    mainLayout->addWidget(m_table);

    // -------- 操作按钮 --------
    QHBoxLayout *actionLayout = new QHBoxLayout();
    m_refreshBtn = new QPushButton("刷新");
    m_deleteBtn = new QPushButton("删除选中");
    m_deleteBtn->setStyleSheet("background-color: #dc3545; color: white;");
    m_messageLabel = new QLabel("就绪");
    m_messageLabel->setStyleSheet("color: #28a745; font-size: 13px;");

    actionLayout->addWidget(m_refreshBtn);
    actionLayout->addWidget(m_deleteBtn);
    actionLayout->addStretch();
    actionLayout->addWidget(m_messageLabel);
    mainLayout->addLayout(actionLayout);

    // -------- 信号连接 --------
    connect(m_addBtn, &QPushButton::clicked, this, &StationManagementDialog::addStation);
    connect(m_deleteBtn, &QPushButton::clicked, this, &StationManagementDialog::deleteStation);
    connect(m_refreshBtn, &QPushButton::clicked, this, &StationManagementDialog::refreshList);
    connect(m_nameInput, &QLineEdit::returnPressed, this, &StationManagementDialog::addStation);
}

void StationManagementDialog::loadData()
{
    if (m_dbManager == nullptr || !m_dbManager->isOpen()) {
        showMessage("数据库未连接", false);
        return;
    }

    const QList<StationRecord> stations = m_dbManager->getAllStations();

    m_table->setRowCount(0);
    int row = 0;
    for (const StationRecord &station : stations) {
        m_table->insertRow(row);
        m_table->setItem(
            row, 0, new QTableWidgetItem(QString::number(station.stationId))
        );
        m_table->setItem(row, 1, new QTableWidgetItem(station.stationName));
        row++;
    }

    showMessage("共 " + QString::number(row) + " 个站点", true);
}

void StationManagementDialog::refreshList()
{
    loadData();
}

void StationManagementDialog::addStation()
{
    QString name = m_nameInput->text().trimmed();
    if (name.isEmpty()) {
        showMessage("请输入站点名称", false);
        return;
    }

    if (m_dbManager == nullptr || !m_dbManager->isOpen()) {
        showMessage("数据库未连接", false);
        return;
    }

    if (m_dbManager->findStationByName(name).has_value()) {
        showMessage("站点 '" + name + "' 已存在", false);
        return;
    }

    StationRecord station;
    station.stationName = name;
    if (!m_dbManager->addStation(station)) {
        showMessage("添加失败: " + m_dbManager->lastError(), false);
        return;
    }

    m_nameInput->clear();
    showMessage("站点 '" + name + "' 添加成功", true);
    loadData();
}

void StationManagementDialog::deleteStation()
{
    int row = m_table->currentRow();
    if (row < 0) {
        showMessage("请先选中一个站点", false);
        return;
    }

    int stationId = m_table->item(row, 0)->text().toInt();
    QString stationName = m_table->item(row, 1)->text();

    if (m_dbManager == nullptr || !m_dbManager->isOpen()) {
        showMessage("数据库未连接", false);
        return;
    }

    QMessageBox::StandardButton reply = QMessageBox::question(
        this, "确认删除",
        "确定要删除站点 '" + stationName + "' 吗？",
        QMessageBox::Yes | QMessageBox::No
        );

    if (reply != QMessageBox::Yes) return;

    if (!m_dbManager->deleteStation(stationId)) {
        showMessage("删除失败: " + m_dbManager->lastError(), false);
        return;
    }

    showMessage("站点 '" + stationName + "' 已删除", true);
    loadData();
}

void StationManagementDialog::showMessage(const QString &msg, bool success)
{
    m_messageLabel->setText(msg);
    m_messageLabel->setStyleSheet(success ? "color: #28a745;" : "color: #dc3545;");
}
