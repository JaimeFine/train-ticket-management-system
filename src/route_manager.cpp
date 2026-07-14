#include "route_manager.h"
#include "database_manager.h"
#include "database/train_record.h"
#include "database/station_record.h"

#include <algorithm>
#include <QDateTime>
#include <QDebug>
#include <QHash>
#include <QSet>
#include <QStringList>
#include <QTime>
#include <queue>

// ============================================================
// RoutePath 实现
// ============================================================
QString RoutePath::description() const
{
    if (stationIds.size() < 2) {
        return QStringLiteral("无有效路径");
    }

    QString desc;
    for (int i = 0; i < stationIds.size(); ++i) {
        if (i > 0) {
            desc += QStringLiteral(" → ");
        }
        desc += QString::number(stationIds[i]);
    }
    return desc + QStringLiteral("（共%1段行程，%2）")
                      .arg(trainIds.size())
                      .arg(timeString());
}

QString RoutePath::timeString() const
{
    int hours = totalTime / 60;
    int minutes = totalTime % 60;
    if (hours == 0) {
        return QString("%1分钟").arg(minutes);
    } else if (minutes == 0) {
        return QString("%1小时").arg(hours);
    } else {
        return QString("%1小时%2分钟").arg(hours).arg(minutes);
    }
}

// ============================================================
// RouteGraph 实现
// ============================================================
void RouteGraph::buildFromDatabase(DatabaseManager* dbManager)
{
    if (dbManager == nullptr) {
        qDebug() << "RouteGraph: DatabaseManager is null";
        return;
    }

    m_stationNames.clear();
    m_adjacency.clear();

    // 1. 加载站点
    QList<StationRecord> stations = dbManager->getAllStations();
    for (const StationRecord& station : stations) {
        m_stationNames[station.stationId] = station.stationName;
    }

    // 2. 加载车次并构建边
    QList<TrainRecord> trains = dbManager->getAllTrains(true);
    int edgeCount = 0;
    for (const TrainRecord& train : trains) {
        // 计算乘车时间（分钟）
        QDateTime dep = QDateTime::fromString(train.departureTime, "yyyy-MM-dd HH:mm");
        QDateTime arr = QDateTime::fromString(train.arrivalTime, "yyyy-MM-dd HH:mm");
        if (!dep.isValid() || !arr.isValid()) {
            dep = QDateTime::fromString(train.departureTime, "yyyy-MM-dd HH:mm:ss");
            arr = QDateTime::fromString(train.arrivalTime, "yyyy-MM-dd HH:mm:ss");
        }
        if (!dep.isValid() || !arr.isValid()) {
            qDebug() << "RouteGraph: 时间格式无效:" << train.departureTime << train.arrivalTime;
            continue;
        }

        int timeWeight = dep.secsTo(arr) / 60;
        if (timeWeight <= 0) timeWeight = 1;

        RouteEdge edge;
        edge.trainId = train.trainId;
        edge.trainNumber = train.trainNumber;
        edge.fromStationId = train.departureStationId;
        edge.toStationId = train.arrivalStationId;
        edge.timeWeight = timeWeight;
        edge.totalWeight = timeWeight;
        edge.departureTime = train.departureTime;
        edge.arrivalTime = train.arrivalTime;
        edge.remainingSeats = train.remainingSeats;

        m_adjacency[train.departureStationId].append(edge);
        ++edgeCount;
    }

    qDebug() << "RouteGraph: 构建完成，站点数=" << m_stationNames.size()
             << ", 边数=" << edgeCount;
}

QList<RouteEdge> RouteGraph::getEdgesFrom(int stationId) const
{
    return m_adjacency.value(stationId, QList<RouteEdge>());
}

QList<int> RouteGraph::getAllStationIds() const
{
    return m_stationNames.keys();
}

QString RouteGraph::getStationName(int stationId) const
{
    return m_stationNames.value(stationId, QString::number(stationId));
}

int RouteGraph::getStationCount() const
{
    return m_stationNames.size();
}

bool RouteGraph::isEmpty() const
{
    return m_stationNames.isEmpty() || m_adjacency.isEmpty();
}

// ============================================================
// RouteManager 实现
// ============================================================
RouteManager::RouteManager(DatabaseManager* dbManager)
    : m_dbManager(dbManager)
{
}

bool RouteManager::buildGraph()
{
    if (m_dbManager == nullptr) {
        setError("DatabaseManager 未初始化");
        return false;
    }

    m_graph.buildFromDatabase(m_dbManager);
    m_graphReady = !m_graph.isEmpty();
    if (!m_graphReady) {
        setError("无法构建线路图：无有效站点或车次");
    } else {
        clearError();
    }
    return m_graphReady;
}

// ============================================================
// 时间辅助函数
// ============================================================
QDateTime RouteManager::parseTime(const QString& timeStr, const QDate& baseDate) const
{
    // 尝试标准格式
    QDateTime dt = QDateTime::fromString(timeStr, "yyyy-MM-dd HH:mm");
    if (dt.isValid()) {
        dt.setDate(baseDate);
        return dt;
    }
    dt = QDateTime::fromString(timeStr, "yyyy-MM-dd HH:mm:ss");
    if (dt.isValid()) {
        dt.setDate(baseDate);
        return dt;
    }
    // 如果是 "HH:mm" 格式，直接拼接到基准日期
    QStringList parts = timeStr.split(':');
    if (parts.size() >= 2) {
        int hour = parts[0].toInt();
        int min = parts[1].toInt();
        if (hour >= 0 && hour < 24 && min >= 0 && min < 60) {
            return QDateTime(baseDate, QTime(hour, min));
        }
    }
    return QDateTime();
}

// ============================================================
// 核心算法：带时间窗换乘检查的 Dijkstra
// ============================================================
RoutePath RouteManager::findShortestPath(int fromStationId, int toStationId,
                                         OptimizationCriterion criterion,
                                         const QDateTime& startTime) const
{
    RoutePath emptyResult;

    if (!m_graphReady) {
        setError("线路图未构建或为空");
        return emptyResult;
    }

    if (fromStationId == toStationId) {
        RoutePath result;
        result.stationIds.append(fromStationId);
        result.totalWeight = 0;
        result.totalTime = 0;
        result.transferCount = 0;
        return result;
    }

    QList<int> allStations = m_graph.getAllStationIds();
    if (!allStations.contains(fromStationId) || !allStations.contains(toStationId)) {
        setError("站点不存在于线路图中");
        return emptyResult;
    }

    // 基准日期（从 startTime 取日期）
    QDate baseDate = startTime.date();

    // 数据结构
    QHash<int, int> bestScore;          // 当前优化目标下的最佳评分
    QHash<int, int> bestTotalTime;      // 到某站的最短总时间（分钟）
    QHash<int, int> bestTravelTime;     // 到某站的最短纯乘车时间
    QHash<int, int> bestTransferCount;  // 到某站的最短换乘次数
    QHash<int, QDateTime> bestArrivalTime;
    QHash<int, int> prevStation;
    QHash<int, int> prevTrain;
    QHash<int, QString> prevTrainNumber;
    QHash<int, int> prevSegmentTime;

    for (int stationId : allStations) {
        bestScore[stationId] = INT_MAX;
        bestTotalTime[stationId] = INT_MAX;
        bestTravelTime[stationId] = INT_MAX;
        bestTransferCount[stationId] = INT_MAX;
    }

    // 状态结构（小顶堆）
    struct State {
        int score;          // 当前优化目标对应的评分
        int totalTime;      // 总时间（分钟）
        int travelTime;     // 纯乘车时间
        int transfers;      // 换乘次数
        int legs;           // 已乘坐的车次数
        int stationId;
        QDateTime currentTime;
        bool operator>(const State& other) const {
            if (score != other.score) {
                return score > other.score;
            }
            if (totalTime != other.totalTime) {
                return totalTime > other.totalTime;
            }
            if (transfers != other.transfers) {
                return transfers > other.transfers;
            }
            return travelTime > other.travelTime;
        }
    };
    std::priority_queue<State, std::vector<State>, std::greater<State>> pq;

    // 起点初始化
    bestScore[fromStationId] = 0;
    bestTotalTime[fromStationId] = 0;
    bestTravelTime[fromStationId] = 0;
    bestTransferCount[fromStationId] = 0;
    bestArrivalTime[fromStationId] = startTime;
    pq.push({0, 0, 0, 0, 0, fromStationId, startTime});

    while (!pq.empty()) {
        State cur = pq.top();
        pq.pop();

        int u = cur.stationId;
        // 如果当前状态已经比记录差，跳过
        if (cur.score > bestScore[u]) continue;
        if (cur.score == bestScore[u] && cur.totalTime > bestTotalTime[u]) continue;
        if (cur.score == bestScore[u] && cur.totalTime == bestTotalTime[u]
            && cur.transfers > bestTransferCount[u]) continue;

        if (u == toStationId) break;

        QList<RouteEdge> edges = m_graph.getEdgesFrom(u);
        for (const RouteEdge& edge : edges) {
            QDateTime dep = parseTime(edge.departureTime, baseDate);
            QDateTime arr = parseTime(edge.arrivalTime, baseDate);
            if (!dep.isValid() || !arr.isValid()) {
                continue;
            }

            // 如果到达时间早于出发时间（跨天），补一天
            if (arr < dep) {
                arr = arr.addDays(1);
            }
            // 同样处理出发时间，如果出发时间早于当前日期时间，可能也是跨天
            if (dep < QDateTime(baseDate, QTime(0,0))) {
                dep = dep.addDays(1);
                arr = arr.addDays(1);
            }

            // 检查是否能赶上这班车
            QDateTime currentArrival = bestArrivalTime[u];
            int waitMinutes = 0;
            if (u == fromStationId) {
                // 起点：必须发车时间 >= startTime（或允许稍等）
                if (dep < startTime) {
                    // 如果发车时间已过，可以跨天（比如今天已发，明天再坐）
                    dep = dep.addDays(1);
                    arr = arr.addDays(1);
                }
                waitMinutes = startTime.secsTo(dep) / 60;
                if (waitMinutes < 0) {
                    // 如果还是负数，说明发车时间已过，跳过
                    continue;
                }
            } else {
                // 换乘站：必须发车时间 >= 当前到达时间（含换乘缓冲）
                int transferBuffer = 30;  // 最小换乘时间（分钟）
                QDateTime earliestDepart = currentArrival.addSecs(transferBuffer * 60);
                if (dep < earliestDepart) {
                    // 如果赶不上这班车，尝试同车次下一天（跨天）
                    dep = dep.addDays(1);
                    arr = arr.addDays(1);
                    if (dep < earliestDepart) {
                        continue;  // 仍然赶不上
                    }
                }
                waitMinutes = currentArrival.secsTo(dep) / 60;
            }

            int travelMinutes = dep.secsTo(arr) / 60;
            if (travelMinutes <= 0) travelMinutes = 1;

            // 计算新状态
            int newTotalTime = cur.totalTime + waitMinutes + travelMinutes;
            int newTravelTime = cur.travelTime + travelMinutes;
            int newLegs = cur.legs + 1;
            int newTransfers = std::max(0, newLegs - 1);

            // 根据优化目标计算综合权重（用于比较）
            int score = 0;
            switch (criterion) {
            case OptimizationCriterion::TimeFirst:
                score = newTotalTime;
                break;
            case OptimizationCriterion::TransferFirst:
                score = newTransfers;
                break;
            case OptimizationCriterion::Balanced:
                score = newTotalTime + newTransfers * 20;  // 换乘一次惩罚20分钟
                break;
            default:
                score = newTotalTime;
                break;
            }

            // 只保留更优的状态
            if (score < bestScore[edge.toStationId] ||
                (score == bestScore[edge.toStationId] && newTotalTime < bestTotalTime[edge.toStationId]) ||
                (score == bestScore[edge.toStationId] && newTotalTime == bestTotalTime[edge.toStationId]
                 && newTransfers < bestTransferCount[edge.toStationId]) ||
                (score == bestScore[edge.toStationId] && newTotalTime == bestTotalTime[edge.toStationId]
                 && newTransfers == bestTransferCount[edge.toStationId]
                 && newTravelTime < bestTravelTime[edge.toStationId])) {
                bestScore[edge.toStationId] = score;
                bestTotalTime[edge.toStationId] = newTotalTime;
                bestTravelTime[edge.toStationId] = newTravelTime;
                bestTransferCount[edge.toStationId] = newTransfers;
                bestArrivalTime[edge.toStationId] = arr;
                prevStation[edge.toStationId] = u;
                prevTrain[edge.toStationId] = edge.trainId;
                prevTrainNumber[edge.toStationId] = edge.trainNumber;
                prevSegmentTime[edge.toStationId] = travelMinutes;
                pq.push({score, newTotalTime, newTravelTime, newTransfers, newLegs, edge.toStationId, arr});
            }
        }
    }

    // 检查是否找到路径
    if (bestScore[toStationId] == INT_MAX) {
        setError("未找到可行的换乘路线");
        return emptyResult;
    }

    // 重建路径
    RoutePath result;
    int currentStation = toStationId;
    while (currentStation != fromStationId) {
        result.stationIds.prepend(currentStation);
        int prev = prevStation[currentStation];
        result.trainIds.prepend(prevTrain[currentStation]);
        result.trainNumbers.prepend(prevTrainNumber[currentStation]);
        result.segmentTimes.prepend(prevSegmentTime[currentStation]);
        currentStation = prev;
    }
    result.stationIds.prepend(fromStationId);

    // 计算各种时间
    result.totalWeight = bestScore[toStationId];
    result.totalTime = bestTotalTime[toStationId];
    result.travelTime = bestTravelTime[toStationId];
    result.waitTime = result.totalTime - result.travelTime;
    result.transferCount = bestTransferCount[toStationId];

    clearError();
    return result;
}

RoutePath RouteManager::recommendTransfer(int fromStationId, int toStationId,
                                          OptimizationCriterion criterion,
                                          const QDateTime& startTime) const
{
    return findShortestPath(fromStationId, toStationId, criterion, startTime);
}

bool RouteManager::isGraphReady() const
{
    return m_graphReady;
}

QString RouteManager::lastError() const
{
    return m_lastError;
}

int RouteManager::stationCount() const
{
    return m_graph.getStationCount();
}

void RouteManager::setError(const QString& msg) const
{
    m_lastError = msg;
}

void RouteManager::clearError() const
{
    m_lastError.clear();
}
