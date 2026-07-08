// include/train_manager.h
#pragma once

#include <QString>
#include <QVector>

// ---------- 车次数据结构（对齐数据库 schema）----------
struct Train {
    int trainId;                 // 自增主键
    QString trainNumber;         // 车次号，如 G1234
    int departureStationId;      // 出发站ID
    int arrivalStationId;        // 到达站ID
    QString departureTime;       // 出发时间，如 "08:00"
    QString arrivalTime;         // 到达时间，如 "12:30"
    int totalSeats;              // 总座位数
    int remainingSeats;          // 剩余座位数
    bool enabled;                // 是否启用（1=启用，0=停运）
};

// ---------- 车次管理类 ----------
class TrainManager
{
public:
    TrainManager() = default;

    // 获取最后一次操作的状态信息（供 UI 调用）
    QString statusMessage() const;

    // ---------- 查询 ----------
    QVector<Train> getAllTrains(bool onlyEnabled = true);

private:
    QString m_lastStatus;        // 记录最后一次操作的状态信息
    void setStatus(const QString& msg) { m_lastStatus = msg; }
};