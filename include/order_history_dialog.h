#pragma once

#include <QDialog>

class QTableWidget;
class TicketManager;

class OrderHistoryDialog : public QDialog
{
public:
    explicit OrderHistoryDialog(const TicketManager &ticketManager,
                                int userId,
                                QWidget *parent = nullptr);

private:
    void loadOrders();
    QString statusText(int status) const;

    const TicketManager *m_ticketManager = nullptr;
    int m_userId = 0;
    QTableWidget *m_ordersTable = nullptr;
};
