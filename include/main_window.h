#pragma once

#include <QMainWindow>

#include "login_manager.h"

class TrainManager;

class MainWindow : public QMainWindow
{
public:
    explicit MainWindow(const LoginResult &loginResult,
                        const LoginManager &loginManager,
                        TrainManager* trainManager,
                        QWidget *parent = nullptr);
    bool logoutRequested() const;

private:
    LoginResult m_loginResult;
    const LoginManager *m_loginManager = nullptr;
    TrainManager* m_trainManager = nullptr;
    bool m_logoutRequested = false;
};
