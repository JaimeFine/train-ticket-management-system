#pragma once
/** @file ticket_search_dialog.h - Issue 9: 车票查询 */
#include <QDialog>
class DatabaseManager;
class QLineEdit;
class QTableWidget;

class TicketSearchDialog : public QDialog
{
public:
    explicit TicketSearchDialog(DatabaseManager &db, QWidget *parent = nullptr);
private:
    void searchByStation();
    void searchByNumber();
    void populateTable(const QVector<QVariantMap> &results);
    DatabaseManager &m_db;
    QLineEdit *m_depEdit   = nullptr;
    QLineEdit *m_arrEdit   = nullptr;
    QLineEdit *m_dateEdit  = nullptr;
    QLineEdit *m_numEdit   = nullptr;
    QTableWidget *m_table  = nullptr;
};
