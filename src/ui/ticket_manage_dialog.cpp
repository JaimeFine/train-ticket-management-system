/**
 * @file ticket_manage_dialog.cpp - Issue 9+10: 订票+退票+改签
 */
#include "ticket_manage_dialog.h"
#include "database_manager.h"
#include "ticket_manager.h"

#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QVBoxLayout>

TicketManageDialog::TicketManageDialog(DatabaseManager &db, int userId,
                                       QWidget *parent)
    : QDialog(parent), m_db(db), m_userId(userId)
{
    setWindowTitle(QStringLiteral("票务管理"));
    setModal(true);
    resize(600, 520);

    setStyleSheet(QStringLiteral(R"QSS(
        QDialog { background:#eef2f3; color:#1f2933;
            font-family:"Microsoft YaHei UI","Microsoft YaHei","Segoe UI"; font-size:14px; }
        QGroupBox { background:#fbfcfb; border:1px solid #d8e0dc; border-radius:10px;
            margin-top:12px; padding:12px; font-weight:700; }
        QGroupBox::title { subcontrol-origin:margin; left:12px; padding:0 6px; }
        QLabel#pageTitle { color:#17231f; font-size:22px; font-weight:700; }
        QLabel#pageHint { color:#65716c; }
        QLineEdit { min-height:32px; border:1px solid #cbd8d2; border-radius:8px;
            padding:4px 10px; background:#ffffff; }
        QLineEdit:focus { border:2px solid #0f766e; padding:3px 9px; }
        QPushButton { min-height:32px; border-radius:8px; padding:5px 14px; font-weight:700;
            color:#ffffff; background:#176b5b; border:none; }
        QPushButton:hover { background:#0f5749; }
        QPushButton#dangerBtn { background:#b91c1c; }
        QPushButton#dangerBtn:hover { background:#991b1b; }
    )QSS"));

    auto *root = new QVBoxLayout(this);
    root->setContentsMargins(24, 22, 24, 22);
    root->setSpacing(14);

    auto *titleLabel = new QLabel(QStringLiteral("票务管理"), this);
    titleLabel->setObjectName(QStringLiteral("pageTitle"));
    auto *hintLabel = new QLabel(QStringLiteral("办理订票、退票和改签业务。"), this);
    hintLabel->setObjectName(QStringLiteral("pageHint"));
    root->addWidget(titleLabel);
    root->addWidget(hintLabel);

    // ── Message label ──────────────────────────────────────────
    m_msgLabel = new QLabel(QStringLiteral(" "), this);
    m_msgLabel->setMinimumHeight(34);
    m_msgLabel->setWordWrap(true);
    root->addWidget(m_msgLabel);

    // ── Book ───────────────────────────────────────────────────
    auto *bookGroup = new QGroupBox(QStringLiteral("订票"), this);
    auto *bookForm  = new QFormLayout;
    bookForm->setHorizontalSpacing(14);
    bookForm->setVerticalSpacing(10);

    m_bookTrainEdit = new QLineEdit(bookGroup);
    m_bookTrainEdit->setPlaceholderText(QStringLiteral("输入车次 trainId"));
    m_bookNameEdit  = new QLineEdit(bookGroup);
    m_bookNameEdit->setPlaceholderText(QStringLiteral("乘客姓名"));

    bookForm->addRow(QStringLiteral("车次ID"),   m_bookTrainEdit);
    bookForm->addRow(QStringLiteral("乘客姓名"), m_bookNameEdit);

    auto *bookBtn = new QPushButton(QStringLiteral("订票"), bookGroup);
    connect(bookBtn, &QPushButton::clicked, this, &TicketManageDialog::handleBook);

    auto *bookLayout = new QVBoxLayout(bookGroup);
    bookLayout->addLayout(bookForm);
    bookLayout->addWidget(bookBtn, 0, Qt::AlignRight);
    root->addWidget(bookGroup);

    // ── Refund ─────────────────────────────────────────────────
    auto *refundGroup = new QGroupBox(QStringLiteral("退票"), this);
    auto *refundForm  = new QFormLayout;
    refundForm->setHorizontalSpacing(14);
    refundForm->setVerticalSpacing(10);

    m_refundOrderEdit = new QLineEdit(refundGroup);
    m_refundOrderEdit->setPlaceholderText(QStringLiteral("输入订单号 orderId"));
    refundForm->addRow(QStringLiteral("订单号"), m_refundOrderEdit);

    auto *refundBtn = new QPushButton(QStringLiteral("退票"), refundGroup);
    refundBtn->setObjectName(QStringLiteral("dangerBtn"));
    connect(refundBtn, &QPushButton::clicked, this, &TicketManageDialog::handleRefund);

    auto *refundLayout = new QVBoxLayout(refundGroup);
    refundLayout->addLayout(refundForm);
    refundLayout->addWidget(refundBtn, 0, Qt::AlignRight);
    root->addWidget(refundGroup);

    // ── Change ─────────────────────────────────────────────────
    auto *changeGroup = new QGroupBox(QStringLiteral("改签"), this);
    auto *changeForm  = new QFormLayout;
    changeForm->setHorizontalSpacing(14);
    changeForm->setVerticalSpacing(10);

    m_changeOrderEdit = new QLineEdit(changeGroup);
    m_changeOrderEdit->setPlaceholderText(QStringLiteral("原订单号 orderId"));
    m_changeTrainEdit = new QLineEdit(changeGroup);
    m_changeTrainEdit->setPlaceholderText(QStringLiteral("新车次 trainId"));

    changeForm->addRow(QStringLiteral("原订单号"), m_changeOrderEdit);
    changeForm->addRow(QStringLiteral("新车次ID"), m_changeTrainEdit);

    auto *changeBtn = new QPushButton(QStringLiteral("改签"), changeGroup);
    connect(changeBtn, &QPushButton::clicked, this, &TicketManageDialog::handleChange);

    auto *changeLayout = new QVBoxLayout(changeGroup);
    changeLayout->addLayout(changeForm);
    changeLayout->addWidget(changeBtn, 0, Qt::AlignRight);
    root->addWidget(changeGroup);

    // ── Bottom ─────────────────────────────────────────────────
    auto *bottom = new QHBoxLayout;
    bottom->addStretch();
    auto *closeBtn = new QPushButton(QStringLiteral("关闭"), this);
    connect(closeBtn, &QPushButton::clicked, this, [this]() { close(); });
    bottom->addWidget(closeBtn);
    root->addLayout(bottom);
}

void TicketManageDialog::showMsg(bool ok, const QString &msg)
{
    m_msgLabel->setText(msg);
    m_msgLabel->setStyleSheet(ok
        ? QStringLiteral("color:#14532d; background:#dcfce7; border:1px solid #86efac; border-radius:8px; padding:8px 10px;")
        : QStringLiteral("color:#9f1239; background:#fff1f2; border:1px solid #fecdd3; border-radius:8px; padding:8px 10px;"));
}

void TicketManageDialog::handleBook()
{
    TicketManager tm(m_db);
    bool ok;
    int trainId = m_bookTrainEdit->text().toInt(&ok);
    if (!ok || trainId <= 0) { showMsg(false, QStringLiteral("请输入有效的车次ID")); return; }
    if (m_bookNameEdit->text().trimmed().isEmpty()) { showMsg(false, QStringLiteral("请输入乘客姓名")); return; }

    int orderId = tm.bookTicket(m_userId, trainId, m_bookNameEdit->text().trimmed());
    if (orderId <= 0) { showMsg(false, tm.lastError()); return; }
    showMsg(true, QStringLiteral("订票成功，订单号：%1").arg(orderId));
    m_bookTrainEdit->clear();
    m_bookNameEdit->clear();
}

void TicketManageDialog::handleRefund()
{
    TicketManager tm(m_db);
    bool ok;
    int orderId = m_refundOrderEdit->text().toInt(&ok);
    if (!ok || orderId <= 0) { showMsg(false, QStringLiteral("请输入有效的订单号")); return; }

    if (!tm.refundTicket(orderId)) { showMsg(false, tm.lastError()); return; }
    showMsg(true, QStringLiteral("退票成功，订单号：%1").arg(orderId));
    m_refundOrderEdit->clear();
}

void TicketManageDialog::handleChange()
{
    TicketManager tm(m_db);
    bool ok;
    int orderId  = m_changeOrderEdit->text().toInt(&ok);
    if (!ok || orderId <= 0) { showMsg(false, QStringLiteral("请输入有效的原订单号")); return; }
    int newTrain = m_changeTrainEdit->text().toInt(&ok);
    if (!ok || newTrain <= 0) { showMsg(false, QStringLiteral("请输入有效的新车次ID")); return; }

    if (!tm.changeTicket(orderId, newTrain)) { showMsg(false, tm.lastError()); return; }
    showMsg(true, QStringLiteral("改签成功，原订单号：%1 → 新车次ID：%2").arg(orderId).arg(newTrain));
    m_changeOrderEdit->clear();
    m_changeTrainEdit->clear();
}
