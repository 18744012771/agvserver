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

void AgvTaskPublisher::run()
{
    zmq::context_t context(1);
    zmq::socket_t publisher(context, ZMQ_PUB);


    std::string portStr = intToStdString(GLOBAL_PORT_TASK);
    std::string url = "tcp://*:"+portStr;
    publisher.bind(url.c_str());

    while(!isQuit){
        //组装订阅信息
        QMap<QString,QString> responseDatas;
        QList<QMap<QString,QString> > responseDatalists;

        responseDatas.insert(QString("type"),QString("task"));
        responseDatas.insert(QString("todo"),QString("periodica"));

        QList<Task *> undos =  g_taskCenter->getUnassignedTasks();
        QList<Task *> doings = g_taskCenter->getDoingTasks();

        for(int i=0;i<undos.length();++i){
            QMap<QString,QString> mm;
            Task *t = undos.at(i);
            if(t!=NULL){
                mm.insert(QString("status"),QString("%1").arg(t->status));
                mm.insert(QString("excutecar"),QString("%1").arg(t->excuteCar));
                mm.insert(QString("id"),QString("%1").arg(t->id));
                responseDatalists.push_back(mm);
            }
        }
        for(int i=0;i<doings.length();++i)
        {
            QMap<QString,QString> mm;
            Task *t = doings.at(i);
            if(t!=NULL)
            {
                mm.insert(QString("status"),QString("%1").arg(t->status));
                mm.insert(QString("excutecar"),QString("%1").arg(t->excuteCar));
                mm.insert(QString("id"),QString("%1").arg(t->id));
                responseDatalists.push_back(mm);
            }
        }

        std::string xml = getResponseXml(responseDatas,responseDatalists);

        //发送订阅信息
        zmq::message_t message(xml.size());
        memcpy (message.data(), xml.data(), xml.size());
        publisher.send (message);

        QyhSleep(1000);
    }
}

