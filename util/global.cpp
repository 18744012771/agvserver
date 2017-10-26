#include "global.h"

#include <QTime>
#include <QCoreApplication>

//全局的一些common变量
QString g_strExeRoot;
Sql *g_sql = NULL;
Log *g_log = NULL;
SqlServer *g_sqlServer = NULL;

//所有的bean集合
QMap<int,Agv *> g_m_agvs;             //所有车辆们
QMap<int,AgvStation *> g_m_stations;  //所有的站点(站点+线路 = 地图)
QMap<int,AgvLine *> g_m_lines;        //所有的线路(站点+线路 = 地图)
QMap<PATH_LEFT_MIDDLE_RIGHT,int> g_m_lmr;//用来保存左中右信息，用于通知agv左中右信息
QMap<int,QVector<AgvLine*> > g_m_l_adj;  //从一条线路到另一条线路的关联表。用来计算可到达的位置
QMap<int,int> g_reverseLines;           //线路和它的反方向线路的集合。

//所有的业务处理
MapCenter g_agvMapCenter;//地图管理(地图载入，地图保存，地图计算)
TaskCenter g_taskCenter;//任务管理(任务分配，任务保存，任务调度)
AgvCenter g_hrgAgvCenter;//车辆管理(车辆载入。车辆保存。车辆增加。车辆删除)
MsgCenter g_msgCenter;   //消息处理中心，对所有的消息进行解析和组装等

//公共函数
void QyhSleep(int msec)
{
    QTime dieTime = QTime::currentTime().addMSecs(msec);

    while( QTime::currentTime() < dieTime )
        QCoreApplication::processEvents(QEventLoop::AllEvents, 100);
}

int getRandom(int maxRandom)
{
    QTime t;
    t= QTime::currentTime();
    qsrand(t.msec()+t.second()*1000);
    if(maxRandom>0)
        return qrand()%maxRandom;
    return qrand();
}

