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


void AgvPositionPublisher::run()
{
    zmq::context_t context(1);
    zmq::socket_t publisher(context, ZMQ_PUB);
    std::string portStr = intToStdString(GLOBAL_PORT_AGV_POSITION);
    std::string url = "tcp://*:"+portStr;
    publisher.bind(url.c_str());

    while(!isQuit){
        //组装订阅信息
        QMap<QString,QString> responseDatas;
        QList<QMap<QString,QString> > responseDatalists;

        responseDatas.insert(QString("type"),QString("map"));
        responseDatas.insert(QString("todo"),QString("periodica"));

        for(QMap<int,AgvAgent *>::iterator itr = g_m_agvs.begin();itr!=g_m_agvs.end();++itr)
        {
            QMap<QString,QString> mm;
            mm.insert(QString("x"),QString("%1").arg(itr.value()->x));
            mm.insert(QString("y"),QString("%1").arg(itr.value()->y));
            mm.insert(QString("id"),QString("%1").arg(itr.value()->id));
            mm.insert(QString("name"),QString("%1").arg(itr.value()->name));
            mm.insert(QString("rotation"),QString("%1").arg(itr.value()->rotation));
            mm.insert(QString("status"),QString("%1").arg(itr.value()->status));

            responseDatalists.push_back(mm);
        }
        std::string xml = getResponseXml(responseDatas,responseDatalists);

        //发送订阅信息
        zmq::message_t message(xml.size());
        memcpy (message.data(), xml.data(), xml.size());
        publisher.send (message);
        QyhSleep(100);
    }
}
