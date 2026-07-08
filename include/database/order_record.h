#pragma once

// Properties of OrderRecord

#include <QString>

struct OrderRecord {
    int orderId = 0;
    int userId = 0;
    int trainId = 0;
    QString passengerName;
    QString purchaseTime;
    int status = 0;
};
