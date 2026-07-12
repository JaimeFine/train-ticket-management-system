// include/train_manager.h
#pragma once

#include <QString>
#include <QVector>

class DatabaseManager;

// ============================================================
// 车次数据结构（业务层使用）
// ============================================================
struct Train {
    int trainId;                 // 自增主键
    QString trainNumber;         // 车次号，如 G1234
    int departureStationId;      // 出发站ID
    int arrivalStationId;        // 到达站ID
    QString departureTime;       // 出发时间，如 "08:00"
    QString arrivalTime;         // 到达时间，如 "12:30"
    int totalSeats;              // 总座位数
    int remainingSeats;          // 剩余座位数
    bool enabled;                // true=启用，false=停运
};

// ============================================================
// 车次管理类
// ============================================================
class TrainManager
{
public:
    explicit TrainManager(DatabaseManager* dbManager = nullptr);

    // ---------- 状态信息 ----------
    QString statusMessage() const;

    // ---------- 查询 ----------
    QVector<Train> getAllTrains(bool onlyEnabled = true);

    // ---------- CRUD ----------
    bool addTrain(const Train& train);
    bool updateTrain(const Train& train);
    bool deleteTrain(int trainId);          // 逻辑删除（停运）

    // ---------- 搜索 ----------
    QVector<Train> searchTrains(const QString& keyword);
    QVector<Train> searchByStation(int stationId, bool isDeparture = true);

    // ---------- 座位管理 ----------
    bool updateRemainingSeats(int trainId, int delta);

private:
    DatabaseManager* m_dbManager;
    QString m_lastStatus;
    void setStatus(const QString& msg) { m_lastStatus = msg; }

    // ---------- 辅助转换函数 ----------
    Train convertToTrain(const struct TrainRecord& record) const;
    struct TrainRecord convertToRecord(const Train& train) const;
};