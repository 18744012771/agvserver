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

        for(QMap<int,AgvAgent *>::iterator itr = g_m_agvs.begin();itr!=g_m_agvs.end();++itr)
        {
            QMap<QString,QString> responseData;
            responseData.insert(QString("id"),QString("%1").arg(itr.value()->id));
            responseData.insert(QString("name"),QString("%1").arg(itr.value()->name));
            responseData.insert(QString("ip"),QString("%1").arg(itr.value()->ip));
            responseData.insert(QString("port"),QString("%1").arg(itr.value()->port));
            responseData.insert(QString("mode"),QString("%1").arg(itr.value()->mode));
            responseData.insert(QString("mileage"),QString("%1").arg(itr.value()->mileage));
            responseData.insert(QString("currentRfid"),QString("%1").arg(itr.value()->currentRfid));
            responseData.insert(QString("current"),QString("%1").arg(itr.value()->current));
            responseData.insert(QString("voltage"),QString("%1").arg(itr.value()->voltage));
            responseData.insert(QString("positionMagneticStripe"),QString("%1").arg(itr.value()->positionMagneticStripe));
            responseData.insert(QString("pcbTemperature"),QString("%1").arg(itr.value()->pcbTemperature));
            responseData.insert(QString("motorTemperature"),QString("%1").arg(itr.value()->motorTemperature));
            responseData.insert(QString("cpu"),QString("%1").arg(itr.value()->cpu));
            responseData.insert(QString("speed"),QString("%1").arg(itr.value()->speed));
            responseData.insert(QString("angle"),QString("%1").arg(itr.value()->angle));
            responseData.insert(QString("error_no"),QString("%1").arg(itr.value()->error_no));
            responseData.insert(QString("currentQueueNumber"),QString("%1").arg(itr.value()->recvQueueNumber));
            responseData.insert(QString("orderCount"),QString("%1").arg(itr.value()->orderCount));
            responseData.insert(QString("nextRfid"),QString("%1").arg(itr.value()->nextRfid));
            responseData.insert(QString("status"),QString("%1").arg(itr.value()->status));
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
