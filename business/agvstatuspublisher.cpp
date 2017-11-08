#include "agvstatuspublisher.h"
#include <algorithm>
#include <sstream>
#include "util/global.h"

AgvStatusPublisher::AgvStatusPublisher(QObject *parent) : QThread(parent),isQuit(false)
{

}

AgvStatusPublisher::~AgvStatusPublisher()
{
    isQuit = true;
}

void AgvStatusPublisher::addSubscribe(int subscribe,int agvid)
{
    mutex.lock();
    subscribers.insert(subscribe,agvid);
    mutex.unlock();
}

void AgvStatusPublisher::removeSubscribe(int subscribe,int agvid)
{
    mutex.lock();
    for(QMap<int,int>::iterator itr = subscribers.begin();itr!=subscribers.end();++itr){
        if(itr.key()==subscribe && (agvid==0 || itr.value() == agvid)){
            subscribers.erase(itr);
        }
    }
    mutex.unlock();
}

void AgvStatusPublisher::run()
{
    while(!isQuit)
    {
        if(subscribers.size()==0||g_m_agvs.size()==0){
            QyhSleep(400);
            continue;
        }

        for(QMap<int,Agv *>::iterator itr = g_m_agvs.begin();itr!=g_m_agvs.end();++itr)
        {
            //组装订阅信息
            QMap<QString,QString> responseDatas;
            QList<QMap<QString,QString> > responseDatalists;

            responseDatas.insert(QString("type"),QString("agv"));
            responseDatas.insert(QString("todo"),QString("periodica"));

            responseDatas.insert(QString("id"),QString("%1").arg(itr.value()->id()));
            responseDatas.insert(QString("speed"),QString("%1").arg(itr.value()->speed()));
            responseDatas.insert(QString("turnSpeed"),QString("%1").arg(itr.value()->turnSpeed()));
            responseDatas.insert(QString("cpu"),QString("%1").arg(itr.value()->cpu()));
            responseDatas.insert(QString("status"),QString("%1").arg(itr.value()->status()));
            responseDatas.insert(QString("leftMotorStatus"),QString("%1").arg(itr.value()->leftMotorStatus()));
            responseDatas.insert(QString("rightMotorStatus"),QString("%1").arg(itr.value()->rightMotorStatus()));
            responseDatas.insert(QString("systemVoltage"),QString("%1").arg(itr.value()->systemVoltage()));
            responseDatas.insert(QString("systemCurrent"),QString("%1").arg(itr.value()->systemCurrent()));
            responseDatas.insert(QString("positionMagneticStripe"),QString("%1").arg(itr.value()->positionMagneticStripe()));
            responseDatas.insert(QString("frontObstruct"),QString("%1").arg(itr.value()->frontObstruct()));
            responseDatas.insert(QString("backObstruct"),QString("%1").arg(itr.value()->backObstruct()));
            responseDatas.insert(QString("currentOrder"),QString("%1").arg(itr.value()->currentOrder()));
            responseDatas.insert(QString("currentQueueNumber"),QString("%1").arg(itr.value()->currentQueueNumber()));
            responseDatas.insert(QString("ip"),QString("%1").arg(itr.value()->ip()));
            responseDatas.insert(QString("mileage"),QString("%1").arg(itr.value()->mileage()));
            responseDatas.insert(QString("rad"),QString("%1").arg(itr.value()->rad()));
            responseDatas.insert(QString("currentRfid"),QString("%1").arg(itr.value()->currentRfid()));

            QString xml = getResponseXml(responseDatas,responseDatalists);

            //发送订阅信息
            mutex.lock();
            //这个保存的是map< socket,agvId >
            for(QMap<int,int>::iterator pos = subscribers.begin();pos!=subscribers.end();++pos)
            {
                if(pos.value() == itr.key())
                    g_netWork->sendToOne(pos.key(),xml.toLocal8Bit().data(),xml.toLocal8Bit().length());
            }
            mutex.unlock();
        }

        QyhSleep(50);
    }
}
