#ifndef TRAIN_MANAGEMENT_DIALOG_H
#define TRAIN_MANAGEMENT_DIALOG_H

#include <QDialog>
#include "train_manager.h"

class QTableWidget;
class QLineEdit;
class QComboBox;
class QPushButton;
class QLabel;

class TrainManagementDialog : public QDialog
{
    Q_OBJECT

public:
    explicit TrainManagementDialog(TrainManager* manager, QWidget *parent = nullptr);

private slots:
    void refreshList();
    void searchTrain();
    void searchByStation();
    void addTrain();
    void editTrain();
    void deleteTrain();
    void updateSeats();
    void onTableRowClicked();

private:
    void setupUI();
    void loadData();
    void loadStations();
    void clearSelection();
    void showMessage(const QString &msg, bool success = false);
    int getSelectedTrainId();

    QTableWidget *m_table;
    QLineEdit *m_searchInput;
    QComboBox *m_stationCombo;
    QPushButton *m_searchBtn;
    QPushButton *m_searchStationBtn;
    QPushButton *m_refreshBtn;
    QPushButton *m_addBtn;
    QPushButton *m_editBtn;
    QPushButton *m_deleteBtn;
    QPushButton *m_seatBtn;
    QLabel *m_messageLabel;
    QLabel *m_countLabel;
    QMap<int, QString> m_stationNameMap;

    TrainManager* m_manager;
};

#endif // TRAIN_MANAGEMENT_DIALOG_H