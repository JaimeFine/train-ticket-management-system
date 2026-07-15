#pragma once

// OrderRecord：订单记录（V2），一行订单即一张车票

#include <QString>

struct OrderRecord {
    int orderId = 0;          // 订单ID
    int userId = 0;           // 下单用户ID
    int trainId = 0;          // 车次ID（兼容查询结果展示）
    int tripId = 0;           // 所购班次ID（关联 Trip）
    QString passengerName;    // 乘车人姓名
    QString travelDate;       // 出行日期（冗余存储，便于直接查询）
    QString purchaseTime;     // 购票时间
    double price = 0.0;       // 成交票价
    int status = 0;           // 0=已预订 1=已退票 2=已改签 3=已过期
};
