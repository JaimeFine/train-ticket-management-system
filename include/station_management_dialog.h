#ifndef STATION_MANAGEMENT_DIALOG_H
#define STATION_MANAGEMENT_DIALOG_H

#include <QDialog>
#include <QVector>

class QTableWidget;
class QLineEdit;
class QPushButton;
class QLabel;

struct StationItem {
    int id;
    QString name;
};

class StationManagementDialog : public QDialog
{
    Q_OBJECT

public:
    explicit StationManagementDialog(QWidget *parent = nullptr);

private slots:
    void refreshList();
    void addStation();
    void deleteStation();

private:
    void setupUI();
    void loadData();
    void showMessage(const QString &msg, bool success = false);

    QTableWidget *m_table;
    QLineEdit *m_nameInput;
    QPushButton *m_addBtn;
    QPushButton *m_deleteBtn;
    QPushButton *m_refreshBtn;
    QLabel *m_messageLabel;
};

#endif // STATION_MANAGEMENT_DIALOG_H