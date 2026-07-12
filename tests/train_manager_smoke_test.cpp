#include "database_manager.h"
#include "train_manager.h"

#include <QCoreApplication>
#include <QDateTime>
#include <QDebug>

namespace {
QString makeUniqueName(const QString &prefix)
{
    return QStringLiteral("%1_%2")
    .arg(prefix)
        .arg(QDateTime::currentMSecsSinceEpoch());
}

bool checkResult(bool result, const char *message)
{
    if (!result) {
        qCritical() << "❌" << message;
    }
    return result;
}

int createTestStation(DatabaseManager &manager, const QString &name)
{
    StationRecord station;
    station.stationName = name;
    if (!manager.addStation(station)) {
        qCritical() << "Failed to create station:" << manager.lastError();
        return -1;
    }
    auto record = manager.findStationByName(name);
    return record.has_value() ? record->stationId : -1;
}
}

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);

    bool ok = true;

    // Initialize database
    DatabaseManager manager;
    if (!manager.initialize()) {
        qCritical() << "Database initialization failed:" << manager.lastError();
        return 1;
    }
    qDebug() << "✅ Database initialized";

    TrainManager trainManager(&manager);

    // Create test stations
    int stationA = createTestStation(manager, makeUniqueName("Station_A"));
    int stationB = createTestStation(manager, makeUniqueName("Station_B"));
    ok = checkResult(stationA > 0 && stationB > 0, "Failed to create test stations") && ok;

    if (!ok) return 1;

    qDebug() << "✅ Test stations created";

    // ============================================================
    // Test 1: addTrain() - Normal
    // ============================================================
    qDebug() << "\n=== Test 1: addTrain() (normal) ===";

    Train train1;
    train1.trainNumber = makeUniqueName("G");
    train1.departureStationId = stationA;
    train1.arrivalStationId = stationB;
    train1.departureTime = "2026-07-10 08:00";
    train1.arrivalTime = "2026-07-10 12:00";
    train1.totalSeats = 100;
    train1.remainingSeats = 100;
    train1.enabled = true;

    ok = checkResult(trainManager.addTrain(train1),
                     "addTrain() should succeed") && ok;
    qDebug() << "  " << trainManager.statusMessage();

    // ============================================================
    // Test 2: addTrain() - Duplicate
    // ============================================================
    qDebug() << "\n=== Test 2: addTrain() (duplicate) ===";

    Train train1_dup = train1;
    ok = checkResult(!trainManager.addTrain(train1_dup),
                     "Duplicate should be rejected") && ok;
    qDebug() << "  " << trainManager.statusMessage();

    // ============================================================
    // Test 3: addTrain() - Validation
    // ============================================================
    qDebug() << "\n=== Test 3: addTrain() (validation) ===";

    Train invalid1;  // empty train number
    invalid1.trainNumber = "";
    invalid1.departureStationId = stationA;
    invalid1.arrivalStationId = stationB;
    invalid1.departureTime = "2026-07-10 08:00";
    invalid1.arrivalTime = "2026-07-10 12:00";
    invalid1.totalSeats = 100;
    invalid1.remainingSeats = 100;
    invalid1.enabled = true;
    ok = checkResult(!trainManager.addTrain(invalid1),
                     "Empty train number should be rejected") && ok;
    qDebug() << "  " << trainManager.statusMessage();

    Train invalid2 = train1;  // departure == arrival
    invalid2.trainNumber = makeUniqueName("G");
    invalid2.departureStationId = stationA;
    invalid2.arrivalStationId = stationA;
    ok = checkResult(!trainManager.addTrain(invalid2),
                     "Same departure/arrival should be rejected") && ok;
    qDebug() << "  " << trainManager.statusMessage();

    Train invalid3 = train1;  // invalid time format
    invalid3.trainNumber = makeUniqueName("G");
    invalid3.departureTime = "08:00";
    ok = checkResult(!trainManager.addTrain(invalid3),
                     "Invalid time format should be rejected") && ok;
    qDebug() << "  " << trainManager.statusMessage();

    Train invalid4 = train1;  // totalSeats <= 0
    invalid4.trainNumber = makeUniqueName("G");
    invalid4.totalSeats = 0;
    ok = checkResult(!trainManager.addTrain(invalid4),
                     "totalSeats <= 0 should be rejected") && ok;
    qDebug() << "  " << trainManager.statusMessage();

    Train invalid5 = train1;  // remainingSeats > totalSeats
    invalid5.trainNumber = makeUniqueName("G");
    invalid5.totalSeats = 100;
    invalid5.remainingSeats = 150;
    ok = checkResult(!trainManager.addTrain(invalid5),
                     "remainingSeats > totalSeats should be rejected") && ok;
    qDebug() << "  " << trainManager.statusMessage();

    // ============================================================
    // Test 4: updateTrain() - Normal
    // ============================================================
    qDebug() << "\n=== Test 4: updateTrain() (normal) ===";

    auto trainRecord = manager.findTrainByNumber(train1.trainNumber);
    if (trainRecord.has_value()) {
        Train updateTrain;
        updateTrain.trainId = trainRecord->trainId;
        updateTrain.trainNumber = train1.trainNumber;
        updateTrain.departureStationId = stationA;
        updateTrain.arrivalStationId = stationB;
        updateTrain.departureTime = "2026-07-10 09:00";
        updateTrain.arrivalTime = "2026-07-10 13:00";
        updateTrain.totalSeats = 150;
        updateTrain.remainingSeats = 120;
        updateTrain.enabled = true;

        ok = checkResult(trainManager.updateTrain(updateTrain),
                         "updateTrain() should succeed") && ok;
        qDebug() << "  " << trainManager.statusMessage();

        auto updated = manager.findTrainById(trainRecord->trainId);
        if (updated.has_value()) {
            ok = checkResult(updated->departureTime == "2026-07-10 09:00",
                             "Departure time not updated") && ok;
            ok = checkResult(updated->totalSeats == 150,
                             "Total seats not updated") && ok;
            qDebug() << "  ✅ Update verified";
        }
    } else {
        qCritical() << "❌ Train not found";
        ok = false;
    }

    // ============================================================
    // Test 5: updateTrain() - Non-existent
    // ============================================================
    qDebug() << "\n=== Test 5: updateTrain() (non-existent) ===";

    Train nonExist;
    nonExist.trainId = 99999;
    nonExist.trainNumber = "NON_EXIST";
    nonExist.departureStationId = stationA;
    nonExist.arrivalStationId = stationB;
    nonExist.departureTime = "2026-07-10 08:00";
    nonExist.arrivalTime = "2026-07-10 12:00";
    nonExist.totalSeats = 100;
    nonExist.remainingSeats = 100;
    nonExist.enabled = true;

    ok = checkResult(!trainManager.updateTrain(nonExist),
                     "Non-existent train should be rejected") && ok;
    qDebug() << "  " << trainManager.statusMessage();

    // ============================================================
    // Test 6: updateRemainingSeats()
    // ============================================================
    qDebug() << "\n=== Test 6: updateRemainingSeats() ===";

    auto seatTrain = manager.findTrainByNumber(train1.trainNumber);
    if (seatTrain.has_value()) {
        int id = seatTrain->trainId;

        // Sell 5
        if (trainManager.updateRemainingSeats(id, -5)) {
            qDebug() << "  ✅ Sold 5 tickets:" << trainManager.statusMessage();
            auto after = manager.findTrainById(id);
            if (after.has_value()) {
                ok = checkResult(after->remainingSeats == 95,
                                 "Remaining seats should be 95 after selling 5") && ok;
            }
        } else {
            qCritical() << "❌ Failed to sell:" << trainManager.statusMessage();
            ok = false;
        }

        // Refund 3
        if (trainManager.updateRemainingSeats(id, +3)) {
            qDebug() << "  ✅ Refunded 3 tickets:" << trainManager.statusMessage();
            auto after = manager.findTrainById(id);
            if (after.has_value()) {
                ok = checkResult(after->remainingSeats == 98,
                                 "Remaining seats should be 98 after refunding 3") && ok;
            }
        } else {
            qCritical() << "❌ Failed to refund:" << trainManager.statusMessage();
            ok = false;
        }

        // Over-sell (should fail)
        if (!trainManager.updateRemainingSeats(id, -999)) {
            qDebug() << "  ✅ Over-sell rejected:" << trainManager.statusMessage();
        } else {
            qCritical() << "❌ Over-sell should have failed!";
            ok = false;
        }

        // Delta = 0 (should fail)
        if (!trainManager.updateRemainingSeats(id, 0)) {
            qDebug() << "  ✅ Delta=0 rejected:" << trainManager.statusMessage();
        } else {
            qCritical() << "❌ Delta=0 should have failed!";
            ok = false;
        }
    } else {
        qCritical() << "❌ Train not found";
        ok = false;
    }

    // ============================================================
    // Tests 7-10: Conditional (disabled by default)
    // ============================================================

#ifdef TRAIN_MANAGER_TEST_GET_ALL_TRAINS
    qDebug() << "\n=== Test 7: getAllTrains() ===";
    auto all = trainManager.getAllTrains(false);
    qDebug() << "  Total trains:" << all.size();
    bool found = false;
    for (const auto& t : all) {
        if (t.trainNumber == train1.trainNumber) found = true;
    }
    ok = checkResult(found, "Test train should be in getAllTrains()") && ok;
#else
    qDebug() << "\n=== Test 7: getAllTrains() ===";
    qDebug() << "  ⏳ Skipped (requires DatabaseManager::getAllTrains)";
#endif

#ifdef TRAIN_MANAGER_TEST_DELETE_TRAIN
    qDebug() << "\n=== Test 8: deleteTrain() ===";
    Train temp;
    temp.trainNumber = makeUniqueName("TEMP_DEL");
    temp.departureStationId = stationA;
    temp.arrivalStationId = stationB;
    temp.departureTime = "2026-07-10 08:00";
    temp.arrivalTime = "2026-07-10 12:00";
    temp.totalSeats = 50;
    temp.remainingSeats = 50;
    temp.enabled = true;

    if (trainManager.addTrain(temp)) {
        auto rec = manager.findTrainByNumber(temp.trainNumber);
        if (rec.has_value()) {
            if (trainManager.deleteTrain(rec->trainId)) {
                qDebug() << "  ✅ Delete succeeded:" << trainManager.statusMessage();
                auto v = manager.findTrainById(rec->trainId);
                if (v.has_value()) {
                    ok = checkResult(!v->enabled, "Train should be disabled") && ok;
                }
            } else {
                qCritical() << "❌ Delete failed:" << trainManager.statusMessage();
                ok = false;
            }
            if (!trainManager.deleteTrain(rec->trainId)) {
                qDebug() << "  ✅ Duplicate delete rejected:" << trainManager.statusMessage();
            } else {
                qCritical() << "❌ Duplicate delete should have failed!";
                ok = false;
            }
        }
    }
    if (!trainManager.deleteTrain(99999)) {
        qDebug() << "  ✅ Non-existent delete rejected:" << trainManager.statusMessage();
    }
#else
    qDebug() << "\n=== Test 8: deleteTrain() ===";
    qDebug() << "  ⏳ Skipped (requires DatabaseManager::deleteTrain)";
#endif

#ifdef TRAIN_MANAGER_TEST_SEARCH_TRAINS
    qDebug() << "\n=== Test 9: searchTrains() ===";
    auto results = trainManager.searchTrains(train1.trainNumber);
    qDebug() << "  Found" << results.size() << "matching trains";
    ok = checkResult(!results.isEmpty(), "searchTrains should find test train") && ok;
#else
    qDebug() << "\n=== Test 9: searchTrains() ===";
    qDebug() << "  ⏳ Skipped (requires DatabaseManager::searchTrains)";
#endif

#ifdef TRAIN_MANAGER_TEST_SEARCH_BY_STATION
    qDebug() << "\n=== Test 10: searchByStation() ===";
    auto dep = trainManager.searchByStation(stationA, true);
    auto arr = trainManager.searchByStation(stationB, false);
    qDebug() << "  Departures from stationA:" << dep.size();
    qDebug() << "  Arrivals at stationB:" << arr.size();
    ok = checkResult(!dep.isEmpty() && !arr.isEmpty(),
                     "searchByStation should find trains") && ok;
#else
    qDebug() << "\n=== Test 10: searchByStation() ===";
    qDebug() << "  ⏳ Skipped (requires DatabaseManager::searchByStation)";
#endif

    // ============================================================
    // Summary
    // ============================================================
    qDebug() << "\n=== Summary ===";
    qDebug() << "  ✅ Working now: addTrain, updateTrain, updateRemainingSeats";
    qDebug() << "  ⏳ Pending APIs: getAllTrains, deleteTrain, searchTrains, searchByStation";

    if (ok) {
        qDebug() << "✅ All active tests passed!";
        return 0;
    } else {
        qCritical() << "❌ Some tests failed!";
        return 1;
    }
}