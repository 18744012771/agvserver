#include "agvpositionpublisher.h"
#include <algorithm>
#include <sstream>
#include "util/global.h"


AgvPositionPublisher::AgvPositionPublisher(QObject *parent) : QThread(parent),isQuit(false)
{

}

AgvPositionPublisher::~AgvPositionPublisher()
{
    isQuit = true;
}

void AgvPositionPublisher::addSubscribe(int subscribe)
{
    mutex.lock();
    subscribers.push_back(subscribe);
    mutex.unlock();
}

void AgvPositionPublisher::removeSubscribe(int subscribe)
{
    mutex.lock();
    std::list<int>::iterator itr;
    if((itr = std::find(subscribers.begin(),subscribers.end(),subscribe))!=subscribers.end())
        subscribers.erase(itr);
    mutex.unlock();
}

void AgvPositionPublisher::run()
{
    while(!isQuit){
        if(subscribers.size()==0||g_m_agvs.size()==0){//没有订阅者或者没有车辆
            QyhSleep(400);
            continue;
        }

        //组装订阅信息
        std::map<std::string,std::string> responseDatas;
        std::vector<std::map<std::string,std::string> > responseDatalists;

        responseDatas.insert(std::make_pair(std::string("type"),std::string("map")));
        responseDatas.insert(std::make_pair(std::string("todo"),std::string("periodica")));


        for(QMap<int,Agv *>::iterator itr = g_m_agvs.begin();itr!=g_m_agvs.end();++itr)
        {
            ///<id,x,y,rotation,status>
            std::map<std::string,std::string> mm;

            std::stringstream ss_x;
            std::string str_x;
            ss_x<<itr.value()->x();
            ss_x>>str_x;
            mm.insert(std::make_pair(std::string("x"),str_x));

            std::stringstream ss_y;
            std::string str_y;
            ss_y<<itr.value()->y();
            ss_y>>str_y;
            mm.insert(std::make_pair(std::string("y"),str_y));

            std::stringstream ss_id;
            std::string str_id;
            ss_id<<itr.value()->id();
            ss_id>>str_id;
            mm.insert(std::make_pair(std::string("id"),str_id));

            std::stringstream ss_rotation;
            std::string str_rotation;
            ss_rotation<<itr.value()->rotation();
            ss_rotation>>str_rotation;
            mm.insert(std::make_pair(std::string("rotation"),str_rotation));

            std::stringstream ss_status;
            std::string str_status;
            ss_status<<itr.value()->status();
            ss_status>>str_status;
            mm.insert(std::make_pair(std::string("status"),str_status));

            responseDatalists.push_back(mm);
        }
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
