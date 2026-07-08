#include "main_window.h"

#include <QFrame>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
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

}

MainWindow::MainWindow(const LoginResult &loginResult, QWidget *parent)
    : QMainWindow(parent)
    , m_loginResult(loginResult)
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

    auto *subtitle = new QLabel(QStringLiteral("当前为系统首页，各业务模块将在集成后继续接入。"), headerPanel);
    subtitle->setObjectName(QStringLiteral("mainSubtitle"));

    titleBlock->addWidget(eyebrow);
    titleBlock->addWidget(title);
    titleBlock->addWidget(subtitle);

    auto *roleBadge = new QLabel(QStringLiteral("当前身份：%1").arg(roleName(m_loginResult.role)), headerPanel);
    roleBadge->setObjectName(QStringLiteral("roleBadge"));
    roleBadge->setAlignment(Qt::AlignCenter);

    headerLayout->addLayout(titleBlock, 1);
    headerLayout->addWidget(roleBadge, 0, Qt::AlignTop);

    // 下面四个卡片先手动写出来，后面其他模块做好后再接功能。
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
    loginCardLayout->addWidget(loginTitleLabel);
    loginCardLayout->addWidget(loginDescriptionLabel);
    loginCardLayout->addStretch();

    auto *trainCard = new QFrame(centralWidget);
    trainCard->setObjectName(QStringLiteral("moduleCard"));
    auto *trainCardLayout = new QVBoxLayout(trainCard);
    trainCardLayout->setContentsMargins(18, 16, 18, 16);
    trainCardLayout->setSpacing(8);
    auto *trainTitleLabel = new QLabel(QStringLiteral("车次管理"), trainCard);
    trainTitleLabel->setObjectName(QStringLiteral("cardTitle"));
    auto *trainDescriptionLabel = new QLabel(QStringLiteral("车次、站点和时刻表功能将在此接入。"), trainCard);
    trainDescriptionLabel->setObjectName(QStringLiteral("cardDescription"));
    trainDescriptionLabel->setWordWrap(true);
    trainCardLayout->addWidget(trainTitleLabel);
    trainCardLayout->addWidget(trainDescriptionLabel);
    trainCardLayout->addStretch();

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
    ticketCardLayout->addWidget(ticketTitleLabel);
    ticketCardLayout->addWidget(ticketDescriptionLabel);
    ticketCardLayout->addStretch();

    auto *statisticsCard = new QFrame(centralWidget);
    statisticsCard->setObjectName(QStringLiteral("moduleCard"));
    auto *statisticsCardLayout = new QVBoxLayout(statisticsCard);
    statisticsCardLayout->setContentsMargins(18, 16, 18, 16);
    statisticsCardLayout->setSpacing(8);
    auto *statisticsTitleLabel = new QLabel(QStringLiteral("统计分析"), statisticsCard);
    statisticsTitleLabel->setObjectName(QStringLiteral("cardTitle"));
    auto *statisticsDescriptionLabel = new QLabel(QStringLiteral("销售统计和报表汇总功能将在此接入。"), statisticsCard);
    statisticsDescriptionLabel->setObjectName(QStringLiteral("cardDescription"));
    statisticsDescriptionLabel->setWordWrap(true);
    statisticsCardLayout->addWidget(statisticsTitleLabel);
    statisticsCardLayout->addWidget(statisticsDescriptionLabel);
    statisticsCardLayout->addStretch();

    gridLayout->addWidget(loginCard, 0, 0);
    gridLayout->addWidget(trainCard, 0, 1);
    gridLayout->addWidget(ticketCard, 1, 0);
    gridLayout->addWidget(statisticsCard, 1, 1);

    pageLayout->addWidget(headerPanel);
    pageLayout->addLayout(gridLayout);
    pageLayout->addStretch();

    setCentralWidget(centralWidget);
    statusBar()->showMessage(QStringLiteral("当前登录身份：%1").arg(roleName(m_loginResult.role)));
}
