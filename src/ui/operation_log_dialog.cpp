#include "operation_log_dialog.h"

#include "database_manager.h"

#include <QAbstractItemView>
#include <QHeaderView>
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

    auto *layout = new QVBoxLayout(this);
    m_table = new QTableWidget(this);
    m_table->setColumnCount(5);
    m_table->setHorizontalHeaderLabels({
        QStringLiteral("日志ID"),
        QStringLiteral("操作人"),
        QStringLiteral("操作"),
        QStringLiteral("详情"),
        QStringLiteral("时间")
    });
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->setSelectionMode(QAbstractItemView::SingleSelection);
    m_table->verticalHeader()->setVisible(false);
    m_table->horizontalHeader()->setStretchLastSection(true);

    auto *closeButton = new QPushButton(QStringLiteral("关闭"), this);
    connect(closeButton, &QPushButton::clicked, this, &QDialog::accept);

    layout->addWidget(m_table);
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
    m_table->resizeColumnsToContents();
}
