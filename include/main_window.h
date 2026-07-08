#pragma once

#include <QMainWindow>

#include "login_manager.h"

class MainWindow : public QMainWindow
{
public:
    explicit MainWindow(const LoginResult &loginResult, QWidget *parent = nullptr);

private:
    LoginResult m_loginResult;
};
