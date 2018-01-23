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

#include "bean/agv.h"
#include "bean/agvline.h"
#include "bean/agvstation.h"
#include "bean/agvtask.h"

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


//全局共有变量处理类实例
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


bool agvTaskLessThan( const AgvTask *a, const AgvTask *b );

unsigned char crc(unsigned char *data,int len);

#endif // GLOBAL_H
