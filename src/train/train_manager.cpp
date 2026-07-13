#include "train_manager.h"
#include "database_manager.h"
#include "database/train_record.h"

#include <QDateTime>
#include <QRegularExpression>

// ============================================================
// 构造函数
// ============================================================
TrainManager::TrainManager(DatabaseManager* dbManager)
    : m_dbManager(dbManager)
{
}

// ============================================================
// 状态信息
// ============================================================
QString TrainManager::statusMessage() const
{
    return m_lastStatus;
}

DatabaseManager* TrainManager::databaseManager() const
{
    return m_dbManager;
}

// ============================================================
// 辅助：Train ↔ TrainRecord 转换
// ============================================================
Train TrainManager::convertToTrain(const TrainRecord& record) const
{
    Train t;
    t.trainId = record.trainId;
    t.trainNumber = record.trainNumber;
    t.departureStationId = record.departureStationId;
    t.arrivalStationId = record.arrivalStationId;
    t.departureTime = record.departureTime;
    t.arrivalTime = record.arrivalTime;
    t.totalSeats = record.totalSeats;
    t.remainingSeats = record.remainingSeats;
    t.enabled = record.enabled;
    return t;
}

TrainRecord TrainManager::convertToRecord(const Train& train) const
{
    TrainRecord record;
    record.trainId = train.trainId;
    record.trainNumber = train.trainNumber;
    record.departureStationId = train.departureStationId;
    record.arrivalStationId = train.arrivalStationId;
    record.departureTime = train.departureTime;
    record.arrivalTime = train.arrivalTime;
    record.totalSeats = train.totalSeats;
    record.remainingSeats = train.remainingSeats;
    record.enabled = train.enabled;
    return record;
}

// ============================================================
// 1. 获取所有车次
// ============================================================
QVector<Train> TrainManager::getAllTrains(bool onlyEnabled)
{
    QVector<Train> result;

    if (!m_dbManager) {
        setStatus("数据库管理器未初始化");
        return result;
    }

    // 实现代码（待 DatabaseManager 补充后取消注释）：
    auto records = m_dbManager->getAllTrains(onlyEnabled);
    for (const auto& record : records) {
        result.append(convertToTrain(record));
    }

    setStatus("查询成功，共 " + QString::number(result.size()) + " 条记录");
    return result;
}

// ============================================================
// 2. 添加车次
// ============================================================
bool TrainManager::addTrain(const Train& train)
{
    if (!m_dbManager) {
        setStatus("数据库管理器未初始化");
        return false;
    }

    // ---- 参数校验 ----
    if (train.trainNumber.trimmed().isEmpty()) {
        setStatus("车次号不能为空");
        return false;
    }

    if (train.departureStationId <= 0 || train.arrivalStationId <= 0) {
        setStatus("出发站和到达站必须有效");
        return false;
    }

    if (train.departureStationId == train.arrivalStationId) {
        setStatus("出发站和到达站不能相同");
        return false;
    }
    // 校验日期时间格式（yyyy-MM-dd HH:mm）
    QRegularExpression dateTimeRegex(R"(^\d{4}-\d{2}-\d{2} \d{2}:\d{2}$)");
    if (!dateTimeRegex.match(train.departureTime).hasMatch()) {
        setStatus("出发时间格式无效，请使用 yyyy-MM-dd HH:mm 格式");
        return false;
    }
    if (!dateTimeRegex.match(train.arrivalTime).hasMatch()) {
        setStatus("出发时间格式无效，请使用 yyyy-MM-dd HH:mm 格式");
        return false;
    }

    QDateTime dep = QDateTime::fromString(train.departureTime, "yyyy-MM-dd HH:mm");
    QDateTime arr = QDateTime::fromString(train.arrivalTime, "yyyy-MM-dd HH:mm");
    if (!dep.isValid() || !arr.isValid() || dep >= arr) {
        setStatus("出发时间必须早于到达时间，且格式正确");
        return false;
    }

    if (train.totalSeats <= 0) {
        setStatus("总座位数必须大于 0");
        return false;
    }

    if (train.remainingSeats < 0 || train.remainingSeats > train.totalSeats) {
        setStatus("剩余座位数必须在 0 到总座位数之间");
        return false;
    }

    // ---- 检查车次号是否已存在 ----
    // SQL: SELECT trainId FROM Train WHERE trainNumber = ?;
    auto existing = m_dbManager->findTrainByNumber(train.trainNumber);
    if (existing.has_value()) {
        setStatus("车次号 " + train.trainNumber + " 已存在");
        return false;
    }

    // ---- 插入新记录 ----
    // SQL: INSERT INTO Train (trainNumber, departureStationId, arrivalStationId,
    //                         departureTime, arrivalTime, totalSeats, remainingSeats, enabled)
    //      VALUES (?, ?, ?, ?, ?, ?, ?, ?);
    TrainRecord record = convertToRecord(train);
    if (!m_dbManager->addTrain(record)) {
        setStatus("添加车次失败: " + m_dbManager->lastError());
        return false;
    }

    setStatus("车次 " + train.trainNumber + " 添加成功");
    return true;
}

// ============================================================
// 3. 更新车次
// ============================================================
bool TrainManager::updateTrain(const Train& train)
{
    if (!m_dbManager) {
        setStatus("数据库管理器未初始化");
        return false;
    }

    // ---- 参数校验 ----
    if (train.trainId <= 0) {
        setStatus("无效的车次ID");
        return false;
    }

    if (train.trainNumber.trimmed().isEmpty()) {
        setStatus("车次号不能为空");
        return false;
    }

    if (train.departureStationId <= 0 || train.arrivalStationId <= 0) {
        setStatus("出发站和到达站必须有效");
        return false;
    }

    if (train.departureStationId == train.arrivalStationId) {
        setStatus("出发站和到达站不能相同");
        return false;
    }

    QRegularExpression dateTimeRegex(R"(^\d{4}-\d{2}-\d{2} \d{2}:\d{2}$)");
    if (!dateTimeRegex.match(train.departureTime).hasMatch()) {
        setStatus("出发时间格式无效，请使用 yyyy-MM-dd HH:mm 格式");
        return false;
    }
    if (!dateTimeRegex.match(train.arrivalTime).hasMatch()) {
        setStatus("出发时间格式无效，请使用 yyyy-MM-dd HH:mm 格式");
        return false;
    }

    QDateTime dep = QDateTime::fromString(train.departureTime, "yyyy-MM-dd HH:mm");
    QDateTime arr = QDateTime::fromString(train.arrivalTime, "yyyy-MM-dd HH:mm");
    if (!dep.isValid() || !arr.isValid() || dep >= arr) {
        setStatus("出发时间必须早于到达时间，且格式正确");
        return false;
    }

    if (train.totalSeats <= 0) {
        setStatus("总座位数必须大于 0");
        return false;
    }

    if (train.remainingSeats < 0 || train.remainingSeats > train.totalSeats) {
        setStatus("剩余座位数必须在 0 到总座位数之间");
        return false;
    }

    // ---- 检查车次是否存在 ----
    // SQL: SELECT trainId FROM Train WHERE trainId = ?;
    auto existing = m_dbManager->findTrainById(train.trainId);
    if (!existing.has_value()) {
        setStatus("未找到要更新的车次 (ID: " + QString::number(train.trainId) + ")");
        return false;
    }

    // ---- 检查车次号是否被其他记录占用 ----
    // SQL: SELECT trainId FROM Train WHERE trainNumber = ? AND trainId != ?;
    auto duplicate = m_dbManager->findTrainByNumber(train.trainNumber);
    if (duplicate.has_value() && duplicate->trainId != train.trainId) {
        setStatus("车次号 " + train.trainNumber + " 已被其他车次使用");
        return false;
    }

    // ---- 更新记录 ----
    // SQL: UPDATE Train SET
    //        trainNumber = ?,
    //        departureStationId = ?,
    //        arrivalStationId = ?,
    //        departureTime = ?,
    //        arrivalTime = ?,
    //        totalSeats = ?,
    //        remainingSeats = ?,
    //        enabled = ?
    //      WHERE trainId = ?;
    TrainRecord record = convertToRecord(train);
    if (!m_dbManager->updateTrain(record)) {
        setStatus("更新车次失败: " + m_dbManager->lastError());
        return false;
    }

    setStatus("车次 " + train.trainNumber + " 更新成功");
    return true;
}

// ============================================================
// 4. 逻辑删除（停运）
// ============================================================
bool TrainManager::deleteTrain(int trainId)
{
    if (!m_dbManager) {
        setStatus("数据库管理器未初始化");
        return false;
    }

    if (trainId <= 0) {
        setStatus("无效的车次ID");
        return false;
    }

    if (!m_dbManager->deleteTrain(trainId)) {
        setStatus("停运车次失败: " + m_dbManager->lastError());
        return false;
    }

    setStatus("车次已停运");
    return true;
}

// ============================================================
// 5. 按关键字搜索
// ============================================================
QVector<Train> TrainManager::searchTrains(const QString& keyword)
{
    QVector<Train> result;

    if (!m_dbManager) {
        setStatus("数据库管理器未初始化");
        return result;
    }

    if (keyword.trimmed().isEmpty()) {
        setStatus("搜索关键字不能为空");
        return result;
    }

    auto records = m_dbManager->searchTrains(keyword);
    for (const auto& record : records) {
        result.append(convertToTrain(record));
    }

    setStatus("搜索完成，找到 " + QString::number(result.size()) + " 条匹配记录");
    return result;
}

// ============================================================
// 6. 按车站搜索
// ============================================================
QVector<Train> TrainManager::searchByStation(int stationId, bool isDeparture)
{
    QVector<Train> result;

    if (!m_dbManager) {
        setStatus("数据库管理器未初始化");
        return result;
    }

    if (stationId <= 0) {
        setStatus("无效的车站ID");
        return result;
    }

    auto records = m_dbManager->searchByStation(stationId, isDeparture);
    for (const auto& record : records) {
        result.append(convertToTrain(record));
    }

    QString direction = isDeparture ? "出发" : "到达";
    setStatus("按" + direction + "站查询完成，共 " + QString::number(result.size()) + " 条记录");
    return result;
}

// ============================================================
// 7. 更新剩余座位
// ============================================================
bool TrainManager::updateRemainingSeats(int trainId, int delta)
{
    if (!m_dbManager) {
        setStatus("数据库管理器未初始化");
        return false;
    }

    if (trainId <= 0) {
        setStatus("无效的车次ID");
        return false;
    }

    if (delta == 0) {
        setStatus("变动数量不能为 0");
        return false;
    }

    if (!m_dbManager->adjustTrainSeats(trainId, delta)) {
        setStatus("更新余票失败: " + m_dbManager->lastError());
        return false;
    }

    setStatus("余票更新成功");
    return true;
}
