#include "transfer_dialog.h"
#include "database_manager.h"
#include "database/station_record.h"

#include <QComboBox>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QPushButton>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QVBoxLayout>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>

namespace {
QString formatMinutes(int minutes)
{
    if (minutes <= 0) {
        return QStringLiteral("-");
    }

    const int hours = minutes / 60;
    const int remainMinutes = minutes % 60;
    if (hours == 0) {
        return QStringLiteral("%1分钟").arg(remainMinutes);
    }
    if (remainMinutes == 0) {
        return QStringLiteral("%1小时").arg(hours);
    }
    return QStringLiteral("%1小时%2分钟").arg(hours).arg(remainMinutes);
}
}

TransferDialog::TransferDialog(RouteManager* routeManager, QWidget *parent)
    : QDialog(parent)
    , m_routeManager(routeManager)
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
    setWindowTitle("路线换乘查询");
    resize(750, 520);
}

void TransferDialog::setupUI()
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);

    // -------- 标题 --------
    QLabel *titleLabel = new QLabel("🚆 路线换乘推荐");
    titleLabel->setStyleSheet("font-size: 20px; font-weight: bold; color: #17231f;");
    mainLayout->addWidget(titleLabel);

    QLabel *hintLabel = new QLabel("选择出发站和到达站，系统将自动推荐最优换乘路线");
    hintLabel->setStyleSheet("color: #65716c; font-size: 13px;");
    mainLayout->addWidget(hintLabel);

    mainLayout->addSpacing(12);

    // -------- 优化条件选择 --------
    QHBoxLayout *conditionLayout = new QHBoxLayout();
    QLabel *conditionLabel = new QLabel("优化目标：");
    conditionLabel->setStyleSheet("color: #33433d; font-weight: 600; font-size: 13px;");

    m_conditionCombo = new QComboBox();
    m_conditionCombo->addItem("⏱ 时间最短", static_cast<int>(OptimizationCriterion::TimeFirst));
    m_conditionCombo->addItem("🔄 换乘最少", static_cast<int>(OptimizationCriterion::TransferFirst));
    m_conditionCombo->addItem("⚖️ 综合平衡", static_cast<int>(OptimizationCriterion::Balanced));
    m_conditionCombo->setCurrentIndex(0);
    m_conditionCombo->setStyleSheet(
        "QComboBox {"
        "    color: #1f2933;"
        "    background-color: #ffffff;"
        "    border: 1px solid #cbd8d2;"
        "    border-radius: 6px;"
        "    padding: 6px 10px;"
        "    min-height: 32px;"
        "    min-width: 150px;"
        "}"
        "QComboBox:hover { border: 1px solid #0f766e; }"
        "QComboBox::drop-down { border: none; }"
        "QComboBox QAbstractItemView {"
        "    color: #1f2933;"
        "    background-color: #ffffff;"
        "    border: 1px solid #cbd8d2;"
        "    border-radius: 6px;"
        "    selection-background-color: #d9f99d;"
        "    selection-color: #153832;"
        "}"
        );

    conditionLayout->addWidget(conditionLabel);
    conditionLayout->addWidget(m_conditionCombo);
    conditionLayout->addStretch();
    mainLayout->addLayout(conditionLayout);

    mainLayout->addSpacing(8);

    // -------- 站点选择区域 --------
    QHBoxLayout *stationLayout = new QHBoxLayout();

    // 出发站
    QVBoxLayout *fromLayout = new QVBoxLayout();
    QLabel *fromLabel = new QLabel("出发站");
    fromLabel->setStyleSheet("color: #33433d; font-weight: 600; font-size: 13px;");

    m_fromCombo = new QComboBox();
    m_fromCombo->setStyleSheet(
        "QComboBox {"
        "    color: #1f2933;"
        "    background-color: #ffffff;"
        "    border: 1px solid #cbd8d2;"
        "    border-radius: 6px;"
        "    padding: 6px 10px;"
        "    min-height: 32px;"
        "    min-width: 180px;"
        "}"
        "QComboBox:hover { border: 1px solid #0f766e; }"
        "QComboBox::drop-down { border: none; }"
        "QComboBox QAbstractItemView {"
        "    color: #1f2933;"
        "    background-color: #ffffff;"
        "    border: 1px solid #cbd8d2;"
        "    border-radius: 6px;"
        "    selection-background-color: #d9f99d;"
        "    selection-color: #153832;"
        "}"
        );
    fromLayout->addWidget(fromLabel);
    fromLayout->addWidget(m_fromCombo);

    // 交换按钮
    QVBoxLayout *swapLayout = new QVBoxLayout();
    swapLayout->addStretch();
    m_swapBtn = new QPushButton("⇄");
    m_swapBtn->setStyleSheet(
        "QPushButton {"
        "    background-color: #eef5f1;"
        "    color: #33433d;"
        "    border: 1px solid #cbd8d2;"
        "    border-radius: 8px;"
        "    font-size: 20px;"
        "    padding: 8px 12px;"
        "    min-width: 40px;"
        "    min-height: 40px;"
        "}"
        "QPushButton:hover { background-color: #d8e0dc; }"
        );
    swapLayout->addWidget(m_swapBtn);
    swapLayout->addStretch();

    // 到达站
    QVBoxLayout *toLayout = new QVBoxLayout();
    QLabel *toLabel = new QLabel("到达站");
    toLabel->setStyleSheet("color: #33433d; font-weight: 600; font-size: 13px;");

    m_toCombo = new QComboBox();
    m_toCombo->setStyleSheet(
        "QComboBox {"
        "    color: #1f2933;"
        "    background-color: #ffffff;"
        "    border: 1px solid #cbd8d2;"
        "    border-radius: 6px;"
        "    padding: 6px 10px;"
        "    min-height: 32px;"
        "    min-width: 180px;"
        "}"
        "QComboBox:hover { border: 1px solid #0f766e; }"
        "QComboBox::drop-down { border: none; }"
        "QComboBox QAbstractItemView {"
        "    color: #1f2933;"
        "    background-color: #ffffff;"
        "    border: 1px solid #cbd8d2;"
        "    border-radius: 6px;"
        "    selection-background-color: #d9f99d;"
        "    selection-color: #153832;"
        "}"
        );
    toLayout->addWidget(toLabel);
    toLayout->addWidget(m_toCombo);

    stationLayout->addLayout(fromLayout);
    stationLayout->addSpacing(10);
    stationLayout->addLayout(swapLayout);
    stationLayout->addSpacing(10);
    stationLayout->addLayout(toLayout);
    stationLayout->addStretch();

    mainLayout->addLayout(stationLayout);

    mainLayout->addSpacing(10);

    // -------- 查询按钮 --------
    m_searchBtn = new QPushButton("🔍 查询路线");
    m_searchBtn->setStyleSheet(
        "QPushButton {"
        "    background-color: #176b5b;"
        "    color: white;"
        "    border: none;"
        "    border-radius: 8px;"
        "    padding: 10px 20px;"
        "    font-size: 15px;"
        "    font-weight: bold;"
        "    min-height: 36px;"
        "    min-width: 140px;"
        "}"
        "QPushButton:hover { background-color: #0f5749; }"
        );
    mainLayout->addWidget(m_searchBtn, 0, Qt::AlignCenter);

    mainLayout->addSpacing(10);

    // -------- 结果摘要 --------
    m_summaryLabel = new QLabel("请选择站点后点击查询");
    m_summaryLabel->setStyleSheet("color: #65716c; font-size: 14px;");
    m_summaryLabel->setAlignment(Qt::AlignCenter);
    mainLayout->addWidget(m_summaryLabel);

    // -------- 结果表格 --------
    m_resultTable = new QTableWidget();
    m_resultTable->setColumnCount(5);
    m_resultTable->setHorizontalHeaderLabels({"步骤", "车次", "出发站", "到达站", "耗时"});
    m_resultTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    m_resultTable->verticalHeader()->setVisible(false);
    m_resultTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_resultTable->setAlternatingRowColors(true);
    m_resultTable->setStyleSheet(
        "QTableWidget {"
        "    background-color: #ffffff;"
        "    color: #1f2933;"
        "    border: 1px solid #d8e0dc;"
        "    border-radius: 8px;"
        "    gridline-color: #e5ece8;"
        "    selection-background-color: #d9f99d;"
        "    selection-color: #153832;"
        "}"
        "QHeaderView::section {"
        "    background-color: #eef5f1;"
        "    color: #33433d;"
        "    padding: 6px;"
        "    border: none;"
        "    font-weight: bold;"
        "    font-size: 13px;"
        "}"
        "QTableWidget::item { padding: 4px; }"
        );
    mainLayout->addWidget(m_resultTable);

    // -------- 状态栏 --------
    m_statusLabel = new QLabel("就绪");
    m_statusLabel->setStyleSheet("color: #28a745; font-size: 13px;");
    mainLayout->addWidget(m_statusLabel);

    // -------- 信号连接 --------
    connect(m_searchBtn, &QPushButton::clicked, this, &TransferDialog::onSearchClicked);
    connect(m_swapBtn, &QPushButton::clicked, this, &TransferDialog::swapStations);
}

void TransferDialog::loadStations()
{
    m_stationNameMap.clear();
    m_fromCombo->clear();
    m_toCombo->clear();

    QSqlDatabase db = QSqlDatabase::database("train_ticket_connection");
    if (!db.isOpen()) {
        showError("数据库未连接");
        return;
    }

    QSqlQuery query(db);
    if (!query.exec("SELECT stationId, stationName FROM Station ORDER BY stationName")) {
        showError("加载站点失败: " + query.lastError().text());
        return;
    }

    while (query.next()) {
        int id = query.value("stationId").toInt();
        QString name = query.value("stationName").toString();
        m_fromCombo->addItem(name, id);
        m_toCombo->addItem(name, id);
        m_stationNameMap[id] = name;
    }

    // 默认选中第一个和第二个站点
    if (m_fromCombo->count() > 0) {
        m_fromCombo->setCurrentIndex(0);
    }
    if (m_toCombo->count() > 1) {
        m_toCombo->setCurrentIndex(1);
    } else if (m_toCombo->count() > 0) {
        m_toCombo->setCurrentIndex(0);
    }
}

void TransferDialog::onSearchClicked()
{
    int fromId = m_fromCombo->currentData().toInt();
    int toId = m_toCombo->currentData().toInt();

    if (fromId == toId) {
        showError("出发站和到达站不能相同");
        return;
    }

    if (m_routeManager == nullptr) {
        showError("路线服务未初始化");
        return;
    }

    if (!m_routeManager->isGraphReady()) {
        if (!m_routeManager->buildGraph()) {
            showError("构建路线图失败: " + m_routeManager->lastError());
            return;
        }
        if (!m_routeManager->isGraphReady()) {
            showError("无法构建路线图，请检查数据库是否有车次数据");
            return;
        }
    }

    clearResult();

    int conditionIndex = m_conditionCombo->currentData().toInt();
    OptimizationCriterion criterion = static_cast<OptimizationCriterion>(conditionIndex);

    RoutePath path = m_routeManager->recommendTransfer(fromId, toId, criterion);

    if (path.stationIds.isEmpty()) {
        QString errorMsg = m_routeManager->lastError();
        if (errorMsg.isEmpty()) {
            errorMsg = "未找到从 " + m_stationNameMap.value(fromId, QString::number(fromId)) +
                       " 到 " + m_stationNameMap.value(toId, QString::number(toId)) + " 的路线";
        }
        showError(errorMsg);
        return;
    }

    showPathResult(path);
}

void TransferDialog::swapStations()
{
    int fromIndex = m_fromCombo->currentIndex();
    int toIndex = m_toCombo->currentIndex();
    m_fromCombo->setCurrentIndex(toIndex);
    m_toCombo->setCurrentIndex(fromIndex);
    clearResult();
    m_summaryLabel->setText("出发站和到达站已交换，请点击查询");
}

void TransferDialog::showPathResult(const RoutePath& path)
{
    // 摘要信息
    m_summaryLabel->setText(QString("✅ 找到路线！总耗时：%1，共 %2 段行程")
                                .arg(path.timeString())
                                .arg(path.trainIds.size()));

    // 填充表格
    int rowCount = path.trainIds.size();
    m_resultTable->setRowCount(rowCount);

    for (int i = 0; i < rowCount; ++i) {
        int fromStation = path.stationIds[i];
        int toStation = path.stationIds[i + 1];

        m_resultTable->setItem(i, 0, new QTableWidgetItem(QString::number(i + 1)));
        m_resultTable->setItem(i, 1, new QTableWidgetItem(
                                         path.trainNumbers.value(i, QString::number(path.trainIds.value(i)))
                                         ));
        m_resultTable->setItem(i, 2, new QTableWidgetItem(
                                         m_stationNameMap.value(fromStation, QString::number(fromStation))
                                         ));
        m_resultTable->setItem(i, 3, new QTableWidgetItem(
                                         m_stationNameMap.value(toStation, QString::number(toStation))
                                         ));
        m_resultTable->setItem(i, 4, new QTableWidgetItem(
                                         formatMinutes(path.segmentTimes.value(i))
                                         ));
    }

    m_statusLabel->setText("✅ 查询成功");
    m_statusLabel->setStyleSheet("color: #28a745; font-size: 13px;");
}

void TransferDialog::showError(const QString& msg)
{
    m_summaryLabel->setText("❌ " + msg);
    m_resultTable->setRowCount(0);
    m_statusLabel->setText("❌ " + msg);
    m_statusLabel->setStyleSheet("color: #dc3545; font-size: 13px;");
}

void TransferDialog::clearResult()
{
    m_resultTable->setRowCount(0);
    m_statusLabel->setText("就绪");
    m_statusLabel->setStyleSheet("color: #28a745; font-size: 13px;");
}
