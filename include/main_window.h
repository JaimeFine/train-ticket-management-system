#pragma once

#include <QMainWindow>

#include "login_manager.h"

class DatabaseManager;

class MainWindow : public QMainWindow
{
public:
    explicit MainWindow(const LoginResult &loginResult,
                        const LoginManager &loginManager,
                        DatabaseManager &db,
                        QWidget *parent = nullptr);
    bool logoutRequested() const;

private:
    LoginResult m_loginResult;
    const LoginManager *m_loginManager = nullptr;
    DatabaseManager &m_db;
    bool m_logoutRequested = false;
};
