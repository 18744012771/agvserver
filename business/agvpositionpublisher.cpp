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
    if(!subscribers.contains(subscribe))
        subscribers.push_back(subscribe);
    mutex.unlock();
}

void AgvPositionPublisher::removeSubscribe(int subscribe)
{
    mutex.lock();
    subscribers.removeAll(subscribe);
//    int index = subscribers.indexOf(subscribe);
//    if(index>=0)
//        subscribers.removeAt(index);
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
        QMap<QString,QString> responseDatas;
        QList<QMap<QString,QString> > responseDatalists;

        responseDatas.insert(QString("type"),QString("map"));
        responseDatas.insert(QString("todo"),QString("periodica"));


        for(QMap<int,Agv *>::iterator itr = g_m_agvs.begin();itr!=g_m_agvs.end();++itr)
        {
            ///<id,x,y,rotation,status>
            QMap<QString,QString> mm;
            mm.insert(QString("x"),QString("%1").arg(itr.value()->x()));
            mm.insert(QString("y"),QString("%1").arg(itr.value()->y()));
            mm.insert(QString("id"),QString("%1").arg(itr.value()->id()));
            mm.insert(QString("rotation"),QString("%1").arg(itr.value()->rotation()));
            mm.insert(QString("status"),QString("%1").arg(itr.value()->status()));

            responseDatalists.push_back(mm);
        }
        QString xml = getResponseXml(responseDatas,responseDatalists);

        //发送订阅信息
        mutex.lock();
        for(QList<int>::iterator itr = subscribers.begin();itr!=subscribers.end();++itr)
        {
            g_netWork->sendToOne(*itr,xml.toStdString().c_str(),xml.toStdString().length());
        }
        mutex.unlock();

        QyhSleep(100);
    }
}
