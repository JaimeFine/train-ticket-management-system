#include "route_manager.h"
#include "database_manager.h"

#include <QCoreApplication>
#include <QDebug>

int main(int argc, char* argv[])
{
    QCoreApplication app(argc, argv);

    DatabaseManager dbManager;
    if (!dbManager.initialize()) {
        qCritical() << "数据库初始化失败:" << dbManager.lastError();
        return 1;
    }

    RouteManager routeManager(&dbManager);
    if (!routeManager.buildGraph()) {
        qCritical() << "构建线路图失败:" << routeManager.lastError();
        return 1;
    }

    // 假设站点 ID 1 -> 2
    RoutePath path = routeManager.recommendTransfer(1, 2);
    qDebug() << "路径描述:" << path.description();
    qDebug() << "总权重:" << path.totalWeight << "分钟";
    qDebug() << "站点序列:" << path.stationIds;
    qDebug() << "车次序列:" << path.trainIds;

    return 0;
}