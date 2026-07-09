#include "account_management_dialog.h"
#include "main_window.h"

#include <QFrame>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QMessageBox>
#include <QPushButton>
#include <QStatusBar>
#include <QVBoxLayout>
#include <QWidget>

namespace {
QString roleName(UserRole role)
{
    switch (role) {
    case UserRole::Admin:
        return QStringLiteral("管理员");
    case UserRole::Seller:
        return QStringLiteral("售票员");
    case UserRole::Guest:
        return QStringLiteral("游客");
    }

    return QStringLiteral("未知角色");
}

QString roleHint(UserRole role)
{
    switch (role) {
    case UserRole::Admin:
        return QStringLiteral("管理员可进入账号管理、票务办理和查询入口。");
    case UserRole::Seller:
        return QStringLiteral("售票员可进入票务办理和查询入口，不能进入账号管理。");
    case UserRole::Guest:
        return QStringLiteral("游客只开放基础查询入口，不能办理售票和账号管理。");
    }

    return QStringLiteral("当前身份暂未配置权限。");
}

QString accessText(bool allowed)
{
    if (allowed) {
        return QStringLiteral("可访问");
    }

    return QStringLiteral("无权限");
}

QString buttonText(bool allowed)
{
    if (allowed) {
        return QStringLiteral("进入");
    }

    return QStringLiteral("锁定");
}

}

MainWindow::MainWindow(const LoginResult &loginResult,
                       const LoginManager &loginManager,
                       QWidget *parent)
    : QMainWindow(parent)
    , m_loginResult(loginResult)
    , m_loginManager(&loginManager)
{
    setWindowTitle(QStringLiteral("火车票务管理系统"));
    resize(980, 640);

    // 主窗口的样式放在这里，颜色跟登录页保持一致。
    setStyleSheet(QStringLiteral(R"QSS(
        QMainWindow {
            background: #eef2f3;
            color: #1f2933;
            font-family: "Microsoft YaHei UI", "Microsoft YaHei", "Segoe UI";
            font-size: 14px;
        }
        QWidget#mainSurface {
            background: #eef2f3;
        }
        QFrame#headerPanel {
            background: #153832;
            border-radius: 14px;
        }
        QLabel#eyebrow {
            color: #a7f3d0;
            font-size: 12px;
            font-weight: 700;
            letter-spacing: 0px;
        }
        QLabel#mainTitle {
            color: #ffffff;
            font-size: 26px;
            font-weight: 700;
        }
        QLabel#mainSubtitle {
            color: #d5e7df;
            font-size: 13px;
        }
        QLabel#roleBadge {
            color: #153832;
            background: #d9f99d;
            border-radius: 12px;
            padding: 6px 12px;
            font-weight: 700;
        }
        QFrame#moduleCard {
            background: #fbfcfb;
            border: 1px solid #d8e0dc;
            border-radius: 12px;
        }
        QLabel#cardTitle {
            color: #17231f;
            font-size: 16px;
            font-weight: 700;
        }
        QLabel#cardDescription {
            color: #65716c;
            font-size: 13px;
        }
        QLabel#openTag {
            color: #14532d;
            background: #dcfce7;
            border-radius: 10px;
            padding: 4px 10px;
            font-size: 12px;
            font-weight: 700;
        }
        QLabel#lockedTag {
            color: #7f1d1d;
            background: #fee2e2;
            border-radius: 10px;
            padding: 4px 10px;
            font-size: 12px;
            font-weight: 700;
        }
        QPushButton {
            color: #ffffff;
            background: #176b5b;
            border: none;
            border-radius: 10px;
            padding: 8px 14px;
            font-weight: 700;
        }
        QPushButton:hover {
            background: #0f5749;
        }
        QPushButton:disabled {
            color: #8b9490;
            background: #d8e0dc;
        }
        QStatusBar {
            background: #fbfcfb;
            color: #33433d;
            border-top: 1px solid #d8e0dc;
        }
    )QSS"));

    auto *centralWidget = new QWidget(this);
    centralWidget->setObjectName(QStringLiteral("mainSurface"));

    auto *pageLayout = new QVBoxLayout(centralWidget);
    pageLayout->setContentsMargins(28, 28, 28, 24);
    pageLayout->setSpacing(22);

    // 顶部显示当前用户和身份。
    auto *headerPanel = new QFrame(centralWidget);
    headerPanel->setObjectName(QStringLiteral("headerPanel"));

    auto *headerLayout = new QHBoxLayout(headerPanel);
    headerLayout->setContentsMargins(26, 24, 26, 24);
    headerLayout->setSpacing(18);

    auto *titleBlock = new QVBoxLayout;
    titleBlock->setSpacing(6);

    auto *eyebrow = new QLabel(QStringLiteral("火车票务管理系统"), headerPanel);
    eyebrow->setObjectName(QStringLiteral("eyebrow"));

    auto *title = new QLabel(QStringLiteral("欢迎，%1").arg(m_loginResult.username), headerPanel);
    title->setObjectName(QStringLiteral("mainTitle"));

    auto *subtitle = new QLabel(roleHint(m_loginResult.role), headerPanel);
    subtitle->setObjectName(QStringLiteral("mainSubtitle"));

    titleBlock->addWidget(eyebrow);
    titleBlock->addWidget(title);
    titleBlock->addWidget(subtitle);

    auto *roleBadge = new QLabel(QStringLiteral("当前身份：%1").arg(roleName(m_loginResult.role)), headerPanel);
    roleBadge->setObjectName(QStringLiteral("roleBadge"));
    roleBadge->setAlignment(Qt::AlignCenter);

    headerLayout->addLayout(titleBlock, 1);
    headerLayout->addWidget(roleBadge, 0, Qt::AlignTop);

    const bool guestAllowed = LoginManager::canAccessGuestFunctions(m_loginResult.role);
    const bool sellerAllowed = LoginManager::canAccessSellerFunctions(m_loginResult.role);
    const bool adminAllowed = LoginManager::canAccessAdminFunctions(m_loginResult.role);
    const bool passwordAllowed = m_loginResult.role == UserRole::Admin
                                 || m_loginResult.role == UserRole::Seller;

    auto openAccountDialog = [this]() {
        if (m_loginManager == nullptr) {
            QMessageBox::warning(this,
                                 QStringLiteral("账号管理"),
                                 QStringLiteral("账号管理服务尚未连接。"));
            return;
        }

        AccountManagementDialog dialog(*m_loginManager, m_loginResult, this);
        dialog.exec();
    };

    // 下面的入口先做权限展示，真正的业务页面等对应模块完成后再接入。
    auto *gridLayout = new QGridLayout;
    gridLayout->setSpacing(16);

    auto *loginCard = new QFrame(centralWidget);
    loginCard->setObjectName(QStringLiteral("moduleCard"));
    auto *loginCardLayout = new QVBoxLayout(loginCard);
    loginCardLayout->setContentsMargins(18, 16, 18, 16);
    loginCardLayout->setSpacing(8);
    auto *loginTitleLabel = new QLabel(QStringLiteral("登录模块"), loginCard);
    loginTitleLabel->setObjectName(QStringLiteral("cardTitle"));
    auto *loginDescriptionLabel = new QLabel(QStringLiteral("当前会话和角色状态已准备就绪。"), loginCard);
    loginDescriptionLabel->setObjectName(QStringLiteral("cardDescription"));
    loginDescriptionLabel->setWordWrap(true);
    auto *loginTagLabel = new QLabel(QStringLiteral("已登录"), loginCard);
    loginTagLabel->setObjectName(QStringLiteral("openTag"));
    loginTagLabel->setAlignment(Qt::AlignCenter);
    auto *passwordButton = new QPushButton(passwordAllowed ? QStringLiteral("修改密码")
                                                           : QStringLiteral("游客无账号"), loginCard);
    passwordButton->setEnabled(passwordAllowed);
    connect(passwordButton, &QPushButton::clicked, this, openAccountDialog);
    loginCardLayout->addWidget(loginTitleLabel);
    loginCardLayout->addWidget(loginDescriptionLabel);
    loginCardLayout->addWidget(loginTagLabel, 0, Qt::AlignLeft);
    loginCardLayout->addStretch();
    loginCardLayout->addWidget(passwordButton, 0, Qt::AlignRight);

    auto *trainCard = new QFrame(centralWidget);
    trainCard->setObjectName(QStringLiteral("moduleCard"));
    auto *trainCardLayout = new QVBoxLayout(trainCard);
    trainCardLayout->setContentsMargins(18, 16, 18, 16);
    trainCardLayout->setSpacing(8);
    auto *trainTitleLabel = new QLabel(QStringLiteral("车票查询"), trainCard);
    trainTitleLabel->setObjectName(QStringLiteral("cardTitle"));
    auto *trainDescriptionLabel = new QLabel(QStringLiteral("游客、售票员和管理员都可以查看车次和余票信息。"), trainCard);
    trainDescriptionLabel->setObjectName(QStringLiteral("cardDescription"));
    trainDescriptionLabel->setWordWrap(true);
    auto *trainTagLabel = new QLabel(accessText(guestAllowed), trainCard);
    trainTagLabel->setObjectName(guestAllowed ? QStringLiteral("openTag") : QStringLiteral("lockedTag"));
    trainTagLabel->setAlignment(Qt::AlignCenter);
    auto *trainButton = new QPushButton(buttonText(guestAllowed), trainCard);
    trainButton->setEnabled(guestAllowed);
    connect(trainButton, &QPushButton::clicked, this, [this]() {
        QMessageBox::information(this,
                                 QStringLiteral("车票查询"),
                                 QStringLiteral("车票查询页面等待车次模块接入。"));
    });
    trainCardLayout->addWidget(trainTitleLabel);
    trainCardLayout->addWidget(trainDescriptionLabel);
    trainCardLayout->addWidget(trainTagLabel, 0, Qt::AlignLeft);
    trainCardLayout->addStretch();
    trainCardLayout->addWidget(trainButton, 0, Qt::AlignRight);

    auto *ticketCard = new QFrame(centralWidget);
    ticketCard->setObjectName(QStringLiteral("moduleCard"));
    auto *ticketCardLayout = new QVBoxLayout(ticketCard);
    ticketCardLayout->setContentsMargins(18, 16, 18, 16);
    ticketCardLayout->setSpacing(8);
    auto *ticketTitleLabel = new QLabel(QStringLiteral("票务办理"), ticketCard);
    ticketTitleLabel->setObjectName(QStringLiteral("cardTitle"));
    auto *ticketDescriptionLabel = new QLabel(QStringLiteral("订票、退票、改签和订单流程将在此接入。"), ticketCard);
    ticketDescriptionLabel->setObjectName(QStringLiteral("cardDescription"));
    ticketDescriptionLabel->setWordWrap(true);
    auto *ticketTagLabel = new QLabel(accessText(sellerAllowed), ticketCard);
    ticketTagLabel->setObjectName(sellerAllowed ? QStringLiteral("openTag") : QStringLiteral("lockedTag"));
    ticketTagLabel->setAlignment(Qt::AlignCenter);
    auto *ticketButton = new QPushButton(buttonText(sellerAllowed), ticketCard);
    ticketButton->setEnabled(sellerAllowed);
    connect(ticketButton, &QPushButton::clicked, this, [this]() {
        QMessageBox::information(this,
                                 QStringLiteral("票务办理"),
                                 QStringLiteral("票务办理页面等待票务模块接入。"));
    });
    ticketCardLayout->addWidget(ticketTitleLabel);
    ticketCardLayout->addWidget(ticketDescriptionLabel);
    ticketCardLayout->addWidget(ticketTagLabel, 0, Qt::AlignLeft);
    ticketCardLayout->addStretch();
    ticketCardLayout->addWidget(ticketButton, 0, Qt::AlignRight);

    auto *accountCard = new QFrame(centralWidget);
    accountCard->setObjectName(QStringLiteral("moduleCard"));
    auto *accountCardLayout = new QVBoxLayout(accountCard);
    accountCardLayout->setContentsMargins(18, 16, 18, 16);
    accountCardLayout->setSpacing(8);
    auto *accountTitleLabel = new QLabel(QStringLiteral("账号管理"), accountCard);
    accountTitleLabel->setObjectName(QStringLiteral("cardTitle"));
    auto *accountDescriptionLabel = new QLabel(QStringLiteral("管理员可进入后台管理入口，具体账号管理功能后续接入。"), accountCard);
    accountDescriptionLabel->setObjectName(QStringLiteral("cardDescription"));
    accountDescriptionLabel->setWordWrap(true);
    auto *accountTagLabel = new QLabel(accessText(adminAllowed), accountCard);
    accountTagLabel->setObjectName(adminAllowed ? QStringLiteral("openTag") : QStringLiteral("lockedTag"));
    accountTagLabel->setAlignment(Qt::AlignCenter);
    auto *accountButton = new QPushButton(buttonText(adminAllowed), accountCard);
    accountButton->setEnabled(adminAllowed);
    connect(accountButton, &QPushButton::clicked, this, openAccountDialog);
    accountCardLayout->addWidget(accountTitleLabel);
    accountCardLayout->addWidget(accountDescriptionLabel);
    accountCardLayout->addWidget(accountTagLabel, 0, Qt::AlignLeft);
    accountCardLayout->addStretch();
    accountCardLayout->addWidget(accountButton, 0, Qt::AlignRight);

    gridLayout->addWidget(loginCard, 0, 0);
    gridLayout->addWidget(trainCard, 0, 1);
    gridLayout->addWidget(ticketCard, 1, 0);
    gridLayout->addWidget(accountCard, 1, 1);

    pageLayout->addWidget(headerPanel);
    pageLayout->addLayout(gridLayout);
    pageLayout->addStretch();

    setCentralWidget(centralWidget);
    statusBar()->showMessage(QStringLiteral("当前登录身份：%1").arg(roleName(m_loginResult.role)));
}
