#pragma once
/** OrderRecord: V2 — one ticket purchase */
#include <QString>

struct OrderRecord {
    int orderId = 0;
    int userId = 0;
    int tripId = 0;
    QString passengerName;
    QString travelDate;
    QString purchaseTime;
    double price = 0.0;
    int status = 0;   // 0=已预订 1=已退票 2=已改签 3=已过期
};
