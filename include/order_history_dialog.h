#pragma once
/** @file order_history_dialog.h - Issue 11: 订单历史面板 */
#include <QDialog>
class DatabaseManager;
class QLabel;
class QTableWidget;

class OrderHistoryDialog : public QDialog
{
public:
    /// @param userId  -1 = show all orders; >=0 = filter by userId
    explicit OrderHistoryDialog(DatabaseManager &db, int userId = -1,
                                QWidget *parent = nullptr);
private:
    void refresh();
    DatabaseManager &m_db;
    int m_userId;
    QTableWidget *m_ordersTable = nullptr;
};
