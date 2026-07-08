#include "login_dialog.h"

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
    setFixedSize(680, 430);

    // 所有视觉样式集中写在登录对话框内部，避免影响其他模块窗口。
    setStyleSheet(QStringLiteral(R"QSS(
        QDialog {
            background: #eef2f3;
            color: #1f2933;
            font-family: "Microsoft YaHei UI", "Microsoft YaHei", "Segoe UI";
            font-size: 14px;
        }
        QFrame#loginShell {
            background: #fbfcfb;
            border: 1px solid #d8e0dc;
            border-radius: 16px;
        }
        QFrame#brandPanel {
            background: #153832;
            border-top-left-radius: 16px;
            border-bottom-left-radius: 16px;
        }
        QLabel#brandTag {
            color: #a7f3d0;
            font-size: 12px;
            font-weight: 700;
            letter-spacing: 0px;
        }
        QLabel#brandTitle {
            color: #ffffff;
            font-size: 26px;
            font-weight: 700;
        }
        QLabel#brandSubtitle {
            color: #d5e7df;
            font-size: 13px;
            line-height: 1.4;
        }
        QLabel[class="brandMetric"] {
            color: #effaf5;
            background: rgba(255, 255, 255, 0.08);
            border: 1px solid rgba(255, 255, 255, 0.14);
            border-radius: 8px;
            padding: 8px 10px;
        }
        QLabel#formTitle {
            color: #17231f;
            font-size: 24px;
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
    rootLayout->setContentsMargins(22, 22, 22, 22);

    // shellFrame 是登录卡片的外层容器，用来统一圆角、边框和阴影。
    auto *shellFrame = new QFrame(this);
    shellFrame->setObjectName(QStringLiteral("loginShell"));

    auto *shadow = new QGraphicsDropShadowEffect(shellFrame);
    shadow->setBlurRadius(28);
    shadow->setOffset(0, 12);
    shadow->setColor(QColor(24, 45, 39, 55));
    shellFrame->setGraphicsEffect(shadow);

    auto *shellLayout = new QHBoxLayout(shellFrame);
    shellLayout->setContentsMargins(0, 0, 0, 0);
    shellLayout->setSpacing(0);

    // 左侧品牌区负责展示系统身份，右侧表单区负责真正的输入交互。
    auto *brandPanel = new QFrame(shellFrame);
    brandPanel->setObjectName(QStringLiteral("brandPanel"));
    brandPanel->setFixedWidth(245);

    auto *brandLayout = new QVBoxLayout(brandPanel);
    brandLayout->setContentsMargins(28, 34, 28, 30);
    brandLayout->setSpacing(14);

    auto *brandTag = new QLabel(QStringLiteral("铁路票务入口"), brandPanel);
    brandTag->setObjectName(QStringLiteral("brandTag"));

    auto *brandTitle = new QLabel(QStringLiteral("火车票务\n管理系统"), brandPanel);
    brandTitle->setObjectName(QStringLiteral("brandTitle"));
    brandTitle->setWordWrap(true);

    auto *brandSubtitle = new QLabel(QStringLiteral("面向售票、查询与管理流程的统一登录入口。"), brandPanel);
    brandSubtitle->setObjectName(QStringLiteral("brandSubtitle"));
    brandSubtitle->setWordWrap(true);

    brandLayout->addWidget(brandTag);
    brandLayout->addWidget(brandTitle);
    brandLayout->addWidget(brandSubtitle);
    brandLayout->addSpacing(18);

    // 这些短标签只做展示，表达游客、售票员、管理员三类入口已经预留。
    const QStringList metricTexts = {
        QStringLiteral("游客余票查询"),
        QStringLiteral("售票员业务入口"),
        QStringLiteral("管理员权限预留")
    };

    for (const QString &text : metricTexts) {
        auto *metricLabel = new QLabel(text, brandPanel);
        metricLabel->setProperty("class", QStringLiteral("brandMetric"));
        brandLayout->addWidget(metricLabel);
    }

    brandLayout->addStretch();

    // 右侧表单继续沿用 LoginManager 认证逻辑，只改变控件排版和视觉层级。
    auto *formPanel = new QFrame(shellFrame);
    auto *formLayout = new QVBoxLayout(formPanel);
    formLayout->setContentsMargins(44, 38, 44, 34);
    formLayout->setSpacing(18);

    auto *formTitle = new QLabel(QStringLiteral("欢迎登录"), formPanel);
    formTitle->setObjectName(QStringLiteral("formTitle"));

    auto *formSubtitle = new QLabel(QStringLiteral("请输入账号密码，或以游客身份进入系统。"), formPanel);
    formSubtitle->setObjectName(QStringLiteral("formSubtitle"));

    auto *inputLayout = new QFormLayout;
    inputLayout->setContentsMargins(0, 4, 0, 0);
    inputLayout->setHorizontalSpacing(16);
    inputLayout->setVerticalSpacing(12);
    inputLayout->setLabelAlignment(Qt::AlignLeft);

    auto *usernameLabel = new QLabel(QStringLiteral("用户名"), formPanel);
    usernameLabel->setProperty("class", QStringLiteral("formLabel"));
    auto *passwordLabel = new QLabel(QStringLiteral("密码"), formPanel);
    passwordLabel->setProperty("class", QStringLiteral("formLabel"));

    m_usernameEdit = new QLineEdit(formPanel);
    m_usernameEdit->setPlaceholderText(QStringLiteral("请输入用户名"));
    m_usernameEdit->setClearButtonEnabled(true);

    m_passwordEdit = new QLineEdit(formPanel);
    m_passwordEdit->setPlaceholderText(QStringLiteral("请输入密码"));
    m_passwordEdit->setEchoMode(QLineEdit::Password);
    m_passwordEdit->setClearButtonEnabled(true);

    inputLayout->addRow(usernameLabel, m_usernameEdit);
    inputLayout->addRow(passwordLabel, m_passwordEdit);

    m_messageLabel = new QLabel(QStringLiteral(" "), formPanel);
    m_messageLabel->setObjectName(QStringLiteral("messageLabel"));
    m_messageLabel->setWordWrap(true);
    m_messageLabel->setMinimumHeight(42);

    auto *buttonLayout = new QHBoxLayout;
    buttonLayout->setSpacing(12);

    // Guest 是次要入口，Login 是主操作按钮，所以使用不同的按钮样式。
    auto *guestButton = new QPushButton(QStringLiteral("游客进入"), formPanel);
    guestButton->setObjectName(QStringLiteral("guestButton"));

    auto *loginButton = new QPushButton(QStringLiteral("登录"), formPanel);
    loginButton->setObjectName(QStringLiteral("loginButton"));
    loginButton->setDefault(true);

    buttonLayout->addWidget(guestButton);
    buttonLayout->addStretch();
    buttonLayout->addWidget(loginButton);

    formLayout->addWidget(formTitle);
    formLayout->addWidget(formSubtitle);
    formLayout->addLayout(inputLayout);
    formLayout->addWidget(m_messageLabel);
    formLayout->addStretch();
    formLayout->addLayout(buttonLayout);

    shellLayout->addWidget(brandPanel);
    shellLayout->addWidget(formPanel, 1);
    rootLayout->addWidget(shellFrame);

    // 登录按钮和回车键复用同一处理函数，避免登录流程分散在多个 UI 事件里。
    connect(loginButton, &QPushButton::clicked, this, [this]() {
        handleLogin();
    });
    connect(guestButton, &QPushButton::clicked, this, [this]() {
        handleGuestAccess();
    });
    connect(m_passwordEdit, &QLineEdit::returnPressed, this, [this]() {
        handleLogin();
    });
}

LoginResult LoginDialog::loginResult() const
{
    return m_loginResult;
}

void LoginDialog::handleLogin()
{
    // 对话框只负责收集输入和展示结果，具体认证规则交给 LoginManager。
    m_loginResult = m_loginManager.authenticate(m_usernameEdit->text(), m_passwordEdit->text());

    if (!m_loginResult.success) {
        showMessage(m_loginResult.message, true);
        return;
    }

    accept();
}

void LoginDialog::handleGuestAccess()
{
    m_loginResult = m_loginManager.loginAsGuest();
    accept();
}

void LoginDialog::showMessage(const QString &message, bool isError)
{
    m_messageLabel->setText(message);
    // 错误和成功提示使用不同底色，便于快速观察验证结果。
    m_messageLabel->setStyleSheet(isError
                                      ? QStringLiteral("color: #9f1239; background: #fff1f2; border: 1px solid #fecdd3;")
                                      : QStringLiteral("color: #14532d; background: #f0fdf4; border: 1px solid #bbf7d0;"));
}
