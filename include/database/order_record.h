#pragma once

// Properties of OrderRecord

#include <QString>

struct OrderRecord {
    int orderId = 0;
    int userId = 0;
    int trainId = 0;
    int tripId = 0;
    QString passengerName;
    QString travelDate;
    QString purchaseTime;
    double price = 0.0;
    int status = 0;
};
