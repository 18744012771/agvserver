#include <QCoreApplication>
#include <QDir>
#include "util/global.h"

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);
    QString g_strExeRoot = QCoreApplication::applicationDirPath();
    QDir::setCurrent(g_strExeRoot);

    //初始化日志
    g_log = new AgvLog();
    g_logProcess = new AgvLogProcess();
    g_logProcess->start();

    //初始化数据库
    g_sql = new Sql();
    g_sql->createConnection();

    //初始化agv_center
    g_hrgAgvCenter = new AgvCenter;
    g_hrgAgvCenter->init();//载入车辆


    g_agvMapCenter->load();//地图路径中心

    g_taskCenter = new TaskCenter;
    g_taskCenter->init();//任务中心

    userMsgProcessor = new UserMsgProcessor;//消息处理

    g_server = new QyhZmqServer;
    std::thread(std::bind(&QyhZmqServer::run, g_server)).detach();//zmq server

    //启动一个独立线程，执行任务产生和完成回调
    g_taskMaker = new TaskMaker;
    if(!g_taskMaker->init())
    {
        g_log->log(AGV_LOG_LEVEL_ERROR,"task make fail init,check your sqlserver connection!");
    }

    return a.exec();
}
