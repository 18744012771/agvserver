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

void AgvLogProcess::run()
{
    zmq::context_t context(1);
    zmq::socket_t publisher(context, ZMQ_PUB);

    std::string portStr = intToStdString(GLOBAL_PORT_LOG);
    std::string url = "tcp://*:"+portStr;
    publisher.bind(url.c_str());

    //处理日志队列的消息
    //处理方法如下:
    //取出消息，存入数据库
    //并且看有没有订阅，如果有发送
    while(true)
    {
        OneLog onelog;
        if(!g_log_queue.try_dequeue(onelog))
        {
            QyhSleep(50);
            continue;
        }

        //保存到数据库
        QString insertSql = "insert into agv_log(log_level,log_time,log_msg)values(?,?,?);SELECT @@Identity;";
        QList<QVariant> params;
        params<<onelog.level<<onelog.time<<onelog.msg;

        QList<QList<QVariant> > result;

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

            std::string xml = getResponseXml(responseDatas,responseDatalists);

            //发送订阅信息
            zmq::message_t message(xml.size());
            memcpy (message.data(), xml.data(), xml.size());
            publisher.send (message);
        }

        QyhSleep(20);
    }
}
