#pragma once

#include <QDialog>

#include "login_manager.h"

class QLabel;
class QLineEdit;

class LoginDialog : public QDialog
{
public:
    explicit LoginDialog(const LoginManager &loginManager, QWidget *parent = nullptr);

    LoginResult loginResult() const;

private:
    void handleLogin();
    void handleGuestAccess();
    void showMessage(const QString &message, bool isError);

    const LoginManager &m_loginManager;
    LoginResult m_loginResult;
    QLineEdit *m_usernameEdit = nullptr;
    QLineEdit *m_passwordEdit = nullptr;
    QLabel *m_messageLabel = nullptr;
};
