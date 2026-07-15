#ifndef TRANSFER_DIALOG_H
#define TRANSFER_DIALOG_H

#include <QDialog>
#include <QMap>
#include "route_manager.h"

class QComboBox;
class QPushButton;
class QLabel;
class QTableWidget;

class TransferDialog : public QDialog
{
    Q_OBJECT

public:
    explicit TransferDialog(RouteManager* routeManager, QWidget *parent = nullptr);

private slots:
    void onSearchClicked();
    void swapStations();

private:
    void setupUI();
    void loadStations();
    void showPathResult(const RoutePath& path);
    void showError(const QString& msg);
    void clearResult();

    RouteManager* m_routeManager;
    QMap<int, QString> m_stationNameMap;

    QComboBox *m_fromCombo;
    QComboBox *m_toCombo;
    QComboBox *m_conditionCombo;
    QPushButton *m_swapBtn;
    QPushButton *m_searchBtn;
    QTableWidget *m_resultTable;
    QLabel *m_statusLabel;
    QLabel *m_summaryLabel;
};

#endif // TRANSFER_DIALOG_H