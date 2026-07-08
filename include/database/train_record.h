#pragma once

// Properties of TrainRecord

#include <QString>

struct TrainRecord {
    int trainId = 0;
    QString trainNumber;
    int departureStationId = 0;
    int arrivalStationId = 0;
    QString departureTime;
    QString arrivalTime;
    int totalSeats = 0;
    int remainingSeats = 0;
    bool enabled = true;
};
