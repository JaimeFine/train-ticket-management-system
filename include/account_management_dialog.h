#pragma once

#include <QDialog>

#include "login_manager.h"

class QLabel;
class QLineEdit;

class AccountManagementDialog : public QDialog
{
public:
    explicit AccountManagementDialog(const LoginManager &loginManager,
                                     const LoginResult &loginResult,
                                     QWidget *parent = nullptr);

private:
    void handleCreateSeller();
    void handleResetSellerPassword();
    void handleSetSellerEnabled(bool enabled);
    void handleChangeOwnPassword();
    void showMessage(const AccountResult &result);
    void showPlainMessage(bool success, const QString &message);

    const LoginManager &m_loginManager;
    LoginResult m_loginResult;

    QLineEdit *m_createUsernameEdit = nullptr;
    QLineEdit *m_createPasswordEdit = nullptr;
    QLineEdit *m_createConfirmEdit = nullptr;

    QLineEdit *m_manageUsernameEdit = nullptr;
    QLineEdit *m_resetPasswordEdit = nullptr;
    QLineEdit *m_resetConfirmEdit = nullptr;

    QLineEdit *m_oldPasswordEdit = nullptr;
    QLineEdit *m_newPasswordEdit = nullptr;
    QLineEdit *m_newConfirmEdit = nullptr;

    QLabel *m_messageLabel = nullptr;
};
