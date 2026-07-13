#pragma once

#include <QDialog>

class QLabel;
class QTableWidget;
class StatisticsManager;

class StatisticsDialog : public QDialog
{
public:
    explicit StatisticsDialog(const StatisticsManager &statisticsManager,
                              QWidget *parent = nullptr);

private:
    void loadData();
    QLabel *createMetricValueLabel();

    const StatisticsManager *m_statisticsManager = nullptr;
    QLabel *m_totalOrdersValueLabel = nullptr;
    QLabel *m_totalBookedValueLabel = nullptr;
    QLabel *m_totalRefundedValueLabel = nullptr;
    QLabel *m_totalChangedValueLabel = nullptr;
    QTableWidget *m_popularRoutesTable = nullptr;
    QTableWidget *m_monthlyFlowTable = nullptr;
};
