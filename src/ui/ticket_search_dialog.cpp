/**
 * @file ticket_search_dialog.cpp - Issue 9: 车票查询
 */
#include "ticket_search_dialog.h"
#include "database_manager.h"
#include "ticket_manager.h"

#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QTableWidget>
#include <QVBoxLayout>

TicketSearchDialog::TicketSearchDialog(DatabaseManager &db, QWidget *parent)
    : QDialog(parent), m_db(db)
{
    setWindowTitle(QStringLiteral("车票查询"));
    setModal(true);
    resize(820, 560);

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
        QTableWidget { background:#ffffff; border:1px solid #cbd8d2; border-radius:8px;
            gridline-color:#e5ece8; selection-background-color:#d9f99d; selection-color:#153832; }
        QHeaderView::section { background:#eef5f1; color:#33433d; border:none; padding:7px; font-weight:700; }
        QPushButton { min-height:32px; border-radius:8px; padding:5px 14px; font-weight:700;
            color:#ffffff; background:#176b5b; border:none; }
        QPushButton:hover { background:#0f5749; }
    )QSS"));

    auto *root = new QVBoxLayout(this);
    root->setContentsMargins(24, 22, 24, 22);
    root->setSpacing(14);

    auto *titleLabel = new QLabel(QStringLiteral("车票查询"), this);
    titleLabel->setObjectName(QStringLiteral("pageTitle"));
    auto *hintLabel = new QLabel(QStringLiteral("按出发/到达站或车次号查询车次和余票信息。"), this);
    hintLabel->setObjectName(QStringLiteral("pageHint"));
    root->addWidget(titleLabel);
    root->addWidget(hintLabel);

    // ── Search by station ──────────────────────────────────────
    auto *stationGroup = new QGroupBox(QStringLiteral("按站点查询"), this);
    auto *stationLayout = new QVBoxLayout(stationGroup);
    auto *stationForm = new QFormLayout;
    stationForm->setHorizontalSpacing(14);
    stationForm->setVerticalSpacing(10);

    m_depEdit  = new QLineEdit(stationGroup);
    m_depEdit->setPlaceholderText(QStringLiteral("出发站（支持模糊匹配）"));
    m_arrEdit  = new QLineEdit(stationGroup);
    m_arrEdit->setPlaceholderText(QStringLiteral("到达站（支持模糊匹配）"));
    m_dateEdit = new QLineEdit(stationGroup);
    m_dateEdit->setPlaceholderText(QStringLiteral("日期，如 2026-07（可选）"));

    stationForm->addRow(QStringLiteral("出发站"), m_depEdit);
    stationForm->addRow(QStringLiteral("到达站"), m_arrEdit);
    stationForm->addRow(QStringLiteral("日期"),    m_dateEdit);

    auto *stationSearchBtn = new QPushButton(QStringLiteral("搜索"), stationGroup);
    connect(stationSearchBtn, &QPushButton::clicked, this, &TicketSearchDialog::searchByStation);

    stationLayout->addLayout(stationForm);
    stationLayout->addWidget(stationSearchBtn, 0, Qt::AlignRight);
    root->addWidget(stationGroup);

    // ── Search by train number ─────────────────────────────────
    auto *numGroup = new QGroupBox(QStringLiteral("按车次号查询"), this);
    auto *numLayout = new QHBoxLayout(numGroup);
    numLayout->setSpacing(12);

    m_numEdit = new QLineEdit(numGroup);
    m_numEdit->setPlaceholderText(QStringLiteral("请输入车次号，如 G123"));

    auto *numSearchBtn = new QPushButton(QStringLiteral("查询"), numGroup);
    connect(numSearchBtn, &QPushButton::clicked, this, &TicketSearchDialog::searchByNumber);

    numLayout->addWidget(m_numEdit, 1);
    numLayout->addWidget(numSearchBtn);
    root->addWidget(numGroup);

    // ── Results table ──────────────────────────────────────────
    m_table = new QTableWidget(this);
    m_table->setColumnCount(6);
    m_table->setHorizontalHeaderLabels({
        QStringLiteral("车次"),   QStringLiteral("出发站"),
        QStringLiteral("到达站"), QStringLiteral("出发时间"),
        QStringLiteral("到达时间"), QStringLiteral("余票")
    });
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->verticalHeader()->setVisible(false);
    m_table->horizontalHeader()->setStretchLastSection(true);
    m_table->setMinimumHeight(200);
    root->addWidget(m_table);

    // ── Bottom ─────────────────────────────────────────────────
    auto *bottom = new QHBoxLayout;
    bottom->addStretch();
    auto *closeBtn = new QPushButton(QStringLiteral("关闭"), this);
    connect(closeBtn, &QPushButton::clicked, this, [this]() { close(); });
    bottom->addWidget(closeBtn);
    root->addLayout(bottom);
}

void TicketSearchDialog::searchByStation()
{
    TicketManager tm(m_db);
    populateTable(tm.searchTrains(
        m_depEdit->text().trimmed(),
        m_arrEdit->text().trimmed(),
        m_dateEdit->text().trimmed()));
}

void TicketSearchDialog::searchByNumber()
{
    TicketManager tm(m_db);
    populateTable(tm.searchByTrainNumber(m_numEdit->text().trimmed()));
}

void TicketSearchDialog::populateTable(const QVector<QVariantMap> &results)
{
    m_table->setRowCount(results.size());
    for (int i = 0; i < results.size(); ++i) {
        const auto &r = results[i];
        m_table->setItem(i, 0, new QTableWidgetItem(r["trainNumber"].toString()));
        m_table->setItem(i, 1, new QTableWidgetItem(r["departureStation"].toString()));
        m_table->setItem(i, 2, new QTableWidgetItem(r["arrivalStation"].toString()));
        m_table->setItem(i, 3, new QTableWidgetItem(r["departureTime"].toString()));
        m_table->setItem(i, 4, new QTableWidgetItem(r["arrivalTime"].toString()));

        auto *seat = new QTableWidgetItem;
        seat->setData(Qt::DisplayRole, r["remainingSeats"].toInt());
        seat->setTextAlignment(Qt::AlignCenter);
        m_table->setItem(i, 5, seat);
    }
    m_table->resizeRowsToContents();
}
