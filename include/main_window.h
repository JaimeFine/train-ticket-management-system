#pragma once

#include <QMainWindow>
#include <memory>

class DatabaseManager;
class TicketManager;
class StatisticsManager;
class TicketWindow;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;

private:
    void initDatabase();
    void insertTestData();
    void setupUi();

    std::unique_ptr<DatabaseManager>    m_db;
    std::unique_ptr<TicketManager>      m_ticketMgr;
    std::unique_ptr<StatisticsManager>  m_statsMgr;
    TicketWindow *m_ticketWindow = nullptr;
};
