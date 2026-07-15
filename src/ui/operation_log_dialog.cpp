#include "operation_log_dialog.h"

#include "database_manager.h"
#include "app_style.h"

#include <QAbstractItemView>
#include <QHeaderView>
#include <QLabel>
#include <QPushButton>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QVBoxLayout>

OperationLogDialog::OperationLogDialog(const DatabaseManager &databaseManager,
                                       QWidget *parent)
    : QDialog(parent)
    , m_databaseManager(&databaseManager)
{
    setWindowTitle(QStringLiteral("系统操作日志"));
    setModal(true);
    resize(860, 560);
    setStyleSheet(UiStyle::dialogStyleSheet());

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(24, 22, 24, 22);
    layout->setSpacing(14);

    auto *titleLabel = new QLabel(QStringLiteral("系统操作日志"), this);
    titleLabel->setObjectName(QStringLiteral("titleLabel"));
    auto *hintLabel = new QLabel(QStringLiteral("查看登录、账号维护和票务办理等操作记录。"), this);
    hintLabel->setObjectName(QStringLiteral("hintLabel"));

    m_table = new QTableWidget(this);
    m_table->setColumnCount(5);
    m_table->setHorizontalHeaderLabels({
        QStringLiteral("日志ID"),
        QStringLiteral("操作人"),
        QStringLiteral("操作"),
        QStringLiteral("详情"),
        QStringLiteral("时间")
    });
    UiStyle::prepareTable(m_table);
    QHeaderView *header = m_table->horizontalHeader();
    header->setStretchLastSection(false);
    header->setSectionResizeMode(QHeaderView::ResizeToContents);
    header->setSectionResizeMode(3, QHeaderView::Stretch);
    header->setSectionResizeMode(4, QHeaderView::Stretch);

    auto *closeButton = new QPushButton(QStringLiteral("关闭"), this);
    connect(closeButton, &QPushButton::clicked, this, &QDialog::accept);

    layout->addWidget(titleLabel);
    layout->addWidget(hintLabel);
    layout->addWidget(m_table, 1);
    layout->addWidget(closeButton, 0, Qt::AlignRight);

    loadLogs();
}

void OperationLogDialog::loadLogs()
{
    const auto logs = m_databaseManager->findAllOperationLogs();
    m_table->setRowCount(logs.size());
    for (int row = 0; row < logs.size(); ++row) {
        const auto &log = logs[row];
        m_table->setItem(row, 0, new QTableWidgetItem(QString::number(log.logId)));
        m_table->setItem(row, 1, new QTableWidgetItem(log.operatorUsername));
        m_table->setItem(row, 2, new QTableWidgetItem(log.action));
        m_table->setItem(row, 3, new QTableWidgetItem(log.detail));
        m_table->setItem(row, 4, new QTableWidgetItem(log.createdAt));
    }
}
