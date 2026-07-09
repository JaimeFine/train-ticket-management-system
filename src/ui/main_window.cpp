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
    case UserRole::User:
        return QStringLiteral("普通用户");
    }

    return QStringLiteral("未知角色");
}

QString roleHint(UserRole role)
{
    switch (role) {
    case UserRole::Admin:
        return QStringLiteral("账号管理和权限维护是当前工作重点。");
    case UserRole::Seller:
        return QStringLiteral("优先处理售票、退票、改签和车票查询。");
    case UserRole::Guest:
        return QStringLiteral("游客模式只保留基础查询入口。");
    case UserRole::User:
        return QStringLiteral("查看车票信息，并维护自己的账号密码。");
    }

    return QStringLiteral("当前身份暂未配置权限。");
}

QString workspaceTitle(UserRole role)
{
    switch (role) {
    case UserRole::Admin:
        return QStringLiteral("管理员工作台");
    case UserRole::Seller:
        return QStringLiteral("售票员工作台");
    case UserRole::Guest:
        return QStringLiteral("游客查询入口");
    case UserRole::User:
        return QStringLiteral("用户服务台");
    }

    return QStringLiteral("系统工作台");
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

    auto *title = new QLabel(workspaceTitle(m_loginResult.role), headerPanel);
    title->setObjectName(QStringLiteral("mainTitle"));

    auto *subtitle = new QLabel(QStringLiteral("%1，%2")
                                    .arg(m_loginResult.username, roleHint(m_loginResult.role)),
                                headerPanel);
    subtitle->setObjectName(QStringLiteral("mainSubtitle"));

    titleBlock->addWidget(eyebrow);
    titleBlock->addWidget(title);
    titleBlock->addWidget(subtitle);

    auto *roleBadge = new QLabel(QStringLiteral("当前身份：%1").arg(roleName(m_loginResult.role)), headerPanel);
    roleBadge->setObjectName(QStringLiteral("roleBadge"));
    roleBadge->setAlignment(Qt::AlignCenter);

    headerLayout->addLayout(titleBlock, 1);
    headerLayout->addWidget(roleBadge, 0, Qt::AlignTop);

    const bool sellerAllowed = LoginManager::canAccessSellerFunctions(m_loginResult.role);
    const bool adminAllowed = LoginManager::canAccessAdminFunctions(m_loginResult.role);
    const bool passwordAllowed = m_loginResult.role == UserRole::Admin
                                 || m_loginResult.role == UserRole::Seller
                                 || m_loginResult.role == UserRole::User;

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

    auto showQueryMessage = [this]() {
        QMessageBox::information(this,
                                 QStringLiteral("车票查询"),
                                 QStringLiteral("车票查询页面暂未开放。"));
    };

    auto showTicketMessage = [this]() {
        QMessageBox::information(this,
                                 QStringLiteral("票务办理"),
                                 QStringLiteral("票务办理页面暂未开放。"));
    };

    // 下面按身份生成工作台，避免普通用户看到管理员才需要的入口。
    auto *gridLayout = new QGridLayout;
    gridLayout->setSpacing(16);
    gridLayout->setColumnStretch(0, 1);
    gridLayout->setColumnStretch(1, 1);

    int cardCount = 0;
    auto addModuleCard = [&](const QString &titleText,
                             const QString &descriptionText,
                             const QString &tagText,
                             const QString &buttonText,
                             bool buttonEnabled,
                             auto onClicked) {
        auto *card = new QFrame(centralWidget);
        card->setObjectName(QStringLiteral("moduleCard"));

        auto *cardLayout = new QVBoxLayout(card);
        cardLayout->setContentsMargins(18, 16, 18, 16);
        cardLayout->setSpacing(8);

        auto *titleLabel = new QLabel(titleText, card);
        titleLabel->setObjectName(QStringLiteral("cardTitle"));

        auto *descriptionLabel = new QLabel(descriptionText, card);
        descriptionLabel->setObjectName(QStringLiteral("cardDescription"));
        descriptionLabel->setWordWrap(true);

        auto *tagLabel = new QLabel(tagText, card);
        tagLabel->setObjectName(buttonEnabled ? QStringLiteral("openTag")
                                              : QStringLiteral("lockedTag"));
        tagLabel->setAlignment(Qt::AlignCenter);

        auto *button = new QPushButton(buttonText, card);
        button->setEnabled(buttonEnabled);
        connect(button, &QPushButton::clicked, this, onClicked);

        cardLayout->addWidget(titleLabel);
        cardLayout->addWidget(descriptionLabel);
        cardLayout->addWidget(tagLabel, 0, Qt::AlignLeft);
        cardLayout->addStretch();
        cardLayout->addWidget(button, 0, Qt::AlignRight);

        gridLayout->addWidget(card, cardCount / 2, cardCount % 2);
        ++cardCount;
    };

    if (adminAllowed) {
        addModuleCard(QStringLiteral("账号管理"),
                      QStringLiteral("创建售票员账号、重置密码、启用或禁用账号。"),
                      QStringLiteral("管理员专属"),
                      QStringLiteral("进入管理"),
                      true,
                      openAccountDialog);
    }

    if (sellerAllowed) {
        addModuleCard(QStringLiteral("票务办理"),
                      QStringLiteral("处理售票、退票、改签等柜台业务。"),
                      QStringLiteral("业务办理"),
                      QStringLiteral("进入办理"),
                      true,
                      showTicketMessage);
    }

    addModuleCard(QStringLiteral("车票查询"),
                  QStringLiteral("查看车次、站点和余票信息。"),
                  QStringLiteral("查询开放"),
                  QStringLiteral("进入查询"),
                  true,
                  showQueryMessage);

    if (passwordAllowed) {
        addModuleCard(QStringLiteral("我的账号"),
                      QStringLiteral("修改当前登录账号的密码。"),
                      QStringLiteral("个人设置"),
                      QStringLiteral("修改密码"),
                      true,
                      openAccountDialog);
    } else {
        addModuleCard(QStringLiteral("游客模式"),
                      QStringLiteral("可以先查询车票信息，登录账号后再使用个人服务。"),
                      QStringLiteral("基础访问"),
                      QStringLiteral("请先登录"),
                      false,
                      []() {});
    }

    pageLayout->addWidget(headerPanel);
    pageLayout->addLayout(gridLayout);
    pageLayout->addStretch();

    setCentralWidget(centralWidget);
    statusBar()->showMessage(QStringLiteral("当前登录身份：%1").arg(roleName(m_loginResult.role)));
}
