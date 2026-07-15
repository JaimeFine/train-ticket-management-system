#include "database_manager.h"
#include "ticket_manager.h"

#include <QApplication>
#include <QAbstractItemView>
#include <QDate>
#include <QDateTime>
#include <QDebug>
#include <QGroupBox>
#include <QHeaderView>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QTimer>
#include <QVBoxLayout>

namespace {
QString uniqueName(const QString &prefix)
{
    return QStringLiteral("%1_%2")
        .arg(prefix)
        .arg(QDateTime::currentMSecsSinceEpoch());
}

struct DemoScenario
{
    QString label;
    QString trainNumber;
    int tripId = 0;
    QString travelDate;
    bool isBusy = false;
};

bool setTripBasePrice(DatabaseManager &db, int tripId, double basePrice)
{
    QSqlQuery query(QSqlDatabase::database(QStringLiteral("train_ticket_connection")));
    query.prepare(QStringLiteral("UPDATE Trip SET basePrice = :basePrice WHERE tripId = :tripId"));
    query.bindValue(QStringLiteral(":basePrice"), basePrice);
    query.bindValue(QStringLiteral(":tripId"), tripId);
    return query.exec() && query.numRowsAffected() == 1;
}

class PricingDashboard : public QWidget
{
public:
    PricingDashboard(DatabaseManager &db, TicketManager &ticketManager, QWidget *parent = nullptr)
        : QWidget(parent)
        , m_db(db)
        , m_ticketManager(ticketManager)
    {
        setWindowTitle(QStringLiteral("动态票价实时演示"));
        resize(980, 620);
        setStyleSheet(QStringLiteral(R"QSS(
            QWidget {
                background: #eef2f3;
                color: #1f2933;
                font-family: "Microsoft YaHei UI", "Microsoft YaHei", "Segoe UI";
                font-size: 14px;
            }
            QLabel#title {
                color: #17231f;
                font-size: 22px;
                font-weight: 700;
            }
            QLabel#hint {
                color: #42514b;
            }
            QGroupBox {
                background: #fbfcfb;
                border: 1px solid #d8e0dc;
                border-radius: 10px;
                margin-top: 12px;
                padding: 12px;
                font-weight: 700;
            }
            QGroupBox::title {
                subcontrol-origin: margin;
                left: 12px;
                padding: 0 6px;
            }
            QLabel#metricLabel {
                color: #42514b;
                font-size: 12px;
            }
            QLabel#metricValue {
                color: #153832;
                font-size: 26px;
                font-weight: 700;
            }
            QTableWidget {
                background: #ffffff;
                color: #1f2933;
                border: 1px solid #d8e0dc;
                border-radius: 8px;
                gridline-color: #e5ece8;
                selection-background-color: #d9f99d;
                selection-color: #153832;
            }
            QHeaderView::section {
                background: #eef5f1;
                color: #33433d;
                padding: 7px;
                border: none;
                font-weight: 700;
            }
            QPushButton {
                background: #176b5b;
                color: white;
                border: none;
                border-radius: 8px;
                padding: 8px 16px;
                font-weight: 700;
            }
            QPushButton:hover {
                background: #0f5749;
            }
        )QSS"));

        auto *rootLayout = new QVBoxLayout(this);
        rootLayout->setContentsMargins(24, 22, 24, 22);
        rootLayout->setSpacing(14);

        auto *title = new QLabel(QStringLiteral("动态票价实时演示"), this);
        title->setObjectName(QStringLiteral("title"));
        auto *hint = new QLabel(QStringLiteral("这个窗口会实时刷新价格结果。高余票场景会在“充足/紧张”之间切换，方便直观看到动态票价变化。"), this);
        hint->setObjectName(QStringLiteral("hint"));
        hint->setWordWrap(true);
        rootLayout->addWidget(title);
        rootLayout->addWidget(hint);

        auto *metricsLayout = new QHBoxLayout;
        metricsLayout->setSpacing(12);
        m_sceneCountValue = createMetricCard(QStringLiteral("场景数"), metricsLayout);
        m_minPriceValue = createMetricCard(QStringLiteral("最低价"), metricsLayout);
        m_maxPriceValue = createMetricCard(QStringLiteral("最高价"), metricsLayout);
        m_avgPriceValue = createMetricCard(QStringLiteral("平均价"), metricsLayout);
        rootLayout->addLayout(metricsLayout);

        auto *tableGroup = new QGroupBox(QStringLiteral("实时价格结果"), this);
        auto *tableLayout = new QVBoxLayout(tableGroup);
        m_table = new QTableWidget(tableGroup);
        m_table->setColumnCount(5);
        m_table->setHorizontalHeaderLabels({
            QStringLiteral("场景"),
            QStringLiteral("出行日期"),
            QStringLiteral("余票"),
            QStringLiteral("基础票价"),
            QStringLiteral("动态票价")
        });
        m_table->horizontalHeader()->setStretchLastSection(true);
        m_table->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
        m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
        m_table->setSelectionMode(QAbstractItemView::SingleSelection);
        m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
        m_table->verticalHeader()->setVisible(false);
        tableLayout->addWidget(m_table);
        rootLayout->addWidget(tableGroup, 1);

        auto *buttonRow = new QHBoxLayout;
        auto *refreshButton = new QPushButton(QStringLiteral("立即刷新"), this);
        buttonRow->addStretch();
        buttonRow->addWidget(refreshButton);
        rootLayout->addLayout(buttonRow);

        m_status = new QLabel(QStringLiteral("就绪"), this);
        m_status->setObjectName(QStringLiteral("hint"));
        rootLayout->addWidget(m_status);

        connect(refreshButton, &QPushButton::clicked, this, [this]() {
            refreshDashboard();
        });

        setupData();
        refreshDashboard();

        m_timer.setInterval(1500);
        connect(&m_timer, &QTimer::timeout, this, [this]() {
            refreshDashboard();
        });
        m_timer.start();
    }

private:
    QLabel *createMetricCard(const QString &title, QHBoxLayout *layout)
    {
        auto *card = new QGroupBox(title, this);
        auto *cardLayout = new QVBoxLayout(card);
        auto *value = new QLabel(QStringLiteral("0"), card);
        value->setObjectName(QStringLiteral("metricValue"));
        cardLayout->addWidget(value);
        layout->addWidget(card);
        return value;
    }

    void setupData()
    {
        StationRecord dep;
        dep.stationName = uniqueName(QStringLiteral("demo_dep"));
        StationRecord arr;
        arr.stationName = uniqueName(QStringLiteral("demo_arr"));
        m_db.addStation(dep);
        m_db.addStation(arr);

        const auto depStation = m_db.findStationByName(dep.stationName);
        const auto arrStation = m_db.findStationByName(arr.stationName);
        if (!depStation || !arrStation) {
            qFatal("Could not create demo stations.");
        }

        const QDate weekday = QDate::currentDate().addDays(7);
        const QDate holiday(QDate::currentDate().year(), 10, 1);
        const QDateTime nearDeparture = QDateTime::currentDateTime().addSecs(2 * 60 * 60);

        auto createScenario = [&](const QString &label,
                                  const QString &trainPrefix,
                                  const QString &travelDate,
                                  const QString &departureTime,
                                  const QString &arrivalTime,
                                  int seats,
                                  bool busy) {
            TrainRecord train;
            train.trainNumber = uniqueName(trainPrefix);
            train.departureStationId = depStation->stationId;
            train.arrivalStationId = arrStation->stationId;
            train.departureTime = departureTime;
            train.arrivalTime = arrivalTime;
            train.totalSeats = seats;
            train.enabled = true;
            if (!m_db.addTrain(train)) {
                qFatal("Could not create demo train.");
            }

            const auto storedTrain = m_db.findTrainByNumber(train.trainNumber);
            if (!storedTrain) {
                qFatal("Could not reload demo train.");
            }

            const auto tripId = m_db.createTrip(storedTrain->trainId, travelDate, seats);
            if (!tripId) {
                qFatal("Could not create demo trip.");
            }
            if (!setTripBasePrice(m_db, *tripId, 300.0)) {
                qFatal("Could not set demo base price.");
            }

            DemoScenario scenario;
            scenario.label = label;
            scenario.trainNumber = train.trainNumber;
            scenario.tripId = *tripId;
            scenario.travelDate = travelDate;
            scenario.isBusy = busy;
            m_scenarios.append(scenario);
        };

        createScenario(QStringLiteral("普通工作日"), QStringLiteral("DEMO_NORMAL"),
                       weekday.toString(QStringLiteral("yyyy-MM-dd")),
                       QStringLiteral("09:00"), QStringLiteral("11:00"), 100, false);
        createScenario(QStringLiteral("余票波动演示"), QStringLiteral("DEMO_BUSY"),
                       weekday.toString(QStringLiteral("yyyy-MM-dd")),
                       QStringLiteral("09:00"), QStringLiteral("11:00"), 100, true);
        createScenario(QStringLiteral("节假日"), QStringLiteral("DEMO_HOLIDAY"),
                       holiday.toString(QStringLiteral("yyyy-MM-dd")),
                       QStringLiteral("10:00"), QStringLiteral("12:00"), 100, false);
        createScenario(QStringLiteral("临近发车"), QStringLiteral("DEMO_NEAR"),
                       nearDeparture.date().toString(QStringLiteral("yyyy-MM-dd")),
                       nearDeparture.time().toString(QStringLiteral("HH:mm")),
                       nearDeparture.addSecs(2 * 60 * 60).time().toString(QStringLiteral("HH:mm")),
                       100, false);
    }

    void refreshDashboard()
    {
        for (DemoScenario &scenario : m_scenarios) {
            if (!scenario.isBusy) {
                continue;
            }

            const auto trip = m_db.findTripById(scenario.tripId);
            if (!trip.has_value()) {
                continue;
            }

            const int targetSeats = m_busyScarce ? 10 : 80;
            const int delta = targetSeats - trip->remainingSeats;
            if (delta != 0) {
                m_db.adjustTripSeats(scenario.tripId, delta);
            }
        }
        m_busyScarce = !m_busyScarce;

        const QVector<QVariantMap> results = m_ticketManager.searchTrips(QString(), QString(), QString());
        m_table->setRowCount(m_scenarios.size());

        double minPrice = 0.0;
        double maxPrice = 0.0;
        double totalPrice = 0.0;

        for (int row = 0; row < m_scenarios.size(); ++row) {
            const DemoScenario &scenario = m_scenarios[row];
            QVariantMap result;
            for (const QVariantMap &item : results) {
                if (item.value(QStringLiteral("trainNumber")).toString() == scenario.trainNumber) {
                    result = item;
                    break;
                }
            }

            const double dynamicPrice = result.value(QStringLiteral("dynamicPrice")).toDouble();
            const double basePrice = result.value(QStringLiteral("basePrice")).toDouble();
            const int remainingSeats = result.value(QStringLiteral("remainingSeats")).toInt();

            if (row == 0 || dynamicPrice < minPrice) {
                minPrice = dynamicPrice;
            }
            if (row == 0 || dynamicPrice > maxPrice) {
                maxPrice = dynamicPrice;
            }
            totalPrice += dynamicPrice;

            m_table->setItem(row, 0, new QTableWidgetItem(scenario.label));
            m_table->setItem(row, 1, new QTableWidgetItem(result.value(QStringLiteral("travelDate")).toString()));
            m_table->setItem(row, 2, new QTableWidgetItem(QString::number(remainingSeats)));
            m_table->setItem(row, 3, new QTableWidgetItem(QStringLiteral("￥%1").arg(basePrice, 0, 'f', 0)));
            m_table->setItem(row, 4, new QTableWidgetItem(QStringLiteral("￥%1").arg(dynamicPrice, 0, 'f', 0)));
        }

        m_sceneCountValue->setText(QString::number(m_scenarios.size()));
        m_minPriceValue->setText(QStringLiteral("￥%1").arg(minPrice, 0, 'f', 0));
        m_maxPriceValue->setText(QStringLiteral("￥%1").arg(maxPrice, 0, 'f', 0));
        m_avgPriceValue->setText(QStringLiteral("￥%1").arg(totalPrice / m_scenarios.size(), 0, 'f', 0));
        m_status->setText(QStringLiteral("已刷新：%1").arg(QDateTime::currentDateTime().toString(QStringLiteral("HH:mm:ss"))));
    }

    DatabaseManager &m_db;
    TicketManager &m_ticketManager;
    QVector<DemoScenario> m_scenarios;
    QTimer m_timer;
    bool m_busyScarce = false;
    QLabel *m_sceneCountValue = nullptr;
    QLabel *m_minPriceValue = nullptr;
    QLabel *m_maxPriceValue = nullptr;
    QLabel *m_avgPriceValue = nullptr;
    QLabel *m_status = nullptr;
    QTableWidget *m_table = nullptr;
};
}

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    DatabaseManager db;
    if (!db.initialize()) {
        qCritical() << "Initialization failed:" << db.lastError();
        return 1;
    }

    TicketManager ticketManager(db);
    PricingDashboard dashboard(db, ticketManager);
    dashboard.show();

    return app.exec();
}
