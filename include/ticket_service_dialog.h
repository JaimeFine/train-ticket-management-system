#pragma once

#include <QDialog>

#include "login_manager.h"

class QLineEdit;
class QLabel;
class QTabWidget;
class QTableWidget;
class TicketManager;

class TicketServiceDialog : public QDialog
{
public:
    explicit TicketServiceDialog(TicketManager &ticketManager,
                                 const LoginResult &loginResult,
                                 int initialTabIndex = 0,
                                 QWidget *parent = nullptr);

private:
    void setupSearchTab();
    void setupManageTab();
    void setupQueryTab();

    void searchTrips();
    void bookSelectedTrain();
    void refundOrder();
    void changeOrder();
    void runOrderQuery();
    void loadOwnOrders();
    void showMessage(bool success, const QString &message);
    QString statusText(int status) const;
    int selectedTrainId() const;
    void writeOperationLog(const QString &action, const QString &detail);

    TicketManager *m_ticketManager = nullptr;
    LoginResult m_loginResult;
    QTabWidget *m_tabWidget = nullptr;

    QLineEdit *m_departureEdit = nullptr;
    QLineEdit *m_arrivalEdit = nullptr;
    QLineEdit *m_trainNumberEdit = nullptr;
    QLineEdit *m_passengerNameEdit = nullptr;
    QTableWidget *m_searchResultsTable = nullptr;

    QLineEdit *m_refundOrderIdEdit = nullptr;
    QLineEdit *m_changeOrderIdEdit = nullptr;
    QLineEdit *m_changeTrainIdEdit = nullptr;

    QLineEdit *m_queryPassengerEdit = nullptr;
    QLineEdit *m_queryOrderIdEdit = nullptr;
    QTableWidget *m_queryResultsTable = nullptr;
    QLabel *m_messageLabel = nullptr;
};
