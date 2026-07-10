#pragma once

#include <QMainWindow>

#include "login_manager.h"

class MainWindow : public QMainWindow
{
public:
    explicit MainWindow(const LoginResult &loginResult,
                        const LoginManager &loginManager,
                        QWidget *parent = nullptr);
    bool logoutRequested() const;

private:
    LoginResult m_loginResult;
    const LoginManager *m_loginManager = nullptr;
    bool m_logoutRequested = false;
};
