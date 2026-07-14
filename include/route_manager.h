#ifndef ROUTE_MANAGER_H
#define ROUTE_MANAGER_H

#include <QList>
#include <QMap>
#include <QString>
#include <QDateTime>
#include <QVector>
#include <climits>

class DatabaseManager;
class TrainRecord;
class StationRecord;

// 边权重计算模式
enum class OptimizationCriterion {
    TimeFirst,      // 时间最短（含换乘等待）
    TransferFirst,  // 换乘最少
    Balanced        // 综合平衡
};

// 边结构（从车次直达关系生成）
struct RouteEdge {
    int trainId;
    QString trainNumber;
    int fromStationId;
    int toStationId;
    int timeWeight;         // 实际乘车时间（分钟）
    int totalWeight;        // 根据条件计算的综合权重（时间/换乘/综合）
    QString departureTime;  // 出发时间 "HH:mm" 或 "yyyy-MM-dd HH:mm"
    QString arrivalTime;    // 到达时间
    int remainingSeats;
};

// 路径结果
struct RoutePath {
    QList<int> stationIds;      // 站点序列（ID）
    QList<int> trainIds;        // 车次序列（ID）
    int totalWeight = 0;        // 综合权重
    int totalTime = 0;          // 实际总出行时间（分钟，含换乘等待）
    int travelTime = 0;         // 纯乘车时间（分钟）
    int waitTime = 0;           // 换乘等待时间（分钟）
    int transferCount = 0;      // 换乘次数

    QString description() const;
    QString timeString() const;
};

// 路线图类
class RouteGraph {
public:
    void buildFromDatabase(DatabaseManager* dbManager);
    QList<RouteEdge> getEdgesFrom(int stationId) const;
    QList<int> getAllStationIds() const;
    QString getStationName(int stationId) const;
    int getStationCount() const;
    bool isEmpty() const;

private:
    QMap<int, QString> m_stationNames;
    QMap<int, QList<RouteEdge>> m_adjacency;
};

// 路线管理器
class RouteManager {
public:
    explicit RouteManager(DatabaseManager* dbManager = nullptr);

    // 构建图结构
    bool buildGraph();

    // 最短路径查询（带优化条件）
    RoutePath findShortestPath(int fromStationId, int toStationId,
                               OptimizationCriterion criterion = OptimizationCriterion::TimeFirst,
                               const QDateTime& startTime = QDateTime::currentDateTime()) const;

    // 推荐换乘路径
    RoutePath recommendTransfer(int fromStationId, int toStationId,
                                OptimizationCriterion criterion = OptimizationCriterion::TimeFirst,
                                const QDateTime& startTime = QDateTime::currentDateTime()) const;

    // 获取状态
    bool isGraphReady() const;
    QString lastError() const;
    int stationCount() const;

private:
    DatabaseManager* m_dbManager;
    RouteGraph m_graph;
    bool m_graphReady = false;
    mutable QString m_lastError;

    void setError(const QString& msg) const;
    void clearError() const;

    // 时间辅助函数：将时间字符串解析为 QDateTime（基于基准日期）
    QDateTime parseTime(const QString& timeStr, const QDate& baseDate) const;
};

#endif // ROUTE_MANAGER_H