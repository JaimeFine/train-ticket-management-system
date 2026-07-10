#pragma once

#include <QDialog>

#include "login_manager.h"

class QLabel;
class QLineEdit;
class QPushButton;

class LoginDialog : public QDialog
{
public:
    explicit LoginDialog(const LoginManager &loginManager, QWidget *parent = nullptr);

    LoginResult loginResult() const;

private:
    void handleLogin();
    void handleRegister();
    void handleGuestAccess();
    void showLoginForm();
    void showRegisterForm();
    void clearMessage();
    void showMessage(const QString &message, bool success = false);

    const LoginManager &m_loginManager;
    LoginResult m_loginResult;
    bool m_isRegisterMode = false;
    QLineEdit *m_usernameEdit = nullptr;
    QLineEdit *m_passwordEdit = nullptr;
    QLineEdit *m_confirmPasswordEdit = nullptr;
    QLabel *m_formTitleLabel = nullptr;
    QLabel *m_formSubtitleLabel = nullptr;
    QLabel *m_confirmPasswordLabel = nullptr;
    QLabel *m_messageLabel = nullptr;
    QPushButton *m_guestButton = nullptr;
    QPushButton *m_openRegisterButton = nullptr;
    QPushButton *m_registerButton = nullptr;
    QPushButton *m_loginButton = nullptr;
    QPushButton *m_backToLoginButton = nullptr;
};
