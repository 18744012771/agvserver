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

#include "bean/agvagent.h"
#include "bean/agvline.h"
#include "bean/agvstation.h"
#include "bean/task.h"

#include "business/agvcenter.h"
#include "business/mapcenter.h"
#include "business/taskcenter.h"
#include "business/usermsgprocessor.h"

#include "util/concurrentqueue.h"

#include "service/taskmaker.h"

//定义几个端口
//消息相应端口
#define GLOBAL_PORT_INTERFACE   5555
//日志publisher 端口
#define GLOBAL_PORT_LOG   5556
//agv状态publisher 端口
#define GLOBAL_PORT_AGV_STATUS   5557
//agv位置publisher 端口
#define GLOBAL_PORT_AGV_POSITION   5558
//任务状态publiser 端口
#define GLOBAL_PORT_TASK   5559

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
    PATH_LMF_NOWAY = -2,//代表可能要掉头行驶
    PATH_LMR_LEFT=-1,
    PATH_LMR_MIDDLE=0,
    PATH_LMR_RIGHT=1,
};


//全局共有变量处理类实例
extern Sql *g_sql;
extern AgvLog *g_log;
extern AgvLogProcess *g_logProcess;
extern QyhZmqServer *g_server;

//全局业务处理类实例
extern MapCenter *g_agvMapCenter;//地图路径中心
extern TaskCenter *g_taskCenter;//任务中心
extern AgvCenter *g_hrgAgvCenter;//车辆管理中心
extern UserMsgProcessor *userMsgProcessor;
extern TaskMaker *g_taskMaker;

//所有的bean集合
extern QMap<int,AgvAgent *> g_m_agvs;//车辆
extern QMap<int,AgvStation *> g_m_stations;//站点
extern QMap<int,AgvLine *> g_m_lines;//线路
extern QMap<PATH_LEFT_MIDDLE_RIGHT,int> g_m_lmr; //左中右
extern QMap<int,QList<AgvLine*> > g_m_l_adj;  //从一条线路到另一条线路的关联表
extern QMap<int,int> g_reverseLines;//线路和它的反方向线路的集合。

void QyhSleep(int msec);

int getRandom(int maxRandom);

std::string intToStdString(int x);

//用户消息的缓存区(用于拆包、分包)
extern QMap<int,std::string> client2serverBuffer;

////将结果封装成xml格式(解析-封装 的封装)
std::string getResponseXml(QMap<QString,QString> &responseDatas, QList<QMap<QString,QString> > &responseDatalists);

////将xml格式转成两个参数模式(解析-封装 的解析)
bool getRequestParam(const std::string &xmlStr, QMap<QString,QString> &params, QList<QMap<QString,QString> > &datalist);

extern const QString DATE_TIME_FORMAT;

//日志的消息队列
extern moodycamel::ConcurrentQueue<OneLog> g_log_queue;


bool agvTaskLessThan( const Task *a, const Task *b );

unsigned char crc(unsigned char *data,int len);

#endif // GLOBAL_H
