#pragma once
/** TripRecord: one specific departure of a train on one date */
#include <QString>

struct TripRecord {
    int tripId = 0;
    int trainId = 0;
    QString travelDate;
    QString departureTime;
    QString arrivalTime;
    int totalSeats = 0;
    int remainingSeats = 0;
    double basePrice = 0.0;
    bool enabled = true;
};
