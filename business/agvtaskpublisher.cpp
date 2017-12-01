#include "agvtaskpublisher.h"
#include <algorithm>
#include <sstream>
#include "util/global.h"
AgvTaskPublisher::AgvTaskPublisher(QObject *parent) : QThread(parent),isQuit(false)
{

}

AgvTaskPublisher::~AgvTaskPublisher()
{
    isQuit = true;
}

void AgvTaskPublisher::addSubscribe(int subscribe)
{
    mutex.lock();
    if(!subscribers.contains(subscribe))
        subscribers.push_back(subscribe);
    mutex.unlock();
}

void AgvTaskPublisher::removeSubscribe(int subscribe)
{
    mutex.lock();
    subscribers.removeAll(subscribe);
    mutex.unlock();
}

void AgvTaskPublisher::run()
{
    while(!isQuit){
        if(subscribers.size()==0){//没有订阅者或者没有车辆
            QyhSleep(500);
            continue;
        }

        //组装订阅信息
        QMap<QString,QString> responseDatas;
        QList<QMap<QString,QString> > responseDatalists;

        responseDatas.insert(QString("type"),QString("task"));
        responseDatas.insert(QString("todo"),QString("periodica"));

        QList<AgvTask *> undos =  g_taskCenter.getUnassignedTasks();
        QList<AgvTask *> doings = g_taskCenter.getDoingTasks();

        for(int i=0;i<undos.length();++i){
            QMap<QString,QString> mm;
            AgvTask *t = undos.at(i);
            if(t!=NULL){
                mm.insert(QString("status"),QString("%1").arg(t->status()));
                mm.insert(QString("excutecar"),QString("%1").arg(t->excuteCar()));
                mm.insert(QString("id"),QString("%1").arg(t->id()));
                responseDatalists.push_back(mm);
            }
        }
        for(int i=0;i<doings.length();++i)
        {
            QMap<QString,QString> mm;
            AgvTask *t = doings.at(i);
            if(t!=NULL)
            {
                mm.insert(QString("status"),QString("%1").arg(t->status()));
                mm.insert(QString("excutecar"),QString("%1").arg(t->excuteCar()));
                mm.insert(QString("id"),QString("%1").arg(t->id()));
                responseDatalists.push_back(mm);
            }
        }

        QString xml = getResponseXml(responseDatas,responseDatalists);

        //发送订阅信息
        mutex.lock();
        for(QList<int>::iterator itr = subscribers.begin();itr!=subscribers.end();++itr)
        {
            g_netWork->sendToOne(*itr,xml.toStdString().c_str(),xml.toStdString().length());
        }
        mutex.unlock();

        QyhSleep(700);
    }
}
