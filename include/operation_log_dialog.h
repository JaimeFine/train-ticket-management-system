#pragma once

#include <QDialog>

class DatabaseManager;
class QTableWidget;

class OperationLogDialog : public QDialog
{
public:
    explicit OperationLogDialog(const DatabaseManager &databaseManager,
                                QWidget *parent = nullptr);

private:
    void loadLogs();

    const DatabaseManager *m_databaseManager = nullptr;
    QTableWidget *m_table = nullptr;
};
