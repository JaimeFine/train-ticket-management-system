#pragma once

#include <QString>

struct TripRecord {
    int tripId = 0;             // 班次ID
    int trainId = 0;            // 所属车次ID（关联 Train 模板）
    QString travelDate;         // 出行日期
    QString departureTime;      // 当日发车时间
    QString arrivalTime;        // 当日到达时间
    int totalSeats = 0;         // 当日总座位数
    int remainingSeats = 0;     // 当日余票数
    double basePrice = 0.0;     // 当日基础票价
    bool enabled = true;        // 是否在售（停运置 false）
};
