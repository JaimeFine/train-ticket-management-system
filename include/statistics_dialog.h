#pragma once
/** @file statistics_dialog.h - Issue 11: 管理员统计面板 */
#include <QDialog>
class DatabaseManager;
class QLabel;
class QTableWidget;

class StatisticsDialog : public QDialog
{
public:
    explicit StatisticsDialog(DatabaseManager &db, QWidget *parent = nullptr);

private:
    void refresh();
    DatabaseManager &m_db;
    QLabel *m_messageLabel    = nullptr;
    QLabel *m_totalOrdersVal  = nullptr;
    QLabel *m_totalBookedVal  = nullptr;
    QLabel *m_totalRefundedVal= nullptr;
    QLabel *m_totalChangedVal = nullptr;
    QTableWidget *m_routesTable   = nullptr;
    QTableWidget *m_monthlyTable  = nullptr;
};
