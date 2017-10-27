#include "agvcenter.h"
#include "util/global.h"

AgvCenter::AgvCenter(QObject *parent) : QObject(parent)
{

}

//获取空闲的车辆
QList<Agv *> AgvCenter::getIdleAgvs()
{
    QList<Agv *> result;
    for(QMap<int,Agv *>::iterator itr =g_m_agvs.begin();itr!=g_m_agvs.end();++itr)
    {
        if(itr.value()->status() == AGV_STATUS_IDLE){
            result.push_back(itr.value());
        }
    }
    return result;
}

bool AgvCenter::agvStartTask(int agvId, QList<int> path)
{
    //TODO:这里需要启动小车，告诉小车下一站和下几站，还有就是左中右信息
    g_m_agvs[agvId]->setCurrentPath(path);
    QByteArray qba =  g_msgCenter.taskControlCmd(agvId,false);
    //组包完成，发送
    return g_m_agvs[agvId]->sendToAgv(qba);
}
