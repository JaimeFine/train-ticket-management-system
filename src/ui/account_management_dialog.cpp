#include "account_management_dialog.h"

#include <QAbstractItemView>
#include <QFormLayout>
#include <QGroupBox>
#include <QHeaderView>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QVBoxLayout>

namespace {
QString roleText(UserRole role)
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

    return QStringLiteral("未知身份");
}

void setupPasswordEdit(QLineEdit *lineEdit)
{
    lineEdit->setEchoMode(QLineEdit::Password);
    lineEdit->setClearButtonEnabled(true);
}

QString pageTitleText(UserRole role, bool accountOnly)
{
    if (role == UserRole::Guest) {
        return QStringLiteral("账户注册");
    }

    if (role == UserRole::Admin && !accountOnly) {
        return QStringLiteral("员工权限管理");
    }

    return QStringLiteral("我的账户");
}

QString pageHintText(const LoginResult &loginResult, bool accountOnly)
{
    if (loginResult.role == UserRole::Guest) {
        return QStringLiteral("注册普通用户账号后，可以登录使用完整功能。");
    }

    if (loginResult.role == UserRole::Admin && !accountOnly) {
        return QStringLiteral("管理售票员账号的创建、密码和启用状态。");
    }

    return QStringLiteral("当前用户：%1（%2）").arg(loginResult.username, roleText(loginResult.role));
}
}

AccountManagementDialog::AccountManagementDialog(const LoginManager &loginManager,
                                                 const LoginResult &loginResult,
                                                 QWidget *parent,
                                                 bool accountOnly)
    : QDialog(parent)
    , m_loginManager(loginManager)
    , m_loginResult(loginResult)
    , m_accountOnly(accountOnly)
{
    setWindowTitle(pageTitleText(m_loginResult.role, m_accountOnly));
    setModal(true);
    resize(720, 560);

    setStyleSheet(QStringLiteral(R"QSS(
        QDialog {
            background: #eef2f3;
            color: #1f2933;
            font-family: "Microsoft YaHei UI", "Microsoft YaHei", "Segoe UI";
            font-size: 14px;
        }
        QGroupBox {
            background: #fbfcfb;
            border: 1px solid #d8e0dc;
            border-radius: 10px;
            margin-top: 12px;
            padding: 12px;
            font-weight: 700;
        }
        QGroupBox::title {
            subcontrol-origin: margin;
            left: 12px;
            padding: 0 6px;
        }
        QLabel#pageTitle {
            color: #17231f;
            font-size: 22px;
            font-weight: 700;
        }
        QLabel#pageHint {
            color: #65716c;
        }
        QLabel#messageLabel {
            border-radius: 8px;
            padding: 8px 10px;
        }
        QLineEdit {
            min-height: 32px;
            border: 1px solid #cbd8d2;
            border-radius: 8px;
            padding: 4px 10px;
            background: #ffffff;
        }
        QLineEdit:focus {
            border: 2px solid #0f766e;
            padding: 3px 9px;
        }
        QTableWidget {
            background: #ffffff;
            border: 1px solid #cbd8d2;
            border-radius: 8px;
            gridline-color: #e5ece8;
            selection-background-color: #d9f99d;
            selection-color: #153832;
        }
        QHeaderView::section {
            background: #eef5f1;
            color: #33433d;
            border: none;
            padding: 7px;
            font-weight: 700;
        }
        QPushButton {
            min-height: 32px;
            border-radius: 8px;
            padding: 5px 14px;
            font-weight: 700;
            color: #ffffff;
            background: #176b5b;
            border: none;
        }
        QPushButton:hover {
            background: #0f5749;
        }
        QPushButton#dangerButton {
            background: #b91c1c;
        }
        QPushButton#dangerButton:hover {
            background: #991b1b;
        }
        QPushButton#logoutButton {
            color: #153832;
            background: #e5ece8;
        }
        QPushButton#logoutButton:hover {
            background: #d8e0dc;
        }
    )QSS"));

    auto *rootLayout = new QVBoxLayout(this);
    rootLayout->setContentsMargins(24, 22, 24, 22);
    rootLayout->setSpacing(14);

    auto *titleLabel = new QLabel(pageTitleText(m_loginResult.role, m_accountOnly), this);
    titleLabel->setObjectName(QStringLiteral("pageTitle"));

    auto *hintLabel = new QLabel(pageHintText(m_loginResult, m_accountOnly), this);
    hintLabel->setObjectName(QStringLiteral("pageHint"));

    m_messageLabel = new QLabel(QStringLiteral(" "), this);
    m_messageLabel->setObjectName(QStringLiteral("messageLabel"));
    m_messageLabel->setMinimumHeight(38);
    m_messageLabel->setWordWrap(true);

    rootLayout->addWidget(titleLabel);
    rootLayout->addWidget(hintLabel);
    rootLayout->addWidget(m_messageLabel);

    if (m_loginResult.role == UserRole::Admin && !m_accountOnly) {
        // 只有管理员能看到这一块。
        auto *createGroup = new QGroupBox(QStringLiteral("创建售票员账号"), this);
        auto *createLayout = new QVBoxLayout(createGroup);
        auto *createForm = new QFormLayout;
        createForm->setHorizontalSpacing(14);
        createForm->setVerticalSpacing(10);

        m_createUsernameEdit = new QLineEdit(createGroup);
        m_createUsernameEdit->setPlaceholderText(QStringLiteral("请输入新售票员用户名"));

        m_createPasswordEdit = new QLineEdit(createGroup);
        m_createPasswordEdit->setPlaceholderText(QStringLiteral("请输入初始密码"));
        setupPasswordEdit(m_createPasswordEdit);

        m_createConfirmEdit = new QLineEdit(createGroup);
        m_createConfirmEdit->setPlaceholderText(QStringLiteral("请再次输入初始密码"));
        setupPasswordEdit(m_createConfirmEdit);

        createForm->addRow(QStringLiteral("用户名"), m_createUsernameEdit);
        createForm->addRow(QStringLiteral("初始密码"), m_createPasswordEdit);
        createForm->addRow(QStringLiteral("确认密码"), m_createConfirmEdit);

        auto *createButton = new QPushButton(QStringLiteral("创建售票员"), createGroup);
        connect(createButton, &QPushButton::clicked, this, [this]() {
            handleCreateSeller();
        });

        createLayout->addLayout(createForm);
        createLayout->addWidget(createButton, 0, Qt::AlignRight);
        rootLayout->addWidget(createGroup);

        auto *manageGroup = new QGroupBox(QStringLiteral("现有售票员账号管理"), this);
        auto *manageLayout = new QVBoxLayout(manageGroup);

        m_sellerTable = new QTableWidget(manageGroup);
        m_sellerTable->setColumnCount(2);
        m_sellerTable->setHorizontalHeaderLabels({
            QStringLiteral("售票员用户名"),
            QStringLiteral("账号状态")
        });
        m_sellerTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
        m_sellerTable->setSelectionBehavior(QAbstractItemView::SelectRows);
        m_sellerTable->setSelectionMode(QAbstractItemView::SingleSelection);
        m_sellerTable->verticalHeader()->setVisible(false);
        m_sellerTable->horizontalHeader()->setStretchLastSection(true);
        m_sellerTable->setMinimumHeight(150);

        auto *manageButtonLayout = new QHBoxLayout;
        auto *resetButton = new QPushButton(QStringLiteral("重置为默认密码"), manageGroup);
        auto *enableButton = new QPushButton(QStringLiteral("启用账号"), manageGroup);
        auto *disableButton = new QPushButton(QStringLiteral("禁用账号"), manageGroup);
        disableButton->setObjectName(QStringLiteral("dangerButton"));

        connect(resetButton, &QPushButton::clicked, this, [this]() {
            handleResetSelectedSellerPassword();
        });
        connect(enableButton, &QPushButton::clicked, this, [this]() {
            handleSetSelectedSellerEnabled(true);
        });
        connect(disableButton, &QPushButton::clicked, this, [this]() {
            handleSetSelectedSellerEnabled(false);
        });

        manageButtonLayout->addStretch();
        manageButtonLayout->addWidget(resetButton);
        manageButtonLayout->addWidget(enableButton);
        manageButtonLayout->addWidget(disableButton);

        manageLayout->addWidget(m_sellerTable);
        manageLayout->addLayout(manageButtonLayout);
        rootLayout->addWidget(manageGroup);

        refreshSellerTable();
    }

    if ((m_loginResult.role == UserRole::Admin && m_accountOnly)
        || m_loginResult.role == UserRole::Seller
        || m_loginResult.role == UserRole::User) {
        // 真实账号可以改自己的密码。
        auto *ownGroup = new QGroupBox(QStringLiteral("修改当前账号密码"), this);
        auto *ownLayout = new QVBoxLayout(ownGroup);
        auto *ownForm = new QFormLayout;
        ownForm->setHorizontalSpacing(14);
        ownForm->setVerticalSpacing(10);

        m_oldPasswordEdit = new QLineEdit(ownGroup);
        m_oldPasswordEdit->setPlaceholderText(QStringLiteral("请输入原密码"));
        setupPasswordEdit(m_oldPasswordEdit);

        m_newPasswordEdit = new QLineEdit(ownGroup);
        m_newPasswordEdit->setPlaceholderText(QStringLiteral("请输入新密码"));
        setupPasswordEdit(m_newPasswordEdit);

        m_newConfirmEdit = new QLineEdit(ownGroup);
        m_newConfirmEdit->setPlaceholderText(QStringLiteral("请再次输入新密码"));
        setupPasswordEdit(m_newConfirmEdit);

        ownForm->addRow(QStringLiteral("原密码"), m_oldPasswordEdit);
        ownForm->addRow(QStringLiteral("新密码"), m_newPasswordEdit);
        ownForm->addRow(QStringLiteral("确认新密码"), m_newConfirmEdit);

        auto *changeButton = new QPushButton(QStringLiteral("修改密码"), ownGroup);
        connect(changeButton, &QPushButton::clicked, this, [this]() {
            handleChangeOwnPassword();
        });

        ownLayout->addLayout(ownForm);
        ownLayout->addWidget(changeButton, 0, Qt::AlignRight);
        rootLayout->addWidget(ownGroup);
    }

    if (m_loginResult.role == UserRole::Guest) {
        auto *registerGroup = new QGroupBox(QStringLiteral("注册普通用户账号"), this);
        auto *registerLayout = new QVBoxLayout(registerGroup);
        auto *registerForm = new QFormLayout;
        registerForm->setHorizontalSpacing(14);
        registerForm->setVerticalSpacing(10);

        m_registerUsernameEdit = new QLineEdit(registerGroup);
        m_registerUsernameEdit->setPlaceholderText(QStringLiteral("请输入用户名"));

        m_registerPasswordEdit = new QLineEdit(registerGroup);
        m_registerPasswordEdit->setPlaceholderText(QStringLiteral("请输入密码"));
        setupPasswordEdit(m_registerPasswordEdit);

        m_registerConfirmEdit = new QLineEdit(registerGroup);
        m_registerConfirmEdit->setPlaceholderText(QStringLiteral("请再次输入密码"));
        setupPasswordEdit(m_registerConfirmEdit);

        registerForm->addRow(QStringLiteral("用户名"), m_registerUsernameEdit);
        registerForm->addRow(QStringLiteral("密码"), m_registerPasswordEdit);
        registerForm->addRow(QStringLiteral("确认密码"), m_registerConfirmEdit);

        auto *registerButton = new QPushButton(QStringLiteral("注册账号"), registerGroup);
        connect(registerButton, &QPushButton::clicked, this, [this]() {
            handleRegisterUser();
        });

        registerLayout->addLayout(registerForm);
        registerLayout->addWidget(registerButton, 0, Qt::AlignRight);
        rootLayout->addWidget(registerGroup);
    }

    rootLayout->addStretch();

    auto *bottomLayout = new QHBoxLayout;
    if (m_accountOnly && m_loginResult.role != UserRole::Guest) {
        auto *logoutButton = new QPushButton(QStringLiteral("退出登录"), this);
        logoutButton->setObjectName(QStringLiteral("logoutButton"));
        connect(logoutButton, &QPushButton::clicked, this, [this]() {
            handleLogout();
        });
        bottomLayout->addWidget(logoutButton);
    }

    bottomLayout->addStretch();

    auto *closeButton = new QPushButton(QStringLiteral("关闭"), this);
    connect(closeButton, &QPushButton::clicked, this, [this]() {
        close();
    });
    bottomLayout->addWidget(closeButton);
    rootLayout->addLayout(bottomLayout);
}

bool AccountManagementDialog::logoutRequested() const
{
    return m_logoutRequested;
}

void AccountManagementDialog::handleRegisterUser()
{
    if (m_registerPasswordEdit->text() != m_registerConfirmEdit->text()) {
        showPlainMessage(false, QStringLiteral("两次输入的密码不一致。"));
        return;
    }

    const AccountResult result =
        m_loginManager.registerUser(m_registerUsernameEdit->text(),
                                    m_registerPasswordEdit->text());
    showMessage(result);

    if (result.success) {
        m_registerUsernameEdit->clear();
        m_registerPasswordEdit->clear();
        m_registerConfirmEdit->clear();
    }
}

void AccountManagementDialog::handleCreateSeller()
{
    if (m_createPasswordEdit->text() != m_createConfirmEdit->text()) {
        showPlainMessage(false, QStringLiteral("两次输入的密码不一致。"));
        return;
    }

    const AccountResult result =
        m_loginManager.createSellerAccount(m_loginResult.role,
                                           m_createUsernameEdit->text(),
                                           m_createPasswordEdit->text());
    showMessage(result);

    if (result.success) {
        m_createUsernameEdit->clear();
        m_createPasswordEdit->clear();
        m_createConfirmEdit->clear();
        refreshSellerTable();
    }
}

void AccountManagementDialog::handleResetSelectedSellerPassword()
{
    const QString username = selectedSellerUsername();

    if (username.isEmpty()) {
        showPlainMessage(false, QStringLiteral("请先选中一个售票员账号。"));
        return;
    }

    const AccountResult result =
        m_loginManager.resetSellerPasswordToDefault(m_loginResult.role, username);
    showMessage(result);
}

void AccountManagementDialog::handleSetSelectedSellerEnabled(bool enabled)
{
    // 账号管理按钮都以表格当前选中的行为准。这里先取出用户名，
    // 再把“启用还是禁用”交给 LoginManager，成功后重新读取列表刷新状态。
    const QString username = selectedSellerUsername();

    if (username.isEmpty()) {
        showPlainMessage(false, QStringLiteral("请先选中一个售票员账号。"));
        return;
    }

    const AccountResult result =
        m_loginManager.setSellerEnabled(m_loginResult.role,
                                        username,
                                        enabled);
    showMessage(result);

    if (result.success) {
        refreshSellerTable();
    }
}

void AccountManagementDialog::handleChangeOwnPassword()
{
    if (m_newPasswordEdit->text() != m_newConfirmEdit->text()) {
        showPlainMessage(false, QStringLiteral("两次输入的新密码不一致。"));
        return;
    }

    const AccountResult result =
        m_loginManager.changeOwnPassword(m_loginResult.username,
                                         m_loginResult.role,
                                         m_oldPasswordEdit->text(),
                                         m_newPasswordEdit->text());
    showMessage(result);

    if (result.success) {
        m_oldPasswordEdit->clear();
        m_newPasswordEdit->clear();
        m_newConfirmEdit->clear();
    }
}

void AccountManagementDialog::handleLogout()
{
    m_logoutRequested = true;
    accept();
}

void AccountManagementDialog::showMessage(const AccountResult &result)
{
    showPlainMessage(result.success, result.message);
}

void AccountManagementDialog::refreshSellerTable()
{
    if (m_sellerTable == nullptr) {
        return;
    }

    // 列表数据每次都重新向 LoginManager 获取，这样创建、启用或禁用账号后，
    // 页面显示的就是数据库里的最新状态。界面只负责把结果逐行放进表格。
    const QList<SellerAccountInfo> sellers =
        m_loginManager.sellerAccounts(m_loginResult.role);

    m_sellerTable->setRowCount(sellers.size());

    for (int row = 0; row < sellers.size(); ++row) {
        const SellerAccountInfo &seller = sellers.at(row);

        auto *usernameItem = new QTableWidgetItem(seller.username);
        auto *statusItem = new QTableWidgetItem(seller.enabled
                                                    ? QStringLiteral("启用中")
                                                    : QStringLiteral("已禁用"));

        usernameItem->setTextAlignment(Qt::AlignVCenter | Qt::AlignLeft);
        statusItem->setTextAlignment(Qt::AlignCenter);

        m_sellerTable->setItem(row, 0, usernameItem);
        m_sellerTable->setItem(row, 1, statusItem);
    }

    m_sellerTable->resizeRowsToContents();
}

QString AccountManagementDialog::selectedSellerUsername() const
{
    if (m_sellerTable == nullptr || m_sellerTable->currentRow() < 0) {
        return QString();
    }

    QTableWidgetItem *item = m_sellerTable->item(m_sellerTable->currentRow(), 0);

    if (item == nullptr) {
        return QString();
    }

    return item->text();
}

void AccountManagementDialog::showPlainMessage(bool success, const QString &message)
{
    m_messageLabel->setText(message);

    if (success) {
        m_messageLabel->setStyleSheet(QStringLiteral(
            "color: #14532d; background: #dcfce7; border: 1px solid #86efac;"));
        return;
    }

    m_messageLabel->setStyleSheet(QStringLiteral(
        "color: #9f1239; background: #fff1f2; border: 1px solid #fecdd3;"));
}
