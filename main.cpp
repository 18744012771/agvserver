#include <QCoreApplication>
#include <QDir>
#include "util/global.h"

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);
    g_strExeRoot = QCoreApplication::applicationDirPath();
    QDir::setCurrent(g_strExeRoot);

    //初始化日志
    g_log = new AgvLog();
    g_logProcess = new AgvLogProcess();
    g_logProcess->start();

    //初始化数据库
    g_sql = new Sql();
    g_sql->createConnection();

    g_sqlServer = new SqlServer();
    g_sqlServer->createConnection();

    //初始化agv_center
    g_hrgAgvCenter.load();//载入车辆
    g_agvMapCenter.load();//地图路径中心
    g_taskCenter.init();//任务中心
    g_msgCenter.init();

    //用户管理中心

    //对外接口
    g_netWork = new AgvNetWork;
    g_netWork->initServer();


    //初始化服务器

    return a.exec();
}
