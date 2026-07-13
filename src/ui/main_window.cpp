#include "account_management_dialog.h"
#include "order_history_dialog.h"
#include "statistics_dialog.h"
#include "ticket_manager.h"
#include "ticket_service_dialog.h"
#include "train_management_dialog.h"
#include "main_window.h"

#include <QFrame>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QMessageBox>
#include <QPixmap>
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

QString greetingText(UserRole role)
{
    switch (role) {
    case UserRole::Admin:
        return QStringLiteral("祝你今天的工作一切顺利");
    case UserRole::Seller:
        return QStringLiteral("祝你今天的工作一切顺利");
    case UserRole::Guest:
        return QStringLiteral("使用完整功能请尽快登录");
    case UserRole::User:
        return QStringLiteral("祝您旅途愉快，一路顺风");
    }

    return QStringLiteral("欢迎使用火车票务管理系统");
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
                       TrainManager* trainManager,
                       TicketManager *ticketManager,
                       StatisticsManager *statisticsManager,
                       QWidget *parent)
    : QMainWindow(parent)
    , m_loginResult(loginResult)
    , m_loginManager(&loginManager)
    , m_trainManager(trainManager)
    , m_ticketManager(ticketManager)
    , m_statisticsManager(statisticsManager)
{
    setWindowTitle(QStringLiteral("火车票务管理系统"));
    resize(980, 640);

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
            color: #e6f4ee;
            font-size: 13px;
        }
        QLabel#roleBadge {
            color: #153832;
            background: #d9f99d;
            border-radius: 12px;
            padding: 6px 12px;
            font-weight: 700;
        }
        QPushButton#accountButton {
            color: #153832;
            background: #eefee0;
            border: 1px solid #d9f99d;
            border-radius: 10px;
            padding: 6px 12px;
            font-weight: 700;
        }
        QPushButton#accountButton:hover {
            background: #d9f99d;
        }
        QPushButton#logoutButton {
            color: #ffffff;
            background: #176b5b;
            border: none;
            border-radius: 10px;
            padding: 6px 12px;
            font-weight: 700;
        }
        QPushButton#logoutButton:hover {
            background: #0f5749;
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
            color: #42514b;
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

    // “我的账户”和“员工权限管理”共用这个窗口。accountOnly 为 true 时只显示
    // 当前账号自己的功能；管理员从工作台进入时传 false，才显示售票员管理区域。
    auto openAccountDialog = [this](bool accountOnly) {
        if (m_loginManager == nullptr) {
            QMessageBox::warning(this,
                                 QStringLiteral("我的账户"),
                                 QStringLiteral("账号服务尚未连接。"));
            return;
        }

        AccountManagementDialog dialog(*m_loginManager, m_loginResult, this, accountOnly);
        dialog.exec();
        if (dialog.logoutRequested()) {
            m_logoutRequested = true;
            close();
        }
    };

    // 顶部放身份和我的账户入口。
    auto *headerPanel = new QFrame(centralWidget);
    headerPanel->setObjectName(QStringLiteral("headerPanel"));

    auto *headerLayout = new QHBoxLayout(headerPanel);
    headerLayout->setContentsMargins(26, 24, 26, 24);
    headerLayout->setSpacing(18);

    auto *titleBlock = new QVBoxLayout;
    titleBlock->setSpacing(6);

    auto *logoLabel = new QLabel(headerPanel);
    logoLabel->setFixedSize(96, 96);
    logoLabel->setAlignment(Qt::AlignCenter);
    const QPixmap logoPixmap(QStringLiteral(":/train.jpg"));
    if (!logoPixmap.isNull()) {
        logoLabel->setPixmap(logoPixmap.scaled(96,
                                               96,
                                               Qt::KeepAspectRatio,
                                               Qt::SmoothTransformation));
    }

    auto *eyebrow = new QLabel(QStringLiteral("火车票务管理系统"), headerPanel);
    eyebrow->setObjectName(QStringLiteral("eyebrow"));

    auto *title = new QLabel(workspaceTitle(m_loginResult.role), headerPanel);
    title->setObjectName(QStringLiteral("mainTitle"));

    auto *subtitle = new QLabel(greetingText(m_loginResult.role), headerPanel);
    subtitle->setObjectName(QStringLiteral("mainSubtitle"));

    titleBlock->addWidget(eyebrow);
    titleBlock->addWidget(title);
    titleBlock->addWidget(subtitle);

    auto *roleBadge = new QLabel(QStringLiteral("当前身份：%1").arg(roleName(m_loginResult.role)), headerPanel);
    roleBadge->setObjectName(QStringLiteral("roleBadge"));
    roleBadge->setAlignment(Qt::AlignCenter);

    auto *accountButton = new QPushButton(m_loginResult.role == UserRole::Guest
                                              ? QStringLiteral("注册账号")
                                              : QStringLiteral("我的账户"),
                                          headerPanel);
    accountButton->setObjectName(QStringLiteral("accountButton"));
    connect(accountButton, &QPushButton::clicked, this, [openAccountDialog]() {
        openAccountDialog(true);
    });

    auto *accountBlock = new QVBoxLayout;
    accountBlock->setSpacing(8);
    accountBlock->addWidget(roleBadge);
    accountBlock->addWidget(accountButton);
    if (m_loginResult.role != UserRole::Guest) {
        auto *logoutButton = new QPushButton(QStringLiteral("退出登录"), headerPanel);
        logoutButton->setObjectName(QStringLiteral("logoutButton"));
        connect(logoutButton, &QPushButton::clicked, this, [this]() {
            m_logoutRequested = true;
            close();
        });
        accountBlock->addWidget(logoutButton);
    }

    headerLayout->addWidget(logoLabel);
    headerLayout->addLayout(titleBlock, 1);
    headerLayout->addLayout(accountBlock);

    auto showTrainStationMessage = [this]() {
        qDebug() << "=== showTrainStationMessage called ===";
        if (m_trainManager == nullptr) {
            qDebug() << "m_trainManager is NULL!";
            QMessageBox::warning(this,
                                 QStringLiteral("车次管理"),
                                 QStringLiteral("车次管理服务尚未初始化。"));
            return;
        }
        qDebug() << "Creating TrainManagementDialog...";
        TrainManagementDialog dialog(m_trainManager, this);
        qDebug() << "Executing TrainManagementDialog...";
        dialog.exec();
        qDebug() << "TrainManagementDialog closed.";
    };

    auto showStatisticsDialog = [this]() {
        if (m_statisticsManager == nullptr) {
            QMessageBox::warning(this,
                                 QStringLiteral("票务数据统计"),
                                 QStringLiteral("统计服务尚未初始化。"));
            return;
        }

        StatisticsDialog dialog(*m_statisticsManager, this);
        dialog.exec();
    };

    auto showOrderHistoryDialog = [this]() {
        if (m_ticketManager == nullptr) {
            QMessageBox::warning(this,
                                 QStringLiteral("我的订单"),
                                 QStringLiteral("订单服务尚未初始化。"));
            return;
        }

        OrderHistoryDialog dialog(*m_ticketManager, m_loginResult.userId, this);
        dialog.exec();
    };

    auto openTicketServiceDialog = [this](int initialTabIndex) {
        if (m_ticketManager == nullptr) {
            QMessageBox::warning(this,
                                 QStringLiteral("票务服务中心"),
                                 QStringLiteral("票务服务尚未初始化。"));
            return;
        }

        TicketServiceDialog dialog(*m_ticketManager, m_loginResult, initialTabIndex, this);
        dialog.exec();
    };
    // 工作台卡片的排版都一样，所以集中在这里创建。点击后的函数由调用处传入，
    // 以后车次、票务模块接入时，只需要把现在的提示函数换成真正的窗口入口，
    // 不用重新改整套主界面布局。
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

    // 权限统一交给 LoginManager 判断。这里从高权限往低权限依次匹配，
    // 管理员命中后就不会再进入售票员工作台，主窗口只负责生成对应界面。
    if (LoginManager::canAccessAdminFunctions(m_loginResult.role)) {
        addModuleCard(QStringLiteral("票务数据统计"),
                      QStringLiteral("查看订单总览、热门路线和月度客流。"),
                      QStringLiteral("统计中心"),
                      QStringLiteral("查看统计"),
                      true,
                      showStatisticsDialog);

        addModuleCard(QStringLiteral("员工权限管理"),
                      QStringLiteral("创建售票员账号，并管理现有售票员账号。"),
                      QStringLiteral("员工账号"),
                      QStringLiteral("进入管理"),
                      true,
                      [openAccountDialog]() {
                          openAccountDialog(false);
                      });

        addModuleCard(QStringLiteral("车次站点管理"),
                      QStringLiteral("维护车次、站点、发到时间和座位库存。"),
                      QStringLiteral("车次站点"),
                      QStringLiteral("进入管理"),
                      true,
                      showTrainStationMessage);
    } else if (m_loginResult.role == UserRole::User) {
        addModuleCard(QStringLiteral("车票查询"),
                      QStringLiteral("查询车次、余票，并可直接预订选中的车次。"),
                      QStringLiteral("查询开放"),
                      QStringLiteral("进入查询"),
                      true,
                      [openTicketServiceDialog]() {
                          openTicketServiceDialog(0);
                      });

        addModuleCard(QStringLiteral("票务管理"),
                      QStringLiteral("办理退票和改签。"),
                      QStringLiteral("票务操作"),
                      QStringLiteral("进入办理"),
                      true,
                      [openTicketServiceDialog]() {
                          openTicketServiceDialog(1);
                      });

        addModuleCard(QStringLiteral("我的账户"),
                      QStringLiteral("修改当前账号密码，或退出后重新登录。"),
                      QStringLiteral("账户中心"),
                      QStringLiteral("进入账户"),
                      true,
                      [openAccountDialog]() {
                          openAccountDialog(true);
                      });

        addModuleCard(QStringLiteral("我的订单"),
                      QStringLiteral("查看当前账号已经创建的订单记录。"),
                      QStringLiteral("订单历史"),
                      QStringLiteral("查看订单"),
                      true,
                      [openTicketServiceDialog]() {
                          openTicketServiceDialog(2);
                      });
    } else if (m_loginResult.role == UserRole::Seller) {
        addModuleCard(QStringLiteral("车票查询"),
                      QStringLiteral("查询车次、余票，并可协助乘客订票。"),
                      QStringLiteral("查询开放"),
                      QStringLiteral("进入查询"),
                      true,
                      [openTicketServiceDialog]() {
                          openTicketServiceDialog(0);
                      });

        addModuleCard(QStringLiteral("票务管理"),
                      QStringLiteral("办理订票、退票和改签。"),
                      QStringLiteral("业务办理"),
                      QStringLiteral("进入办理"),
                      true,
                      [openTicketServiceDialog]() {
                          openTicketServiceDialog(1);
                      });

        addModuleCard(QStringLiteral("票务查询"),
                      QStringLiteral("按订单号、乘客姓名或全部记录查询票务信息。"),
                      QStringLiteral("业务查询"),
                      QStringLiteral("进入查询"),
                      true,
                      [openTicketServiceDialog]() {
                          openTicketServiceDialog(2);
                      });

        addModuleCard(QStringLiteral("我的账户"),
                      QStringLiteral("修改当前账号密码，或退出后重新登录。"),
                      QStringLiteral("账户中心"),
                      QStringLiteral("进入账户"),
                      true,
                      [openAccountDialog]() {
                          openAccountDialog(true);
                      });
    } else if (m_loginResult.role == UserRole::Guest) {
        addModuleCard(QStringLiteral("车票查询"),
                      QStringLiteral("先查询车次和余票，决定是否注册后继续订票。"),
                      QStringLiteral("查询开放"),
                      QStringLiteral("进入查询"),
                      true,
                      [openTicketServiceDialog]() {
                          openTicketServiceDialog(0);
                      });

        addModuleCard(QStringLiteral("注册普通用户账号"),
                      QStringLiteral("注册后即可使用完整用户功能。"),
                      QStringLiteral("注册入口"),
                      QStringLiteral("立即注册"),
                      true,
                      [openAccountDialog]() {
                          openAccountDialog(true);
                      });
    }

    pageLayout->addWidget(headerPanel);
    if (cardCount > 0) {
        pageLayout->addLayout(gridLayout);
    } else {
        auto *emptyStateLabel = new QLabel(QStringLiteral("当前账号可用的工作台入口将随对应模块完成后接入。"),
                                           centralWidget);
        emptyStateLabel->setWordWrap(true);
        emptyStateLabel->setAlignment(Qt::AlignCenter);
        emptyStateLabel->setStyleSheet(QStringLiteral(
            "color: #52615b;"
            "background: #fbfcfb;"
            "border: 1px solid #d8e0dc;"
            "border-radius: 12px;"
            "padding: 24px;"
            "font-size: 14px;"));
        pageLayout->addWidget(emptyStateLabel);
    }
    pageLayout->addStretch();

    setCentralWidget(centralWidget);
    statusBar()->showMessage(QStringLiteral("当前登录身份：%1").arg(roleName(m_loginResult.role)));
}

bool MainWindow::logoutRequested() const
{
    return m_logoutRequested;
}
