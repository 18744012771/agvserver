#include "agvlogprocess.h"
#include "util/global.h"

AgvLogProcess::AgvLogProcess(QObject *parent) : QThread(parent),isQuit(false)
{

}

AgvLogProcess::~AgvLogProcess()
{
    isQuit = true;
}

void AgvLogProcess::addSubscribe(int sock, int role)
{
    mutex.lock();
    subscribers.push_back(std::make_pair(sock,role));
    mutex.unlock();
}

void AgvLogProcess::removeSubscribe(int sock)
{
    mutex.lock();
    std::list<int>::iterator itr;
    for(std::list<std::pair<int,int> >::iterator itr = subscribers.begin();itr!=subscribers.end();++itr){
        if(itr->first == sock){
            subscribers.erase(itr);
            break;
        }
    }
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
        QString insertSql = "insert into agv_log(level,time,msg)values(?,?,?);";
        QStringList params;
        params<<QString("%1").arg(onelog.level)<<onelog.time.toString(DATE_TIME_FORMAT)<<QString::fromStdString( onelog.msg);

        g_sql->exec(insertSql,params);


        //组装订阅信息
        std::map<std::string,std::string> responseDatas;
        std::vector<std::map<std::string,std::string> > responseDatalists;
        responseDatas.insert(std::make_pair(std::string("type"),std::string("log")));
        responseDatas.insert(std::make_pair(std::string("todo"),std::string("periodica")));
        //组织内容
        std::string str_level;
        std::stringstream ss_level;
        ss_level<<onelog.level;
        ss_level>>str_level;

        std::string str_time;
        std::stringstream ss_time;
        ss_time<<onelog.time.toString(DATE_TIME_FORMAT).toStdString();
        ss_time>>str_time;


        responseDatas.insert(std::make_pair(std::string("level"),str_level));
        responseDatas.insert(std::make_pair(std::string("time"),str_time));
        responseDatas.insert(std::make_pair(std::string("msg"),onelog.msg));


        std::string xml = getResponseXml(responseDatas,responseDatalists);

        //发送订阅信息
        mutex.lock();
        for(std::list<std::pair<int,int> >::iterator itr = subscribers.begin();itr!=subscribers.end();++itr)
        {
            if(itr->second==0 && onelog.onlyAdminVisible)continue;
            g_netWork->sendToOne(itr->first,xml.c_str(),xml.length());
        }
        mutex.unlock();

        QyhSleep(20);
    }
}
