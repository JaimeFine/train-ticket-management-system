#pragma once
/** @file ticket_manage_dialog.h - Issue 9+10: 订票+退票+改签 */
#include <QDialog>
class DatabaseManager;
class QLabel;
class QLineEdit;

class TicketManageDialog : public QDialog
{
public:
    explicit TicketManageDialog(DatabaseManager &db, int userId,
                                QWidget *parent = nullptr);
private:
    void handleBook();
    void handleRefund();
    void handleChange();
    void showMsg(bool ok, const QString &msg);
    DatabaseManager &m_db;
    int m_userId;
    QLineEdit *m_bookTrainEdit  = nullptr;
    QLineEdit *m_bookNameEdit   = nullptr;
    QLineEdit *m_refundOrderEdit = nullptr;
    QLineEdit *m_changeOrderEdit = nullptr;
    QLineEdit *m_changeTrainEdit = nullptr;
    QLabel    *m_msgLabel        = nullptr;
};
