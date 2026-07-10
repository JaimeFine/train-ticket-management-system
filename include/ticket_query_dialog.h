#pragma once
/** @file ticket_query_dialog.h - Issue 10: 票务查询 */
#include <QDialog>
class DatabaseManager;
class QLineEdit;
class QTableWidget;

class TicketQueryDialog : public QDialog
{
public:
    explicit TicketQueryDialog(DatabaseManager &db, QWidget *parent = nullptr);
private:
    void searchByOrderId();
    void searchByPassenger();
    void populateTable(const QVector<QVariantMap> &results);
    DatabaseManager &m_db;
    QLineEdit *m_orderEdit    = nullptr;
    QLineEdit *m_passengerEdit = nullptr;
    QTableWidget *m_table     = nullptr;
};
