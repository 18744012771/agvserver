#ifndef AGV_H
#define AGV_H

#include <QObject>
#include <QMap>

enum{
    AGV_STATUS_IDLE=0,//空闲可用
    AGV_STATUS_UNCONNECT=1,//未连接
    AGV_STATUS_TASKING=2,//正在执行任务
    AGV_STATUS_POWER_LOW=3,//电量低
    AGV_STATUS_ERROR=4,//故障
    AGV_STATUS_GO_CHARGING = 5,//返回充电中
    AGV_STATUS_CHARGING=6,//正在充电
};

enum{
    AGV_MODE_AUTO = 0,//自动模式
    AGV_MODE_HAND = 1//手动模式
};



class Agv
{
public:
    Agv():
        id(0),
        name(""),
        x(0),
        y(0),
        rotation(0),
        mileage(0),
        rad(0),
        currentRfid(0),
        speed(0),
        turnSpeed(0),
        cpu(0),
        status(AGV_STATUS_IDLE),
        leftMotorStatus(0),
        rightMotorStatus(0),
        systemVoltage(0),
        systemCurrent(0),
        mode(0),
        positionMagneticStripe(0),
        frontObstruct(0),
        backObstruct(0),
        currentOrder(0),
        currentQueueNumber(0),
        defaultStation(0),
        task(0),
        lastStation(0),
        nowStation(0),
        nextStation(0),
        lastStationOdometer(0),
        nowOdometer(0),
        queueNumber(0),
        m_m_angle(0),
        currentHandUser(0),
        currentHandUserRole(0)
    {

    }

    int getPathRfidAmount(){return currentPath.length();}

public:
    //ID
    int id;

    //编号
    QString name;

    //POSITION
    int x;
    int y;
    int rotation;

    //UPLOAD INFO
    int mileage;
    int rad;
    int currentRfid;
    int speed;
    int turnSpeed;
    int cpu;
    int status;
    int leftMotorStatus;
    int rightMotorStatus;
    int systemVoltage;
    int systemCurrent;

    int mode;
    int positionMagneticStripe;
    bool frontObstruct;
    bool backObstruct;
    int currentOrder;
    int currentQueueNumber;
    int defaultStation;

    //计算路径用的
    int task;
    int lastStation;
    int nowStation;
    int nextStation;

    //用于发送的消息

    QMap<int,QList<int> > commandQueue;//命令队列
    QList<int> currentPath;

    //用于计算当前位置信息
    int lastStationOdometer;
    int nowOdometer;

    //用于发送用的
    int8_t queueNumber;
    double m_m_angle;//这个值仅供参考，后续再考虑

    //用于手动操作用的
    int currentHandUser;//当前手动控制这辆车的用户(默认是0，没有人手动控制)
    int currentHandUserRole;//当前手动控制这辆车的用户 权限
};

#endif // AGV_H
