#pragma once

#include <QDialog>

#include "login_manager.h"

class QLabel;
class QLineEdit;
class QTableWidget;

class AccountManagementDialog : public QDialog
{
public:
    explicit AccountManagementDialog(const LoginManager &loginManager,
                                     const LoginResult &loginResult,
                                     QWidget *parent = nullptr,
                                     bool accountOnly = false);
    bool logoutRequested() const;

private:
    void handleRegisterUser();
    void handleCreateSeller();
    void handleResetSelectedSellerPassword();
    void handleSetSelectedSellerEnabled(bool enabled);
    void handleChangeOwnPassword();
    void handleLogout();
    void refreshSellerTable();
    QString selectedSellerUsername() const;
    void showMessage(const AccountResult &result);
    void showPlainMessage(bool success, const QString &message);

    const LoginManager &m_loginManager;
    LoginResult m_loginResult;
    bool m_accountOnly = false;
    bool m_logoutRequested = false;

    QLineEdit *m_registerUsernameEdit = nullptr;
    QLineEdit *m_registerPasswordEdit = nullptr;
    QLineEdit *m_registerConfirmEdit = nullptr;

    QLineEdit *m_createUsernameEdit = nullptr;
    QLineEdit *m_createPasswordEdit = nullptr;
    QLineEdit *m_createConfirmEdit = nullptr;

    QTableWidget *m_sellerTable = nullptr;

    QLineEdit *m_oldPasswordEdit = nullptr;
    QLineEdit *m_newPasswordEdit = nullptr;
    QLineEdit *m_newConfirmEdit = nullptr;

    QLabel *m_messageLabel = nullptr;
};
