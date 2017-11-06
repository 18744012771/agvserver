#include "agvlogpublisher.h"
#include <algorithm>
#include <sstream>
#include "util/global.h"


AgvLogPublisher::AgvLogPublisher(QObject *parent) : QThread(parent),isQuit(false)
{

}

AgvLogPublisher::~AgvLogPublisher()
{
    isQuit = true;
}

void AgvLogPublisher::addSubscribe(int subscribe)
{
    mutex.lock();
    subscribers.push_back(subscribe);
    mutex.unlock();
}

void AgvLogPublisher::removeSubscribe(int subscribe)
{
    mutex.lock();
    std::list<int>::iterator itr;
    if((itr = std::find(subscribers.begin(),subscribers.end(),subscribe))!=subscribers.end())
        subscribers.erase(itr);
    mutex.unlock();
}

void AgvLogPublisher::run()
{
    while(!isQuit){
        if(subscribers.size()==0||g_m_agvs.size()==0){//没有订阅者或者没有车辆
            QyhSleep(400);
            continue;
        }

        //组装订阅信息
        std::map<std::string,std::string> responseDatas;
        std::vector<std::map<std::string,std::string> > responseDatalists;

        responseDatas.insert(std::make_pair(std::string("type"),std::string("log")));
        responseDatas.insert(std::make_pair(std::string("todo"),std::string("periodica")));

        //组织内容
        //TODO

        std::string xml = getResponseXml(responseDatas,responseDatalists);

        //发送订阅信息
        mutex.lock();
        for(std::list<int>::iterator itr = subscribers.begin();itr!=subscribers.end();++itr)
        {
            g_netWork->sendToOne(*itr,xml.c_str(),xml.length());
        }
        mutex.unlock();

        QyhSleep(100);
    }
}
