#ifndef AGV_H
#define AGV_H

#include <QObject>
#include <QTcpSocket>
#include <QTimer>

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



class Agv : public QObject
{
    Q_OBJECT
public:
    explicit Agv(QObject *parent = nullptr);
    void init();
    bool startTask();
    int getPathRfidAmount();
    bool sendToAgv(QByteArray qba);

    //getter
    int id(){return m_id;}
    int x(){return m_x;}
    int y(){return m_y;}
    double rotation(){return m_rotation;}
    int lastStation(){return m_lastStation;}
    int nowStation(){return m_nowStation;}
    int nextStation(){return m_nextStation;}
    QString name(){return m_name;}
    int defaultStation(){return m_defaultStation;}
    //int battery(){return m_battery;}
    bool isConnected(){return m_isConnected;}
    int task(){return m_task;}
    int mode(){return m_mode;}
    QMap<int,QList<int> > commandQueue(){return m_commandQueue;}
    QList<int> currentPath(){return m_currentPath;}
    int lastStationOdometer(){return m_lastStationOdometer;}
    int nowOdometer(){return m_nowOdometer;}

    int mileage(){return m_mileage;}
    int rad(){return m_rad;}
    int currentRfid(){return m_currentRfid;}
    int speed(){return m_speed;}
    int turnSpeed(){return m_turnSpeed;}
    int cpu(){return m_cpu;}
    int status(){return m_status;}
    int leftMotorStatus(){return m_leftMotorStatus;}
    int rightMotorStatus(){return m_rightMotorStatus;}
    int systemVoltage(){return m_systemVoltage;}
    int systemCurrent(){return m_systemCurrent;}
    int positionMagneticStripe(){return m_positionMagneticStripe;}
    bool frontObstruct(){return m_frontObstruct;}
    bool backObstruct(){return m_backObstruct;}
    int currentOrder(){return m_currentOrder;}
    int currentQueueNumber(){return m_currentQueueNumber;}
    QString ip(){return m_ip;}

    //setter
    void setMileage(int newMileage){m_mileage=newMileage;emit mileageChanged(newMileage);}
    void setRad(int newRad){m_rad=newRad;emit radChanged(newRad);}
    void setCurrentRfid(int newCurrentRfid){m_currentRfid=newCurrentRfid;emit currentRfidChanged(newCurrentRfid);}
    void setId(int newId){m_id=newId;emit idChanged(newId);}
    void setX(int newX){m_x=newX;emit xChanged(newX);}
    void setY(int newY){m_y=newY;emit yChanged(newY);}
    void setRotation(double newRotation){m_rotation=newRotation;emit rotationChanged(newRotation);}
    void setLastStation(int newLastStation){m_lastStation=newLastStation;emit lastStationChanged(newLastStation);}
    void setNowStation(int newNowStation){m_nowStation=newNowStation;emit nowStationChanged(newNowStation);}
    void setNextStation(int newNextStation){m_nextStation=newNextStation;emit nextStationChanged(newNextStation);}
    void setName(QString newName){m_name=newName;emit nameChanged(newName);}
    void setSpeed(int newSpeed){m_speed=newSpeed;emit speedChanged(newSpeed);}
    void setTurnSpeed(int newTurnSpeed){m_turnSpeed=newTurnSpeed;emit turnSpeedChanged(newTurnSpeed);}
    void setCpu(int newCpu){m_cpu=newCpu;emit cpuChanged(newCpu);}
    void setLeftMotorStatus(int newLeftMotorStatus){m_leftMotorStatus=newLeftMotorStatus;emit leftMotorStatusChanged(newLeftMotorStatus);}
    void setRightMotorStatus(int newRightMotorStatus){m_rightMotorStatus=newRightMotorStatus;emit rightMotorStatusChanged(newRightMotorStatus);}
    void setPositionMagneticStripe(int newpositionMagneticStripe){m_positionMagneticStripe=newpositionMagneticStripe;emit positionMagneticStripeChanged(newpositionMagneticStripe);}
    void setStatus(int newStatus){m_status=newStatus;emit statusChanged(newStatus);}
    //void setBattery(int newBattery){m_battery=newBattery;emit batteryChanged(newBattery);}
    void setIsConnected(bool newIsConnected){m_isConnected=newIsConnected;emit isConnectedChanged(newIsConnected);}
    void setTask(int newTask){m_task=newTask;emit taskChanged(newTask);}
    void setMode(int newMode){m_mode=newMode;emit modeChanged(newMode);}
    void setCommandQueue(QMap<int,QList<int> > newCommandQueue){m_commandQueue=newCommandQueue;emit commandQueueChanged(newCommandQueue);}
    void setCurrentPath(QList<int> newCurrentPath){m_currentPath=newCurrentPath;emit currentPathChanged(newCurrentPath);}
    void setLastStationOdometer(int newLastStationOdometer){m_lastStationOdometer=newLastStationOdometer;emit lastStationOdometerChanged(newLastStationOdometer);}
    void setNowOdometer(int newNowOdometer){m_nowOdometer=newNowOdometer;emit nowOdometerChanged(newNowOdometer);}
    void setSystemVoltage(int newSystemVoltage){m_systemVoltage=newSystemVoltage;emit systemVoltageChanged(newSystemVoltage);}
    void setSystemCurrent(int newSystemCurrent){m_systemCurrent=newSystemCurrent;emit systemCurrentChanged(newSystemCurrent);}
    void setFrontObstruct(bool newFrontObstruct){m_frontObstruct=newFrontObstruct;emit frontObstructChanged(newFrontObstruct);}
    void setBackObstruct(bool newBackObstruct){m_backObstruct=newBackObstruct;emit backObstructChanged(newBackObstruct);}
    void setCurrentOrder(int newCurrentOrder){m_currentOrder=newCurrentOrder;emit currentOrderChanged(newCurrentOrder);}
    void setCurrentQueueNumber(int newCurrentQueueNumber){m_currentQueueNumber=newCurrentQueueNumber;emit currentQueueNumberChanged(newCurrentQueueNumber);}
    void setIp(QString newIp){m_ip=newIp;emit ipChanged(newIp);}
    void setDefaultStation(int newDefaultStation){m_defaultStation=newDefaultStation;emit defaultStationChanged(newDefaultStation);}
signals:
    void idChanged(int newId);
    void xChanged(int newX);
    void yChanged(int newY);
    void rotationChanged(double newRotation);
    void lastStationChanged(int newLastStation);
    void nowStationChanged(int newNowStation);
    void nextStationChanged(int newNextStation);
    void nameChanged(QString newName);
    void speedChanged(int newSpeed);
    void turnSpeedChanged(int newTurnSpeed);
    void cpuChanged(int newCpu);
    void leftMotorStatusChanged(int newLeftMotorStatus);
    void rightMotorStatusChanged(int newRightMotorStatus);
    void positionMagneticStripeChanged(int newpositionMagneticStripe);
    void mileageChanged(int newMileage);
    void radChanged(int newRad);
    void currentRfidChanged(int newCurrentRfid);
    void statusChanged(int newStatus);
    void isConnectedChanged(bool newIsConnected);
    void taskChanged(int newTask);
    void modeChanged(int newMode);
    void commandQueueChanged(QMap<int,QList<int> > newCommandQueue);
    void currentPathChanged(QList<int> newCurrentPath);
    void lastStationOdometerChanged(int newLastStationOdometer);
    void nowOdometerChanged(int newNowOdometer);
    void systemVoltageChanged(int newSystemVoltage);
    void systemCurrentChanged(int newSystemCurrent);
    void frontObstructChanged(bool newFrontObstruct);
    void backObstructChanged(bool newBackObstruct);
    void currentOrderChanged(int newCurrentOrder);
    void currentQueueNumberChanged(int newCurrentQueueNumber);
    void defaultStationChanged(int newDefaultStation);
    void ipChanged(QString newIp);

    void carArrivleStation(int car,int station);
public slots:
    void connectStatusChanged(QAbstractSocket::SocketState s);
    void connectToHost();
    void connectRead();
private:
    //member
    int m_id;
    int m_x;
    int m_y;
    double m_rotation;//根据地图线路，计算出来的
    int m_mileage;//开机里程
    int m_rad;//开机弧度
    int m_currentRfid;//当前卡号
    int m_lastStation;//上一站
    int m_nowStation;//现在所在站点
    int m_nextStation;//要去的下一站
    QString m_name;//名字
    int m_speed;//速度
    int m_turnSpeed;
    int m_cpu;
    int m_status;
    int m_leftMotorStatus;
    int m_rightMotorStatus;
    bool m_isConnected;
    int m_systemVoltage;
    int m_systemCurrent;
    int m_task;
    int m_mode;
    int m_positionMagneticStripe;//磁条位置

    QMap<int,QList<int> > m_commandQueue;//命令队列
    QList<int> m_currentPath;
    int m_lastStationOdometer;
    int m_nowOdometer;

    bool m_frontObstruct;
    bool m_backObstruct;

    int m_currentOrder;
    int m_currentQueueNumber;
    int m_defaultStation;
    QString m_ip;
    QTcpSocket m_sock;

public:
    int queueNumber;
    double m_m_angle;//这个值仅供参考，后续再考虑


    int currentHandUser;//当前手动控制这辆车的用户(默认是0，没有人手动控制)
    int currentHandUserRole;//当前手动控制这辆车的用户 权限
private:
    QTimer connectTimer;

    void processOneMsg(QByteArray oneMsg);

    //1.只有里程计
    void updateOdometer(int odometer);

    //2.有站点信息和里程计信息
    void updateStationOdometer(int station,int odometer);

};

#endif // AGV_H
