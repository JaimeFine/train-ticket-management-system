#ifndef TRAIN_MANAGEMENT_DIALOG_H
#define TRAIN_MANAGEMENT_DIALOG_H

#include <QDialog>
#include "train_manager.h"

class QTabWidget;
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
    void resumeTrain();
    void deleteTrainPermanently();
    void updateSeats();
    void onTableRowClicked();
    void loadTripsForTrain(int trainId);
    void generateTrips();
    void editTrip();
    void disableTrip();
    void toggleHistoryMode();
    void onTrainTableDoubleClicked(const QModelIndex &index);
    void goBackToTrainList();

private:
    void setupUI();
    void loadData();
    void loadStations();
    void clearSelection();
    void showMessage(const QString &msg, bool success = false);
    void selectTrainByNumber(const QString& trainNumber);
    int getSelectedTrainId();
    bool m_showHistory = false;

    QTableWidget *m_table;
    QLineEdit *m_searchInput;
    QComboBox *m_stationCombo;
    QPushButton *m_searchBtn;
    QPushButton *m_searchStationBtn;
    QPushButton *m_refreshBtn;
    QPushButton *m_addBtn;
    QPushButton *m_editBtn;
    QPushButton *m_deleteBtn;
    QPushButton *m_resumeBtn;
    QPushButton *m_purgeBtn;
    QPushButton *m_seatBtn;
    QLabel *m_messageLabel;
    QLabel *m_countLabel;
    QMap<int, QString> m_stationNameMap;
    QTabWidget *m_tabWidget;
    QTableWidget *m_tripTable;
    QPushButton *m_generateTripBtn;
    QPushButton *m_editTripBtn;
    QPushButton *m_disableTripBtn;
    QPushButton *m_historyToggleBtn;
    QLabel* m_trainInfoLabel = nullptr;
    QPushButton* m_backToTrainBtn = nullptr;

    TrainManager* m_manager;
};

#endif // TRAIN_MANAGEMENT_DIALOG_H