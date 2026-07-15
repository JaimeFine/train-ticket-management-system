#include "transfer_dialog.h"
#include "app_style.h"

#include <QComboBox>
#include <QGridLayout>
#include <QHeaderView>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QSizePolicy>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QVBoxLayout>

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
    setStyleSheet(UiStyle::dialogStyleSheet());
    setupUI();
    loadStations();
    setWindowTitle(QStringLiteral("路线换乘查询"));
    resize(920, 640);
    setMinimumSize(820, 580);
}

void TransferDialog::setupUI()
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(26, 24, 26, 22);
    mainLayout->setSpacing(10);

    QLabel *titleLabel = new QLabel(QStringLiteral("路线换乘推荐"));
    titleLabel->setObjectName(QStringLiteral("titleLabel"));
    mainLayout->addWidget(titleLabel);

    QLabel *hintLabel = new QLabel(QStringLiteral("选择出发站和到达站，系统将自动推荐合适的换乘路线。"));
    hintLabel->setObjectName(QStringLiteral("hintLabel"));
    mainLayout->addWidget(hintLabel);

    mainLayout->addSpacing(12);

    QHBoxLayout *conditionLayout = new QHBoxLayout();
    QLabel *conditionLabel = new QLabel(QStringLiteral("优化目标："));

    m_conditionCombo = new QComboBox();
    m_conditionCombo->addItem(QStringLiteral("时间最短"),
                              static_cast<int>(OptimizationCriterion::TimeFirst));
    m_conditionCombo->addItem(QStringLiteral("换乘最少"),
                              static_cast<int>(OptimizationCriterion::TransferFirst));
    m_conditionCombo->addItem(QStringLiteral("综合平衡"),
                              static_cast<int>(OptimizationCriterion::Balanced));
    m_conditionCombo->setCurrentIndex(0);
    m_conditionCombo->setMinimumWidth(190);

    conditionLayout->addWidget(conditionLabel);
    conditionLayout->addWidget(m_conditionCombo);
    conditionLayout->addStretch();
    mainLayout->addLayout(conditionLayout);

    mainLayout->addSpacing(8);

    QGridLayout *stationLayout = new QGridLayout();
    stationLayout->setHorizontalSpacing(18);
    stationLayout->setVerticalSpacing(8);
    stationLayout->setColumnStretch(0, 1);
    stationLayout->setColumnStretch(1, 0);
    stationLayout->setColumnStretch(2, 1);

    QLabel *fromLabel = new QLabel(QStringLiteral("出发站"));

    m_fromCombo = new QComboBox();
    m_fromCombo->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    m_fromCombo->setMinimumHeight(46);
    // 中间按钮只负责交换站点，尺寸与两侧输入框保持一致。
    m_swapBtn = new QPushButton(QStringLiteral("⇄"));
    m_swapBtn->setObjectName(QStringLiteral("secondaryButton"));
    m_swapBtn->setFixedSize(58, 54);

    QLabel *toLabel = new QLabel(QStringLiteral("到达站"));

    m_toCombo = new QComboBox();
    m_toCombo->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    m_toCombo->setMinimumHeight(46);
    stationLayout->addWidget(fromLabel, 0, 0);
    stationLayout->addWidget(toLabel, 0, 2);
    stationLayout->addWidget(m_fromCombo, 1, 0);
    stationLayout->addWidget(m_swapBtn, 1, 1, Qt::AlignCenter);
    stationLayout->addWidget(m_toCombo, 1, 2);

    mainLayout->addLayout(stationLayout);

    mainLayout->addSpacing(10);

    m_searchBtn = new QPushButton(QStringLiteral("查询路线"));
    m_searchBtn->setMinimumWidth(150);
    mainLayout->addWidget(m_searchBtn, 0, Qt::AlignCenter);

    mainLayout->addSpacing(10);

    m_summaryLabel = new QLabel(QStringLiteral("请选择站点后点击查询"));
    m_summaryLabel->setObjectName(QStringLiteral("hintLabel"));
    m_summaryLabel->setAlignment(Qt::AlignCenter);
    mainLayout->addWidget(m_summaryLabel);

    m_resultTable = new QTableWidget();
    m_resultTable->setColumnCount(5);
    m_resultTable->setHorizontalHeaderLabels({"步骤", "车次", "出发站", "到达站", "耗时"});
    UiStyle::prepareTable(m_resultTable);
    m_resultTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    mainLayout->addWidget(m_resultTable);

    m_statusLabel = new QLabel(QStringLiteral("就绪"));
    m_statusLabel->setStyleSheet("color: #28a745; font-size: 13px;");
    mainLayout->addWidget(m_statusLabel);

    connect(m_searchBtn, &QPushButton::clicked, this, &TransferDialog::onSearchClicked);
    connect(m_swapBtn, &QPushButton::clicked, this, &TransferDialog::swapStations);
}

void TransferDialog::loadStations()
{
    m_stationNameMap.clear();
    m_fromCombo->clear();
    m_toCombo->clear();

    if (m_routeManager == nullptr || !m_routeManager->isGraphReady()) {
        showError("路线数据尚未准备好");
        return;
    }

    const QList<int> stationIds = m_routeManager->stationIds();
    for (int id : stationIds) {
        const QString name = m_routeManager->stationName(id);
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
