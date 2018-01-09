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
    publisher.bind("tcp://*:5566");

    while(!isQuit){
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
                mm.insert(QString("status"),QString("%1").arg(t->status));
                mm.insert(QString("excutecar"),QString("%1").arg(t->excuteCar));
                mm.insert(QString("id"),QString("%1").arg(t->id));
                responseDatalists.push_back(mm);
            }
        }
        for(int i=0;i<doings.length();++i)
        {
            QMap<QString,QString> mm;
            AgvTask *t = doings.at(i);
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

//void AgvTaskPublisher::updateAgvTaskInfo()
//{
//    QList<AgvTaskBriefInfo> taskstatus = msgCenter.getAgvTaskListModel();

//    //重新显示所有数据
//    agvWarnWidget->clearContents();
//    while(agvWarnWidget->rowCount()>0){
//        agvWarnWidget->removeRow(0);
//    }

//    for(int i=0;i<taskstatus.length();++i){
//        agvWarnWidget->insertRow(i);
//        AgvTaskBriefInfo u = taskstatus.at(i);
//        QTableWidgetItem *itemid = new QTableWidgetItem(tr("%1").arg(u.id));
//        itemid->setTextAlignment(Qt::AlignCenter);
//        agvWarnWidget->setItem(i, 0, itemid);

//        QString s = QString("%1").arg(u.excutecar);
//        if(u.excutecar<=0)s="";
//        QTableWidgetItem *itemmileage = new QTableWidgetItem(s);
//        itemmileage->setTextAlignment(Qt::AlignCenter);
//        agvWarnWidget->setItem(i, 1, itemmileage);

//        QString statusStr = "";
//        if(u.status == AGV_TASK_STATUS_UNEXIST)statusStr = QStringLiteral("不存在");
//        if(u.status == AGV_TASK_STATUS_UNEXCUTE){
//            if(u.excutecar<=0)
//            {
//                statusStr = QStringLiteral("未分配");
//            }else{
//                statusStr = QStringLiteral("已分配未执行");
//            }
//        }
//        if(u.status == AGV_TASK_STATUS_EXCUTING)statusStr = QStringLiteral("正在执行");
//        if(u.status == AGV_TASK_STATUS_DONE)statusStr = QStringLiteral("已完成");
//        if(u.status == AGV_TASK_STATUS_FAIL)statusStr = QStringLiteral("失败");
//        if(u.status == AGV_TASK_STATSU_CANCEL)statusStr = QStringLiteral("取消");

//        QTableWidgetItem *itemrad = new QTableWidgetItem(statusStr);
//        itemrad->setTextAlignment(Qt::AlignCenter);
//        agvWarnWidget->setItem(i, 2, itemrad);

//    }
//    agvWarnWidget->update();

//}
