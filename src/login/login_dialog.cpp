#include "login_dialog.h"

#include <QColor>
#include <QFrame>
#include <QFormLayout>
#include <QGraphicsDropShadowEffect>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QVBoxLayout>

LoginDialog::LoginDialog(const LoginManager &loginManager, QWidget *parent)
    : QDialog(parent)
    , m_loginManager(loginManager)
{
    setWindowTitle(QStringLiteral("火车票务管理系统登录"));
    setModal(true);
    setFixedSize(560, 520);

    // 样式先放在登录窗口里，避免影响别的窗口。
    setStyleSheet(QStringLiteral(R"QSS(
        QDialog {
            background: #edf3f1;
            color: #1f2933;
            font-family: "Microsoft YaHei UI", "Microsoft YaHei", "Segoe UI";
            font-size: 14px;
        }
        QFrame#loginShell {
            background: #fbfdfc;
            border: 1px solid #d5dfda;
            border-radius: 14px;
        }
        QLabel#systemTag {
            color: #0f766e;
            font-size: 13px;
            font-weight: 700;
            letter-spacing: 0px;
        }
        QLabel#systemTitle {
            color: #12221e;
            font-size: 28px;
            font-weight: 700;
        }
        QLabel#systemSubtitle {
            color: #65716c;
            font-size: 13px;
        }
        QLabel#roleLine {
            color: #37534b;
            background: #eef7f3;
            border: 1px solid #cfe2da;
            border-radius: 8px;
            padding: 7px 12px;
            font-size: 12px;
        }
        QLabel#formTitle {
            color: #17231f;
            font-size: 22px;
            font-weight: 700;
        }
        QLabel#formSubtitle {
            color: #65716c;
            font-size: 13px;
        }
        QLabel#messageLabel {
            border-radius: 8px;
            padding: 8px 10px;
            font-size: 13px;
        }
        QLabel[class="formLabel"] {
            color: #33433d;
            font-weight: 600;
        }
        QLineEdit {
            min-height: 38px;
            border: 1px solid #cbd8d2;
            border-radius: 8px;
            padding: 4px 12px;
            background: #ffffff;
            color: #1f2933;
            selection-background-color: #0f766e;
        }
        QLineEdit:focus {
            border: 2px solid #0f766e;
            padding: 3px 11px;
        }
        QPushButton {
            min-height: 38px;
            border-radius: 8px;
            padding: 6px 18px;
            font-weight: 700;
        }
        QPushButton#loginButton {
            color: #ffffff;
            background: #0f766e;
            border: 1px solid #0f766e;
        }
        QPushButton#loginButton:hover {
            background: #115e59;
            border-color: #115e59;
        }
        QPushButton#guestButton {
            color: #33433d;
            background: #eef5f1;
            border: 1px solid #cbd8d2;
        }
        QPushButton#guestButton:hover {
            background: #e1eee8;
            border-color: #9fb5aa;
        }
    )QSS"));

    auto *rootLayout = new QVBoxLayout(this);
    rootLayout->setContentsMargins(30, 28, 30, 28);

    // 外层框负责白色背景、圆角和阴影。
    auto *shellFrame = new QFrame(this);
    shellFrame->setObjectName(QStringLiteral("loginShell"));

    auto *shadow = new QGraphicsDropShadowEffect(shellFrame);
    shadow->setBlurRadius(28);
    shadow->setOffset(0, 12);
    shadow->setColor(QColor(24, 45, 39, 55));
    shellFrame->setGraphicsEffect(shadow);

    auto *shellLayout = new QVBoxLayout(shellFrame);
    shellLayout->setContentsMargins(42, 34, 42, 34);
    shellLayout->setSpacing(16);

    auto *systemTag = new QLabel(QStringLiteral("铁路票务入口"), shellFrame);
    systemTag->setObjectName(QStringLiteral("systemTag"));

    auto *systemTitle = new QLabel(QStringLiteral("火车票务管理系统"), shellFrame);
    systemTitle->setObjectName(QStringLiteral("systemTitle"));

    auto *systemSubtitle = new QLabel(QStringLiteral("按账号身份进入对应功能，也可以游客身份查询余票。"), shellFrame);
    systemSubtitle->setObjectName(QStringLiteral("systemSubtitle"));
    systemSubtitle->setWordWrap(true);

    auto *roleLine = new QLabel(QStringLiteral("游客查询 / 普通用户 / 售票员 / 管理员"), shellFrame);
    roleLine->setObjectName(QStringLiteral("roleLine"));

    // 表单区域只放控件，不在这里写账号规则。
    auto *formPanel = new QFrame(shellFrame);
    auto *formLayout = new QVBoxLayout(formPanel);
    formLayout->setContentsMargins(0, 10, 0, 0);
    formLayout->setSpacing(16);

    m_formTitleLabel = new QLabel(QStringLiteral("欢迎登录"), formPanel);
    m_formTitleLabel->setObjectName(QStringLiteral("formTitle"));

    m_formSubtitleLabel = new QLabel(QStringLiteral("请输入账号密码，或以游客身份进入系统。"), formPanel);
    m_formSubtitleLabel->setObjectName(QStringLiteral("formSubtitle"));

    auto *inputLayout = new QFormLayout;
    inputLayout->setContentsMargins(0, 4, 0, 0);
    inputLayout->setHorizontalSpacing(16);
    inputLayout->setVerticalSpacing(12);
    inputLayout->setLabelAlignment(Qt::AlignLeft);

    auto *usernameLabel = new QLabel(QStringLiteral("用户名"), formPanel);
    usernameLabel->setProperty("class", QStringLiteral("formLabel"));
    auto *passwordLabel = new QLabel(QStringLiteral("密码"), formPanel);
    passwordLabel->setProperty("class", QStringLiteral("formLabel"));
    m_confirmPasswordLabel = new QLabel(QStringLiteral("确认密码"), formPanel);
    m_confirmPasswordLabel->setProperty("class", QStringLiteral("formLabel"));

    m_usernameEdit = new QLineEdit(formPanel);
    m_usernameEdit->setPlaceholderText(QStringLiteral("请输入用户名"));
    m_usernameEdit->setClearButtonEnabled(true);

    m_passwordEdit = new QLineEdit(formPanel);
    m_passwordEdit->setPlaceholderText(QStringLiteral("请输入密码"));
    m_passwordEdit->setEchoMode(QLineEdit::Password);
    m_passwordEdit->setClearButtonEnabled(true);

    m_confirmPasswordEdit = new QLineEdit(formPanel);
    m_confirmPasswordEdit->setPlaceholderText(QStringLiteral("注册时请再次输入密码"));
    m_confirmPasswordEdit->setEchoMode(QLineEdit::Password);
    m_confirmPasswordEdit->setClearButtonEnabled(true);

    inputLayout->addRow(usernameLabel, m_usernameEdit);
    inputLayout->addRow(passwordLabel, m_passwordEdit);
    inputLayout->addRow(m_confirmPasswordLabel, m_confirmPasswordEdit);

    m_messageLabel = new QLabel(QStringLiteral(" "), formPanel);
    m_messageLabel->setObjectName(QStringLiteral("messageLabel"));
    m_messageLabel->setWordWrap(true);
    m_messageLabel->setMinimumHeight(42);

    auto *buttonLayout = new QHBoxLayout;
    buttonLayout->setSpacing(12);

    // 游客入口放左边，登录和注册入口放右边。
    m_guestButton = new QPushButton(QStringLiteral("游客进入"), formPanel);
    m_guestButton->setObjectName(QStringLiteral("guestButton"));

    m_backToLoginButton = new QPushButton(QStringLiteral("返回登录"), formPanel);
    m_backToLoginButton->setObjectName(QStringLiteral("guestButton"));

    m_openRegisterButton = new QPushButton(QStringLiteral("注册用户"), formPanel);
    m_openRegisterButton->setObjectName(QStringLiteral("guestButton"));

    m_registerButton = new QPushButton(QStringLiteral("完成注册"), formPanel);
    m_registerButton->setObjectName(QStringLiteral("loginButton"));

    m_loginButton = new QPushButton(QStringLiteral("登录"), formPanel);
    m_loginButton->setObjectName(QStringLiteral("loginButton"));
    m_loginButton->setDefault(true);

    buttonLayout->addWidget(m_guestButton);
    buttonLayout->addStretch();
    buttonLayout->addWidget(m_backToLoginButton);
    buttonLayout->addWidget(m_openRegisterButton);
    buttonLayout->addWidget(m_registerButton);
    buttonLayout->addWidget(m_loginButton);

    shellLayout->addWidget(systemTag);
    shellLayout->addWidget(systemTitle);
    shellLayout->addWidget(systemSubtitle);
    shellLayout->addWidget(roleLine);
    formLayout->addWidget(m_formTitleLabel);
    formLayout->addWidget(m_formSubtitleLabel);
    formLayout->addLayout(inputLayout);
    formLayout->addWidget(m_messageLabel);
    formLayout->addStretch();
    formLayout->addLayout(buttonLayout);

    shellLayout->addWidget(formPanel);
    rootLayout->addWidget(shellFrame);

    // 按钮只负责切换界面或提交输入，具体账号判断仍然交给 LoginManager。
    connect(m_loginButton, &QPushButton::clicked, this, [this]() {
        handleLogin();
    });
    connect(m_openRegisterButton, &QPushButton::clicked, this, [this]() {
        showRegisterForm();
    });
    connect(m_registerButton, &QPushButton::clicked, this, [this]() {
        handleRegister();
    });
    connect(m_backToLoginButton, &QPushButton::clicked, this, [this]() {
        showLoginForm();
    });
    connect(m_guestButton, &QPushButton::clicked, this, [this]() {
        handleGuestAccess();
    });
    connect(m_passwordEdit, &QLineEdit::returnPressed, this, [this]() {
        if (m_isRegisterMode) {
            m_confirmPasswordEdit->setFocus();
            return;
        }
        handleLogin();
    });
    connect(m_confirmPasswordEdit, &QLineEdit::returnPressed, this, [this]() {
        handleRegister();
    });

    showLoginForm();
}

LoginResult LoginDialog::loginResult() const
{
    return m_loginResult;
}

void LoginDialog::showLoginForm()
{
    m_isRegisterMode = false;

    m_formTitleLabel->setText(QStringLiteral("欢迎登录"));
    m_formSubtitleLabel->setText(QStringLiteral("请输入账号密码，或以游客身份进入系统。"));

    // 登录只需要用户名和密码，确认密码只在注册时出现。
    m_confirmPasswordLabel->setVisible(false);
    m_confirmPasswordEdit->setVisible(false);
    m_confirmPasswordEdit->clear();

    m_guestButton->setVisible(true);
    m_openRegisterButton->setVisible(true);
    m_loginButton->setVisible(true);
    m_backToLoginButton->setVisible(false);
    m_registerButton->setVisible(false);

    m_loginButton->setDefault(true);
    m_registerButton->setDefault(false);
    clearMessage();
    m_usernameEdit->setFocus();
}

void LoginDialog::showRegisterForm()
{
    m_isRegisterMode = true;

    m_formTitleLabel->setText(QStringLiteral("注册普通用户"));
    m_formSubtitleLabel->setText(QStringLiteral("创建普通用户账号，注册后可以用账号密码登录。"));

    m_confirmPasswordLabel->setVisible(true);
    m_confirmPasswordEdit->setVisible(true);
    m_passwordEdit->clear();
    m_confirmPasswordEdit->clear();

    m_guestButton->setVisible(false);
    m_openRegisterButton->setVisible(false);
    m_loginButton->setVisible(false);
    m_backToLoginButton->setVisible(true);
    m_registerButton->setVisible(true);

    m_loginButton->setDefault(false);
    m_registerButton->setDefault(true);
    clearMessage();
    m_passwordEdit->setFocus();
}

void LoginDialog::clearMessage()
{
    m_messageLabel->setText(QStringLiteral(" "));
    m_messageLabel->setStyleSheet(QString());
}

void LoginDialog::handleLogin()
{
    // 这里只负责拿输入，真正的判断交给 LoginManager。
    m_loginResult = m_loginManager.authenticate(m_usernameEdit->text(), m_passwordEdit->text());

    if (!m_loginResult.success) {
        showMessage(m_loginResult.message);
        return;
    }

    accept();
}

void LoginDialog::handleRegister()
{
    if (!m_isRegisterMode) {
        showRegisterForm();
        return;
    }

    if (m_passwordEdit->text() != m_confirmPasswordEdit->text()) {
        showMessage(QStringLiteral("两次输入的密码不一致。"));
        return;
    }

    // 注册只创建普通用户账号，具体写库逻辑交给 LoginManager。
    const AccountResult result =
        m_loginManager.registerUser(m_usernameEdit->text(), m_passwordEdit->text());

    if (result.success) {
        m_passwordEdit->clear();
        m_confirmPasswordEdit->clear();
        showLoginForm();
        showMessage(result.message, true);
        return;
    }

    showMessage(result.message);
}

void LoginDialog::handleGuestAccess()
{
    m_loginResult = m_loginManager.loginAsGuest();
    accept();
}

void LoginDialog::showMessage(const QString &message, bool success)
{
    m_messageLabel->setText(message);
    if (success) {
        m_messageLabel->setStyleSheet(QStringLiteral("color: #14532d; background: #dcfce7; border: 1px solid #86efac;"));
        return;
    }

    m_messageLabel->setStyleSheet(QStringLiteral("color: #9f1239; background: #fff1f2; border: 1px solid #fecdd3;"));
}
