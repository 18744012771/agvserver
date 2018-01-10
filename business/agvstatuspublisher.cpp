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

void AgvStatusPublisher::run()
{
    zmq::context_t context(1);
    zmq::socket_t publisher(context, ZMQ_PUB);

    std::string portStr = intToStdString(GLOBAL_PORT_AGV_STATUS);
    std::string url = "tcp://*:"+portStr;
    publisher.bind(url.c_str());

    publisher.bind("tcp://*:5563");

    while(!isQuit)
    {
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

        std::string xml = getResponseXml(responseDatas,responseDatalists);

        //发送订阅信息
        zmq::message_t message(xml.size());
        memcpy (message.data(), xml.data(), xml.size());
        publisher.send (message);
        QyhSleep(500);
    }
}
