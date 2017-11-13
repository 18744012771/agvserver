#include "agvlogprocess.h"
#include "util/global.h"
#include <sstream>

AgvLogProcess::AgvLogProcess(QObject *parent) : QThread(parent),isQuit(false)
{

}

AgvLogProcess::~AgvLogProcess()
{
    isQuit = true;
}

void AgvLogProcess::addSubscribe(int sock, SubNode node)
{
    mutex.lock();
    subscribers.insert(sock,node);
    mutex.unlock();
}

void AgvLogProcess::removeSubscribe(int sock)
{
    mutex.lock();
    subscribers.remove(sock);
    mutex.unlock();
}

void AgvLogProcess::run()
{
    //处理日志队列的消息
    //处理方法如下:
    //取出消息，存入数据库
    //并且看有没有订阅，如果有发送
    while(true)
    {
        OneLog onelog;
        if(!g_log_queue.try_dequeue(onelog))
        {
            QyhSleep(500);
            continue;
        }

        //保存到数据库
        QString insertSql = "insert into agv_log(log_level,log_time,log_msg)values(?,?,?);;SELECT @@Identity;";
        QStringList params;
        params<<QString("%1").arg(onelog.level)<<onelog.time.toString(DATE_TIME_FORMAT)<<onelog.msg;

        QList<QStringList> result;

        if(g_sql)
             result =  g_sql->query(insertSql,params);
        if(result.length()>0 && result.at(0).length()>0){
            int id = result.at(0).at(0).toInt();
            //组装订阅信息
            QMap<QString,QString> responseDatas;
            QList<QMap<QString,QString> > responseDatalists;
            responseDatas.insert(QString("type"),QString("log"));
            responseDatas.insert(QString("todo"),QString("periodica"));
            //组织内容
            responseDatas.insert(QString("level"),QString("%1").arg(onelog.level));
            responseDatas.insert(QString("time"),onelog.time.toString(DATE_TIME_FORMAT));
            responseDatas.insert(QString("msg"),onelog.msg);
            responseDatas.insert(QString("id"),QString("%1").arg(id));


            QString xml = getResponseXml(responseDatas,responseDatalists);

            //发送订阅信息
            mutex.lock();
            for(QMap<int,SubNode>::iterator itr = subscribers.begin();itr!=subscribers.end();++itr)
            {
                if(onelog.level==AGV_LOG_LEVEL_TRACE &&!itr.value().trace)continue;
                if(onelog.level==AGV_LOG_LEVEL_DEBUG &&!itr.value().debug)continue;
                if(onelog.level==AGV_LOG_LEVEL_INFO &&!itr.value().info)continue;
                if(onelog.level==AGV_LOG_LEVEL_WARN &&!itr.value().warn)continue;
                if(onelog.level==AGV_LOG_LEVEL_ERROR &&!itr.value().error)continue;
                if(onelog.level==AGV_LOG_LEVEL_FATAL &&!itr.value().fatal)continue;
                g_netWork->sendToOne(itr.key(),xml.toStdString().c_str(),xml.toStdString().length());
            }
            mutex.unlock();
        }


        QyhSleep(10);
    }
}
