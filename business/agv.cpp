#include "agv.h"
#include "util/global.h"
#define _USE_MATH_DEFINES
#include <math.h>

//固定端口？
#define AGV_DEFAULT_PORT  8081

Agv::Agv(QObject *parent) : QObject(parent),
    m_id(0),
    m_x(0),
    m_y(0),
    m_lastStation(0),
    m_nowStation(0),
    m_nextStation(0),
    m_speed(5),
    m_status(AGV_STATUS_UNCONNECT),
    //m_battery(0),
    m_task(0),
    m_mode(0),
    m_lastStationOdometer(0),
    m_nowOdometer(0),
//    m_leftMotorEncoder(0),
//    m_rightMotorEncoder(0),
//    m_leftMotorSpeed(0),
//    m_rightMotorSpeed(0),
//    m_leftMotorVoltage(0),
//    m_rightMotorVoltage(0),
//    m_leftMotorCurrent(0),
//    m_rightMotorCurrent(0),
//    m_motorStatus(0),
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
    queueNumber(0),
    m_ip(""),
    currentHandUser(0),
    currentHandUserRole(0)
{
    connectTimer.setInterval(5000);
    connect(&m_sock,&QAbstractSocket::stateChanged,this,&Agv::connectStatusChanged);
    connect(&connectTimer,&QTimer::timeout,this,&Agv::connectToHost);
    connect(&m_sock,&QAbstractSocket::readyRead,this,&Agv::connectRead);
    connectTimer.start();
}

void Agv::init()
{

}

bool Agv::startTask()
{
    //给小车发送指令
    if(!m_sock.isValid()||!m_sock.isOpen()||!m_sock.isWritable()){
        return false;
    }
    //组装一个发送消息
    return sendToAgv(g_msgCenter.taskControlCmd(m_id,false));
}

int Agv::getPathRfidAmount()
{
    return currentPath().length();
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

void Agv::connectStatusChanged(QAbstractSocket::SocketState s)
{
    if(s != QAbstractSocket::ConnectedState && s!= QAbstractSocket::ConnectingState && s!= QAbstractSocket::HostLookupState){
        //TODO:
        connectTimer.start();
    }else{
        connectTimer.stop();
    }
}

void Agv::connectToHost()
{
    if(m_ip.length()>0)
        m_sock.connectToHost(m_ip,AGV_DEFAULT_PORT);
}
void Agv::processOneMsg(QByteArray oneMsg)
{
    char *data = oneMsg.data();
    int functionCode = data[1];

    if(functionCode == 0x44){
        //里程
        int l = (data[2]<<24)|(data[3]<<16)|(data[4]<<8)|data[5];
        //角度
        m_m_angle = (data[6]<<24)|(data[7]<<16)|(data[8]<<8)|data[9];//这个值恐怕只供参考了//TODO:
        //rfid号
        int station = 0;
        int rfid =  (data[10]<<24)|(data[11]<<16)|(data[12]<<8)|data[13];
        for(QMap<int,AgvStation *>::iterator itr = g_m_stations.begin();itr!=g_m_stations.end();++itr){
            if(itr.value()->rfid() == rfid){
                station = itr.value()->id();
                break;
            }
        }
        if(station != 0){
            if(station == m_lastStation){//上一站未变，只是更新了一下里程计
                updateOdometer(l);
            }else{
                updateStationOdometer(station,l);
            }
        }

        //速度
        setSpeed(data[14]);
        //转向速度
        setTurnSpeed(data[15]);
        //cpu
        setCpu(data[16]);
        //状态机 status//位置书
        setStatus(data[17]);
        //左右电机状态
        setLeftMotorStatus(data[18]);
        setRightMotorStatus(data[18]);

        //系统电压
        setSystemVoltage((data[19] << 8) |data[20]);
        //系统电流
        setSystemCurrent((data[21] << 8) | data[22]);

        //磁条位置
        setPositionMagneticStripe((data[23]<<8)|data[24]);
        //受障情况
        setFrontObstruct(data[25]);
        setBackObstruct(data[26]);

        //TODO:当前命令: 0中控控制，1手柄控制 2自动充电
        m_currentOrder = data[27];

        //TODO:当前队列编号
        m_currentQueueNumber = data[28];

        //设备地址//TODO
        //判断
        //QString ip = QString("%1.%2.%3.%4").arg(data[29]).arg(data[30]).arg(data[31]).arg(data[32]);
        std::stringstream ss;
        ss<<"from agv ip:"<<(int)(data[29])<<(int)(data[30])<<(int)(data[31])<<(int)(data[32]);
        g_log->log(AGV_LOG_LEVEL_INFO,ss.str());

        //附件状态 U16
    }

}

void Agv::connectRead()
{
    static QByteArray buffer;
    buffer += m_sock.readAll();
    if(buffer.length() >= 32){
        //寻找起点，、、寻找终点
        int start,end;
        while(true){
            start = buffer.indexOf(0x55);
            if(start<0)break;
            end = buffer.indexOf(0xAA,start);
            if(end<0)break;

            //截取这条消息，
            QByteArray oneMsg = buffer.mid(start,end-start);
            buffer = buffer.right(buffer.length()-end-1);

            //处理这一条消息
            processOneMsg(oneMsg);
        }
    }
}

//1.只有里程计
void Agv::updateOdometer(int odometer)
{
    if(m_nowStation > 0 && m_nextStation > 0)
    {
        //如果之前在一个站点，现在相当于离开了那个站点
        m_lastStation = m_nowStation;
        m_nowStation = 0;
    }

    if(m_lastStation <= 0) return ;//上一站未知，那么未知直接就是未知的
    if(m_lastStationOdometer <= 0) return ;
    if(m_nextStation <= 0) return ;//下一站未知，那么我不知道方向。

    //如果两个都知道了，那么我就可以计算当前位置了
    odometer -= m_lastStationOdometer;

    //例程是否超过了到下一个站点的距离
    if(m_currentPath.length()<=0)
        return ;
    AgvLine *line =g_m_lines[m_currentPath.at(0)];
    if(odometer< line->length()*line->rate()){
        //计算位置
        if(line->line()){
            double theta = atan2(g_m_stations[m_nextStation]->y()-g_m_stations[m_lastStation]->y(),g_m_stations[m_nextStation]->x()-g_m_stations[m_lastStation]->x());
            setRotation(theta*180/M_PI);

            std::stringstream ss;
            ss<< "m_m_angle-rotation="<<m_m_angle*360/628-rotation();
            g_log->log(AGV_LOG_LEVEL_DEBUG,ss.str());

            setX(g_m_stations[m_lastStation]->x()+odometer*cos(theta));
            setY(g_m_stations[m_lastStation]->y()+odometer*sin(theta));
        }else{
            //弧线夹角 = 弧长/半径
            double currentPositionTheta;
            double theta = odometer/(line->radius()*line->rate());
            double thetaStart = atan2(line->startY()-line->centerY(),line->startX()-line->centerX());
            if(line->clockwise()){
                currentPositionTheta=thetaStart + theta;
                setRotation(currentPositionTheta*180/M_PI+90);
            }else{
                currentPositionTheta=thetaStart-theta;
                setRotation(currentPositionTheta*180/M_PI-90);
            }
            setX(line->centerX()+line->radius()*cos(currentPositionTheta));
            setX(line->centerY()+line->radius()*sin(currentPositionTheta));
        }
    }
}

//2.有站点信息和里程计信息
void Agv::updateStationOdometer(int station,int odometer)
{
    if(!g_m_stations.contains(station))return ;
    //更新当前位置

    //到达了这么个站点
    setX(g_m_stations[station]->x());
    setY(g_m_stations[station]->y());

    //设置当前站点
    m_nowStation=station;
    m_lastStationOdometer=odometer;

    //获取path中的下一站
    int nextStationTemp = 0;
    for(int i=0;i<m_currentPath.length();++i){
        if(g_m_lines[m_currentPath.at(i)]->endStation() == station ){
            if(i+1!=m_currentPath.length())
                nextStationTemp = g_m_lines[m_currentPath.at(i+1)]->endStation();
            else
                nextStationTemp = 0;
            break;
        }
    }
    m_nextStation = nextStationTemp;

    //到站消息上报(更新任务信息、更新线路占用问题)
    emit carArrivleStation(m_id,station);
}
