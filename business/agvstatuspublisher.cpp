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

void AgvStatusPublisher::addSubscribe(int subscribe)
{
    mutex.lock();
    if(!subscribers.contains(subscribe))
        subscribers.append(subscribe);
    mutex.unlock();
}

void AgvStatusPublisher::removeSubscribe(int subscribe)
{
    mutex.lock();
    for(QList<int>::iterator itr = subscribers.begin();itr!=subscribers.end();++itr){
        if(*itr == subscribe){
            subscribers.erase(itr);
            break;
        }
    }
    mutex.unlock();
}

void AgvStatusPublisher::run()
{
    while(!isQuit)
    {
        if(subscribers.length()==0||g_m_agvs.size()==0){
            QyhSleep(400);
            continue;
        }

        //组装订阅信息
        QMap<QString,QString> responseDatas;
        QList<QMap<QString,QString> > responseDatalists;

        responseDatas.insert(QString("type"),QString("agv"));
        responseDatas.insert(QString("todo"),QString("periodica"));

        for(QMap<int,Agv *>::iterator itr = g_m_agvs.begin();itr!=g_m_agvs.end();++itr)
        {
            QMap<QString,QString> responseData;
            responseData.insert(QString("id"),QString("%1").arg(itr.value()->id));
            responseData.insert(QString("speed"),QString("%1").arg(itr.value()->speed));
            responseData.insert(QString("turnSpeed"),QString("%1").arg(itr.value()->turnSpeed));
            responseData.insert(QString("cpu"),QString("%1").arg(itr.value()->cpu));
            responseData.insert(QString("status"),QString("%1").arg(itr.value()->status));
            responseData.insert(QString("leftMotorStatus"),QString("%1").arg(itr.value()->leftMotorStatus));
            responseData.insert(QString("rightMotorStatus"),QString("%1").arg(itr.value()->rightMotorStatus));
            responseData.insert(QString("systemVoltage"),QString("%1").arg(itr.value()->systemVoltage));
            responseData.insert(QString("systemCurrent"),QString("%1").arg(itr.value()->systemCurrent));
            responseData.insert(QString("positionMagneticStripe"),QString("%1").arg(itr.value()->positionMagneticStripe));
            responseData.insert(QString("frontObstruct"),QString("%1").arg(itr.value()->frontObstruct));
            responseData.insert(QString("backObstruct"),QString("%1").arg(itr.value()->backObstruct));
            responseData.insert(QString("currentOrder"),QString("%1").arg(itr.value()->currentOrder));
            responseData.insert(QString("currentQueueNumber"),QString("%1").arg(itr.value()->currentQueueNumber));
            responseData.insert(QString("mileage"),QString("%1").arg(itr.value()->mileage));
            responseData.insert(QString("rad"),QString("%1").arg(itr.value()->rad));
            responseData.insert(QString("currentRfid"),QString("%1").arg(itr.value()->currentRfid));
            responseDatalists.append(responseData);
        }

        QString xml = getResponseXml(responseDatas,responseDatalists);

        //发送订阅信息
        mutex.lock();
        for(QList<int>::iterator itr = subscribers.begin();itr!=subscribers.end();++itr)
        {
                g_netWork->sendToOne(*itr,xml.toStdString().c_str(),xml.toStdString().length());
        }
        mutex.unlock();


        QyhSleep(500);
    }
}
