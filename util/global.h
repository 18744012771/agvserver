#ifndef GLOBAL_H
#define GLOBAL_H

#include <QList>
#include <QMap>
#include <QString>
#include <QMutex>
#include <QDebug>
#include <QVariant>

#include "sql/sql.h"
#include "log/agvlog.h"
#include "log/agvlogprocess.h"
#include "concurrentqueue.h"
#include "sql/sqlserver.h"
#include "network/qyhzmqserver.h"

#include "business/agv.h"
#include "business/agvline.h"
#include "business/agvstation.h"
#include "business/agvtask.h"

#include "business/agvcenter.h"
#include "business/mapcenter.h"
#include "business/taskcenter.h"
//#include "business/msgcenter.h"
#include "business/usermsgprocessor.h"

#include "util/concurrentqueue.h"

#include "service/taskmaker.h"


struct QyhMsgDateItem
{
    std::string data;
    QString ip;//发送方IP
    int port;//发送方端口
    SOCKET sock;
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

//全局共有变量处理类实例
extern QString g_strExeRoot;
extern Sql *g_sql;
extern AgvLog *g_log;
extern AgvLogProcess *g_logProcess;
extern QyhZmqServer *g_server;

//全局业务处理类实例
extern MapCenter g_agvMapCenter;//地图路径中心
extern TaskCenter g_taskCenter;//任务中心
extern AgvCenter g_hrgAgvCenter;//车辆管理中心
extern UserMsgProcessor *userMsgProcessor;
extern TaskMaker *g_taskMaker;

//bean的容器
extern QMap<int,Agv *> g_m_agvs;//车辆
extern QMap<int,AgvStation *> g_m_stations;//站点
extern QMap<int,AgvLine *> g_m_lines;//线路
extern QMap<PATH_LEFT_MIDDLE_RIGHT,int> g_m_lmr; //左中右
extern QMap<int,QList<AgvLine*> > g_m_l_adj;  //从一条线路到另一条线路的关联表
extern QMap<int,int> g_reverseLines;//线路和它的反方向线路的集合。

void QyhSleep(int msec);

int getRandom(int maxRandom);

//用户消息的缓存区(用于拆包、分包)
extern QMap<int,std::string> client2serverBuffer;

////将结果封装成xml格式(解析-封装 的封装)
std::string getResponseXml(QMap<QString,QString> &responseDatas, QList<QMap<QString,QString> > &responseDatalists);

////将xml格式转成两个参数模式(解析-封装 的解析)
bool getRequestParam(const std::string &xmlStr, QMap<QString,QString> &params, QList<QMap<QString,QString> > &datalist);



extern const QString DATE_TIME_FORMAT;

//日志的消息队列
extern moodycamel::ConcurrentQueue<OneLog> g_log_queue;

#endif // GLOBAL_H
