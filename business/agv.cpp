#include "agv.h"
#include "util/global.h"

Agv::Agv(QObject *parent) : QObject(parent),
    m_id(0),
    m_x(0),
    m_y(0),
    m_lastStation(0),
    m_nowStation(0),
    m_nextStation(0),
    m_speed(5),
    m_status(0),
    m_battery(0),
    m_task(0),
    m_mode(0),
    m_lastStationOdometer(0),
    m_nowOdometer(0),
    m_leftMotorEncoder(0),
    m_rightMotorEncoder(0),
    m_leftMotorSpeed(0),
    m_rightMotorSpeed(0),
    m_leftMotorVoltage(0),
    m_rightMotorVoltage(0),
    m_leftMotorCurrent(0),
    m_rightMotorCurrent(0),
    m_motorStatus(0),
    m_systemVoltage(0),
    m_systemCurrent(0),
    m_currentOrder(0),
    m_currentQueueNumber(0),
    m_defaultStation(0),
    m_isConnected(false),
    m_frontObstruct(false),
    m_backObstruct(false),
    m_rotation(0.0),
    m_name(""),
    queueNumber(0)
{

}

bool Agv::startTask()
{
    //给小车发送指令
    if(!m_sock.isValid()||!m_sock.isOpen()||!m_sock.isWritable()){
        return false;
    }
    //组装一个发送消息

}

int Agv::getPathRfidAmount()
{
    int result=0;
    for(int i=0;i<currentPath().length();++i){
        AgvLine *line = g_m_lines[currentPath().at(i)];
        if(g_m_stations[line->endStation()]->type() != AGV_STATION_TYPE_FATE){
            ++result;
        }
    }
    return result;

}

bool Agv::sendToAgv(QByteArray qba)
{
    if(m_sock.state() == QAbstractSocket::ConnectedState){
        int sendLen = m_sock.write(qba);
        if(sendLen != qba.length())return false;
    }else{
        return false;
    }

    //等待收到确认消息？？

    return true;
}

