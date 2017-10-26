#ifndef GLOBAL_H
#define GLOBAL_H

#include <QDebug>
#include <QList>
#include <QMap>

#include "sql/sql.h"
#include "log/log.h"
#include "concurrentqueue.h"
#include "sql/sqlserver.h"

#include "business/agv.h"
#include "business/agvline.h"
#include "business/agvstation.h"
#include "business/agvtask.h"

#include "business/agvcenter.h"
#include "business/mapcenter.h"
#include "business/taskcenter.h"
#include "business/msgcenter.h"

#define qyhLog (qDebug())
#define endll ("")


struct QyhDataItem
{
    void *data;
    int size;
};

struct PATH_LEFT_MIDDLE_RIGHT{
    int lastLine;
    int nextLine;

    bool operator == (const PATH_LEFT_MIDDLE_RIGHT &r){
        return lastLine == r.lastLine && nextLine == r.nextLine;
    }

    bool operator < (const PATH_LEFT_MIDDLE_RIGHT &r) const
    {
        if(lastLine!=r.lastLine){
            return lastLine<r.lastLine;
        }

        return nextLine<r.nextLine;
    }
};

enum{
    PATH_LMF_NOWAY = -2,//代表可能要掉头行驶，这个在自动模式下不可取？？？？？？？//TODO
    PATH_LMR_LEFT=-1,
    PATH_LMR_MIDDLE=0,
    PATH_LMR_RIGHT=1,
};

//全局共有变量处理类实例
extern QString g_strExeRoot;
extern Sql *g_sql;
extern Log *g_log;
extern SqlServer *g_sqlServer;

//全局业务处理类实例
extern MapCenter g_agvMapCenter;//地图路径中心
extern TaskCenter g_taskCenter;//任务中心
extern AgvCenter g_hrgAgvCenter;//车辆管理中心
extern MsgCenter g_msgCenter;

//bean的容器
extern QMap<int,Agv *> g_m_agvs;//车辆
extern QMap<int,AgvStation *> g_m_stations;//站点
extern QMap<int,AgvLine *> g_m_lines;//线路
extern QMap<PATH_LEFT_MIDDLE_RIGHT,int> g_m_lmr; //左中右
extern QMap<int,QVector<AgvLine*> > g_m_l_adj;  //从一条线路到另一条线路的关联表
extern QMap<int,int> g_reverseLines;//线路和它的反方向线路的集合。

void QyhSleep(int msec);

int getRandom(int maxRandom);

#endif // GLOBAL_H
