#include "agvcenter.h"
#include "util/global.h"
#include "util/common.h"

#define _USE_MATH_DEFINES
#include <math.h>


AgvCenter::AgvCenter(QObject *parent) : QObject(parent)
{

}

//获取空闲的车辆
QList<Agv *> AgvCenter::getIdleAgvs()
{
    QList<Agv *> result;

    for(QMap<int,Agv *>::iterator itr =g_m_agvs.begin();itr!=g_m_agvs.end();++itr)
    {
        if(itr.value()->status == Agv::AGV_STATUS_IDLE){
            result.push_back(itr.value());
        }
    }
    return result;
}


//1.只有里程计
void AgvCenter::updateOdometer(int odometer)
{
    Agv *agv = static_cast<Agv *>(sender());

    if(odometer == agv->lastStationOdometer)//在原来的位置没动
        return ;

    //正在移动中，不再原来的站点的位置了
    if(agv->nowStation > 0 && agv->nextStation > 0)
    {
        //如果之前在一个站点，现在相当于离开了那个站点
        agv->lastStation = agv->nowStation;
        agv->nowStation = 0;
    }

    if(agv->lastStation <= 0) return ;//上一站未知，那么未知直接就是未知的
    if(agv->nextStation <= 0) return ;//下一站未知，那么我不知道方向。

    //如果两个都知道了，那么我就可以计算当前位置了
    odometer -= agv->lastStationOdometer;

    //例程是否超过了到下一个站点的距离
    if(agv->currentPath.length()<=0)
        return ;


    //这里需要合并所有的锁，因为要计算
    //需要如下几个因素 agvline + startstaion+ endstation
    AgvLine line = g_agvMapCenter->getAgvLine(agv->currentPath.at(0));
    if(line.id<=0)return ;
    AgvStation startStation = g_agvMapCenter->getAgvStation(line.startStation);
    if(startStation.id<=0)return ;
    AgvStation endStation = g_agvMapCenter->getAgvStation(line.endStation);
    if(endStation.id<=0)return ;

    if(odometer <= line.length*line.rate)
    {
        //计算位置
        if(line.line){
            double theta = atan2(endStation.y-startStation.y,endStation.x-startStation.x);
            agv->rotation = (theta*180/M_PI);
            agv->x = (startStation.x+1.0*odometer/line.rate*cos(theta));
            agv->y = (startStation.y+1.0*odometer/line.rate*sin(theta));
        }else{

            //在新的绘图下，计算当前坐标，以及rotation
            double t = 1.0*odometer/line.rate/line.length;
            if(t<0){
                t = 0.0;
            }
            if(t>1){
                t=1.0;
            }
            //计算坐标
            double startX = startStation.x;
            double startY = startStation.y;
            double endX = endStation.x;
            double endY = endStation.y;
            agv->x = (startX*(1-t)*(1-t)*(1-t)
                      +3*line.p1x*t*(1-t)*(1-t)
                      +3*line.p2x*t*t*(1-t)
                      +endX*t*t*t);

            agv->y = (startY*(1-t)*(1-t)*(1-t)
                      +3*line.p1y*t*(1-t)*(1-t)
                      +3*line.p2y*t*t*(1-t)
                      +endY*t*t*t);

            double X = startX * 3 * (1 - t)*(1 - t) * (-1) +
                    3 * line.p1x * ((1 - t) * (1 - t) + t * 2 * (1 - t) * (-1)) +
                    3 * line.p2x * (2 * t * (1 - t) + t * t * (-1)) +
                    endX * 3 * t *t;

            double Y =  startY * 3 * (1 - t)*(1 - t) * (-1) +
                    3 *line.p1y * ((1 - t) *(1 - t) + t * 2 * (1 - t) * (-1)) +
                    3 * line.p2y * (2 * t * (1 - t) + t * t * (-1)) +
                    endY * 3 * t *t;

            agv->rotation = (atan2(Y, X) * 180 / M_PI);
        }
    }
}

//2.有站点信息和里程计信息
void AgvCenter::updateStationOdometer(int rfid, int odometer)
{
    Agv *agv = static_cast<Agv *>(sender());
    AgvStation sstation = g_agvMapCenter->getAgvStationByRfid(rfid);
    if(sstation.id<=0)return ;

    //到达了这么个站点
    agv->x = (sstation.x);
    agv->y = (sstation.y);

    //设置当前站点
    agv->nowStation=sstation.id;
    agv->lastStationOdometer=odometer;

    //获取path中的下一站
    int nextStationTemp = 0;
    for(int i=0;i<agv->currentPath.length();++i)
    {
        AgvLine line = g_agvMapCenter->getAgvLine(agv->currentPath.at(i));
        if(line.id<=0)continue;
        if(line.endStation == sstation.id)
        {
            if(i+1!=agv->currentPath.length())
            {
                AgvLine lineNext = g_agvMapCenter->getAgvLine(agv->currentPath.at(i+1));
                nextStationTemp = lineNext.endStation;
            }
            else
                nextStationTemp = 0;
            break;
        }
    }
    agv->nextStation = nextStationTemp;

    //到站消息上报(更新任务信息、更新线路占用问题)
    emit carArriveStation(agv->id,sstation.id);
}

void  AgvCenter::onPickFinish()
{
    Agv *agv = static_cast<Agv *>( sender());
    emit pickFinish(agv->id);
}

void  AgvCenter::onPutFinish()
{
    Agv *agv = static_cast<Agv *>( sender());
    emit putFinish(agv->id);
}

void  AgvCenter::onStandByFinish()
{
    Agv *agv = static_cast<Agv *>( sender());
    emit standByFinish(agv->id);
}

bool AgvCenter::load()//从数据库载入所有的agv
{
    QString querySql = "select id,agv_name,agv_ip,agv_port from agv_agv";
    QList<QVariant> params;
    QList<QList<QVariant> > result = g_sql->query(querySql,params);
    for(int i=0;i<result.length();++i){
        QList<QVariant> qsl = result.at(i);
        if(qsl.length() == 4){
            Agv *agv = new Agv;
            agv->id=(qsl.at(0).toInt());
            agv->name=(qsl.at(1).toString());
            agv->init(qsl.at(2).toString(),qsl.at(3).toInt());
            connect(agv,SIGNAL(updateOdometer(int)),this,SLOT(updateOdometer(int)));
            connect(agv,SIGNAL(updateRfidAndOdometer(int,int)),this,SLOT(updateStationOdometer(int,int)));
            connect(agv,SIGNAL(pickFinish()),this,SLOT(onPickFinish()));
            connect(agv,SIGNAL(putFinish()),this,SLOT(onPutFinish()));
            connect(agv,SIGNAL(standByFinish()),this,SLOT(onStandByFinish()));
            g_m_agvs.insert(agv->id,agv);
        }
    }
    return true;
}

bool AgvCenter::save()//将agv保存到数据库
{
    //查询所有的，
    QList<int> selectAgvIds;
    QList<QVariant> params;
    QString querySql = "select id from agv_agv";
    QList<QList<QVariant>> queryResult = g_sql->query(querySql,params);
    for(int i=0;i<queryResult.length();++i){
        QList<QVariant> qsl = queryResult.at(i);
        if(qsl.length()==1){
            int id = qsl.at(0).toInt();
            selectAgvIds.push_back(id);
        }
    }

    for(int i=0;i<selectAgvIds.length();++i)
    {
        if(g_m_agvs.contains(selectAgvIds.at(i))){
            //含有,进行更新
            QString updateSql = "update agv_agv set agv_name=?,agv_ip=?,agv_port=? where id=?";
            params.clear();
            params<<g_m_agvs[selectAgvIds.at(i)]->name<<g_m_agvs[selectAgvIds.at(i)]->ip<<g_m_agvs[selectAgvIds.at(i)]->port<<QString("%1").arg(g_m_agvs[selectAgvIds.at(i)]->id);

            if(!g_sql->exeSql(updateSql,params))
            {
                return false;
            }
        }else{
            //不含有，就删除
            QString deleteSql = "delete from agv_agv where id=?";
            params.clear();
            params<<selectAgvIds.at(i);
            if(g_sql->exeSql(deleteSql,params)){
                selectAgvIds.removeAt(i);
                --i;
            }else{
                return false;
            }
        }

    }

    //如果在g_m_agvs中有更多的呢，怎么呢，插入
    for(QMap<int,Agv *>::iterator itr=g_m_agvs.begin();itr!=g_m_agvs.end();++itr){
        if(selectAgvIds.contains(itr.key()))continue;
        //插入操作
        QString insertSql = "insert into agv_agv(id,agv_name,agv_ip,agv_port) values(?,?,?,?)";
        params.clear();
        params<<itr.value()->id<<itr.value()->name<<itr.value()->ip<<itr.value()->port;
        if(!g_sql->exeSql(insertSql,params)){
            return false;
        }
    }
    return true;
}



void AgvCenter::init()
{
    load();
}


/////////////////////////协议封装///////////////////////////////////////////////
//bool AgvCenter::handControlCmd(int agvId,int agvHandType,int speed)
//{
////    if(!g_m_agvs.contains(agvId))
////    {
////        return false;
////    }
////    Agv *agv = g_m_agvs[agvId];

//////    //组装一个手控的命令
//////    QByteArray content;
//////    content.append(0x33);//手控的功能码
//////    short baseSpeed = speed & 0xFFFF;
//////    short forwardSpeed = 0;
//////    short leftSpeed = 0;
//////    switch(agvHandType){
//////    case AGV_HAND_TYPE_STOP:
//////        break;
//////    case AGV_HAND_TYPE_FORWARD:
//////        forwardSpeed = baseSpeed;
//////        break;
//////    case AGV_HAND_TYPE_BACKWARD:
//////        forwardSpeed = -1*baseSpeed;
//////        break;
//////    case AGV_HAND_TYPE_TURNLEFT:
//////        leftSpeed = baseSpeed;
//////        break;
//////    case AGV_HAND_TYPE_TURNRIGHT:
//////        leftSpeed = -1 * baseSpeed;
//////        break;
//////    default:
//////        return false;
//////    }

//////    //前后方向 2Byte
//////    content.append((forwardSpeed>>8) &0xFF);
//////    content.append((forwardSpeed) &0xFF);

//////    //左右方向 2Byte
//////    content.append((leftSpeed>>8) &0xFF);
//////    content.append((leftSpeed) &0xFF);

//////    //附件命令 4Byte
//////    content.append(CHAR_NULL);
//////    content.append(CHAR_NULL);
//////    content.append(CHAR_NULL);
//////    content.append(CHAR_NULL);

//////    //灯带数据 1Byte
//////    content.append(CHAR_NULL);

//////    //控制交接 1Byte
//////    content.append(CHAR_NULL);

//////    //设备地址，指令发起者 2Byte
//////    content.append(CHAR_NULL);
//////    content.append(CHAR_NULL);

//////    //备用字节S32*4 = 16Byte
//////    for(int i=0;i<16;++i){
//////        content.append(CHAR_NULL);
//////    }

//////    QByteArray result = packet(agvId,AGV_PACK_SEND_CODE_HAND_MODE,content);

//////    Agv *agv = g_m_agvs[agvId];
////    bool b = agv->send(result.data(),result.length());
////    return b;
//}

//QByteArray AgvCenter::taskStopCmd(int agvId)
//{
////    //组装一个agv执行path的命令
////    QByteArray content;

////    (g_m_agvs[agvId]->queueNumber) +=1;
////    g_m_agvs[agvId]->queueNumber &=  0xFF;


////    //队列编号 0-255循环使用
////    content[0] = g_m_agvs[agvId]->queueNumber;
////    //首先需要启动
////    //1.立即停止
////    content.append(auto_instruct_stop(AGV_PACK_SEND_RFID_CODE_ETERNITY,0));

////    //固定长度五组
////    while(content.length()+5 < 28){
////        content.append(auto_instruct_wait());///////////////////////////////////////////////////////5*5=25Byte
////    }

////    content.append(CHAR_NULL);
////    content.append(CHAR_NULL);/////////////////////////////////////////////设备地址 2Byte

////    assert(content.length() == 28);
////    //组包//加入包头、功能码、内容、校验和、包尾
////    QByteArray result = packet(agvId,AGV_PACK_SEND_CODE_AUDTO_MODE,content);

////    return result;
//}


//QByteArray AgvCenter::taskControlCmd(int agvId)
//{
//    //组装一个agv执行path的命令
//    QByteArray content;

//    ++(g_m_agvs[agvId]->queueNumber);
//    g_m_agvs[agvId]->queueNumber &=  0xFF;
//    //队列编号 0-255循环使用
//    content[0] = g_m_agvs[agvId]->queueNumber;
//    //首先需要启动
//    //1.立即启动
//    content.append(auto_instruct_forward(AGV_PACK_SEND_RFID_CODE_IMMEDIATELY,g_m_agvs[agvId]->speed));

//    //然后对接下来的要执行的数量进行预判
//    for(int i=0;i<g_m_agvs[agvId]->currentPath.length() && content.length()+5 < 28;++i)
//    {
//        AgvLine line = g_agvMapCenter->getAgvLine(g_m_agvs[agvId]->currentPath.at(i));
//        AgvStation station = g_agvMapCenter->getAgvStation(line.endStation);
//        //加入一个命令
//        content.append(auto_instruct_forward(station.rfid,g_m_agvs[agvId]->speed));
//    }

//    //然后对 到达站后的 执行命令进行处理
//    while(content.length()+5<28)
//    {
//        //TODO
//        //判断后续跟的是什么：

//    }
//    //固定长度五组
//    while(content.length()+5 < 28){
//        content.append(auto_instruct_wait());///////////////////////////////////////////////////////5*5=25Byte
//    }

//    content.append(CHAR_NULL);
//    content.append(CHAR_NULL);/////////////////////////////////////////////设备地址 2Byte

//    assert(content.length() == 28);
//    //组包//加入包头、功能码、内容、校验和、包尾
//    QByteArray result = packet(agvId,AGV_PACK_SEND_CODE_AUDTO_MODE,content);

//    return result;
//}

void AgvCenter::agvConnectCallBack()
{
    qDebug()<<("agv connect OK!\n");
}

void AgvCenter::agvDisconnectCallBack()
{
    qDebug()<<("agv disconnect!\n");
}

bool AgvCenter::agvCancelTask(int agvId)
{
    Agv *agv= g_m_agvs[agvId];
    if(agv->status ==Agv:: AGV_STATUS_TASKING)
        agv->status = Agv::AGV_STATUS_IDLE;
    //先让小车停下来
    agvStopTask(agvId);

    //将小车占用的线路、站点释放
    if(agv->nowStation>0)
    {
        //那么只占用这个站点，线路全部释放
        g_agvMapCenter->freeAgvLines(agvId);
        g_agvMapCenter->freeAgvStation(agvId,agv->nowStation);
    }else{
        //那么小车位于一条线路中间，需要释放所有的站点，但是要占用这条线路的正反两个方向
        int lineId = g_agvMapCenter->getLineId(agv->lastStation,agv->nextStation);
        g_agvMapCenter->freeAgvLines(agvId,lineId);
        g_agvMapCenter->freeAgvStation(agvId);
    }

    if(agv->status == Agv::AGV_STATUS_TASKING)
        agv->status = (Agv::AGV_STATUS_IDLE);

    return true;
}

bool AgvCenter::agvStopTask(int agvId)
{
    if(!g_m_agvs.contains(agvId))
    {
        return false;
    }
    Agv *agv = g_m_agvs[agvId];
    agv->doStop();
    return true;
}

/*
 *leftmidright: 走完路径后，左转、右转还是直行
 *distance:走完路径后(左右转后)，前进的距离
 *height:叉板的高度
 *type: 0原地待命 1取货  -1放货
 * */
bool AgvCenter::agvStartTask(Agv *agv,Task *task)
{
    if(task->currentDoIndex == Task::INDEX_GETTING_GOOD)
    {
        agv->doPick();
    }else if(task->currentDoIndex == Task::INDEX_PUTTING_GOOD)
    {
        agv->doPut();
    }else if(task->currentDoIndex == Task::INDEX_GOING_STANDBY)
    {
        agv->doStandBy();
    }else{
        return false;
    }
    return true;
}

void AgvCenter::goStandBy()
{
    for(QMap<int,Agv *>::iterator itr =g_m_agvs.begin();itr!=g_m_agvs.end();++itr)
    {
        if(itr.value()->status == Agv::AGV_STATUS_IDLE){
            itr.value()->go(0x02000020);
        }
    }
}

