#include "statistics_dialog.h"

#include "statistics_manager.h"
#include "app_style.h"

#include <QHeaderView>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QVBoxLayout>

StatisticsDialog::StatisticsDialog(const StatisticsManager &statisticsManager,
                                   QWidget *parent)
    : QDialog(parent)
    , m_statisticsManager(&statisticsManager)
{
    setWindowTitle(QStringLiteral("票务数据统计"));
    setModal(true);
    resize(860, 620);

    setStyleSheet(QStringLiteral(R"QSS(
        QDialog {
            background: #eef2f3;
            color: #1f2933;
            font-family: "Microsoft YaHei UI", "Microsoft YaHei", "Segoe UI";
            font-size: 14px;
        }
        QLabel#titleLabel {
            color: #17231f;
            font-size: 24px;
            font-weight: 700;
        }
        QLabel#hintLabel {
            color: #42514b;
        }
        QFrame#metricCard {
            background: #fbfcfb;
            border: 1px solid #d8e0dc;
            border-radius: 12px;
        }
        QLabel#metricTitle {
            color: #52615b;
            font-size: 13px;
            font-weight: 700;
        }
        QLabel#metricValue {
            color: #153832;
            font-size: 28px;
            font-weight: 700;
        }
        QLabel#sectionTitle {
            color: #17231f;
            font-size: 16px;
            font-weight: 700;
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
    )QSS"));

    auto *rootLayout = new QVBoxLayout(this);
    rootLayout->setContentsMargins(24, 22, 24, 22);
    rootLayout->setSpacing(16);

    auto *titleLabel = new QLabel(QStringLiteral("管理员统计中心"), this);
    titleLabel->setObjectName(QStringLiteral("titleLabel"));

    auto *hintLabel = new QLabel(QStringLiteral("汇总当前订单状态、热门路线和月度客流，方便管理员快速查看运营情况。"),
                                 this);
    hintLabel->setObjectName(QStringLiteral("hintLabel"));
    hintLabel->setWordWrap(true);

    rootLayout->addWidget(titleLabel);
    rootLayout->addWidget(hintLabel);

    auto *metricsLayout = new QHBoxLayout;
    metricsLayout->setSpacing(12);

    auto createMetricCard = [this](const QString &titleText, QLabel *&valueLabel) {
        auto *card = new QFrame(this);
        card->setObjectName(QStringLiteral("metricCard"));

        auto *layout = new QVBoxLayout(card);
        layout->setContentsMargins(18, 14, 18, 14);
        layout->setSpacing(6);

        auto *titleLabel = new QLabel(titleText, card);
        titleLabel->setObjectName(QStringLiteral("metricTitle"));
        titleLabel->setAlignment(Qt::AlignCenter);

        valueLabel = createMetricValueLabel();
        valueLabel->setParent(card);
        valueLabel->setAlignment(Qt::AlignCenter);

        layout->addWidget(titleLabel);
        layout->addWidget(valueLabel);
        return card;
    };

    metricsLayout->addWidget(createMetricCard(QStringLiteral("总订单数"), m_totalOrdersValueLabel));
    metricsLayout->addWidget(createMetricCard(QStringLiteral("已订票"), m_totalBookedValueLabel));
    metricsLayout->addWidget(createMetricCard(QStringLiteral("已退票"), m_totalRefundedValueLabel));
    metricsLayout->addWidget(createMetricCard(QStringLiteral("已改签"), m_totalChangedValueLabel));

    rootLayout->addLayout(metricsLayout);

    auto *popularTitleLabel = new QLabel(QStringLiteral("热门路线"), this);
    popularTitleLabel->setObjectName(QStringLiteral("sectionTitle"));
    rootLayout->addWidget(popularTitleLabel);

    m_popularRoutesTable = new QTableWidget(this);
    m_popularRoutesTable->setColumnCount(3);
    m_popularRoutesTable->setHorizontalHeaderLabels({
        QStringLiteral("出发站"),
        QStringLiteral("到达站"),
        QStringLiteral("订单数")
    });
    UiStyle::prepareTable(m_popularRoutesTable);
    m_popularRoutesTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    rootLayout->addWidget(m_popularRoutesTable);

    auto *monthlyTitleLabel = new QLabel(QStringLiteral("月度客流"), this);
    monthlyTitleLabel->setObjectName(QStringLiteral("sectionTitle"));
    rootLayout->addWidget(monthlyTitleLabel);

    m_monthlyFlowTable = new QTableWidget(this);
    m_monthlyFlowTable->setColumnCount(4);
    m_monthlyFlowTable->setHorizontalHeaderLabels({
        QStringLiteral("月份"),
        QStringLiteral("总订单"),
        QStringLiteral("已订票"),
        QStringLiteral("已退票")
    });
    UiStyle::prepareTable(m_monthlyFlowTable);
    m_monthlyFlowTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    rootLayout->addWidget(m_monthlyFlowTable);

    auto *closeButton = new QPushButton(QStringLiteral("关闭"), this);
    connect(closeButton, &QPushButton::clicked, this, &QDialog::accept);
    rootLayout->addWidget(closeButton, 0, Qt::AlignRight);

    loadData();
}

void StatisticsDialog::loadData()
{
    if (m_statisticsManager == nullptr) {
        return;
    }

    m_totalOrdersValueLabel->setText(QString::number(m_statisticsManager->totalOrders()));
    m_totalBookedValueLabel->setText(QString::number(m_statisticsManager->totalBooked()));
    m_totalRefundedValueLabel->setText(QString::number(m_statisticsManager->totalRefunded()));
    m_totalChangedValueLabel->setText(QString::number(m_statisticsManager->totalChanged()));

    const auto routes = m_statisticsManager->popularRoutes(10);
    m_popularRoutesTable->setRowCount(routes.size());
    for (int row = 0; row < routes.size(); ++row) {
        const QVariantMap &route = routes[row];
        m_popularRoutesTable->setItem(row, 0,
                                      new QTableWidgetItem(route.value(QStringLiteral("departureStation")).toString()));
        m_popularRoutesTable->setItem(row, 1,
                                      new QTableWidgetItem(route.value(QStringLiteral("arrivalStation")).toString()));
        m_popularRoutesTable->setItem(row, 2,
                                      new QTableWidgetItem(QString::number(route.value(QStringLiteral("orderCount")).toInt())));
    }

    const auto monthlyStats = m_statisticsManager->monthlyPassengerFlow();
    m_monthlyFlowTable->setRowCount(monthlyStats.size());
    for (int row = 0; row < monthlyStats.size(); ++row) {
        const QVariantMap &stat = monthlyStats[row];
        m_monthlyFlowTable->setItem(row, 0,
                                    new QTableWidgetItem(stat.value(QStringLiteral("month")).toString()));
        m_monthlyFlowTable->setItem(row, 1,
                                    new QTableWidgetItem(QString::number(stat.value(QStringLiteral("totalOrders")).toInt())));
        m_monthlyFlowTable->setItem(row, 2,
                                    new QTableWidgetItem(QString::number(stat.value(QStringLiteral("booked")).toInt())));
        m_monthlyFlowTable->setItem(row, 3,
                                    new QTableWidgetItem(QString::number(stat.value(QStringLiteral("refunded")).toInt())));
    }

}

QLabel *StatisticsDialog::createMetricValueLabel()
{
    auto *label = new QLabel(QStringLiteral("0"), this);
    label->setObjectName(QStringLiteral("metricValue"));
    return label;
}
