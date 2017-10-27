#ifndef AGV_H
#define AGV_H

#include <QObject>
#include <QTcpSocket>
#include <QThread>


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
    int speed(){return m_speed;}
    int status(){return m_status;}
    int battery(){return m_battery;}
    bool isConnected(){return m_isConnected;}
    int task(){return m_task;}
    int mode(){return m_mode;}
    QMap<int,QList<int> > commandQueue(){return m_commandQueue;}
    QList<int> currentPath(){return m_currentPath;}
    int lastStationOdometer(){return m_lastStationOdometer;}
    int nowOdometer(){return m_nowOdometer;}
    int leftMotorEncoder(){return m_leftMotorEncoder;}
    int rightMotorEncoder(){return m_rightMotorEncoder;}
    int leftMotorSpeed(){return m_leftMotorSpeed;}
    int rightMotorSpeed(){return m_rightMotorSpeed;}
    int leftMotorVoltage(){return m_leftMotorVoltage;}
    int rightMotorVoltage(){return m_rightMotorVoltage;}
    int leftMotorCurrent(){return m_leftMotorCurrent;}
    int rightMotorCurrent(){return m_rightMotorCurrent;}
    int motorStatus(){return m_motorStatus;}
    int systemVoltage(){return m_systemVoltage;}
    int systemCurrent(){return m_systemCurrent;}
    bool frontObstruct(){return m_frontObstruct;}
    bool backObstruct(){return m_backObstruct;}
    int currentOrder(){return m_currentOrder;}
    int currentQueueNumber(){return m_currentQueueNumber;}
    int defaultStation(){return m_defaultStation;}
    QString ip(){return m_ip;}

    //setter

    void setId(int newId){m_id=newId;emit idChanged(newId);}
    void setX(int newX){m_x=newX;emit xChanged(newX);}
    void setY(int newY){m_y=newY;emit yChanged(newY);}
    void setRotation(double newRotation){m_rotation=newRotation;emit rotationChanged(newRotation);}
    void setLastStation(int newLastStation){m_lastStation=newLastStation;emit lastStationChanged(newLastStation);}
    void setNowStation(int newNowStation){m_nowStation=newNowStation;emit nowStationChanged(newNowStation);}
    void setNextStation(int newNextStation){m_nextStation=newNextStation;emit nextStationChanged(newNextStation);}
    void setName(QString newName){m_name=newName;emit nameChanged(newName);}
    void setSpeed(int newSpeed){m_speed=newSpeed;emit speedChanged(newSpeed);}
    void setStatus(int newStatus){m_status=newStatus;emit statusChanged(newStatus);}
    void setBattery(int newBattery){m_battery=newBattery;emit batteryChanged(newBattery);}
    void setIsConnected(bool newIsConnected){m_isConnected=newIsConnected;emit isConnectedChanged(newIsConnected);}
    void setTask(int newTask){m_task=newTask;emit taskChanged(newTask);}
    void setMode(int newMode){m_mode=newMode;emit modeChanged(newMode);}
    void setCommandQueue(QMap<int,QList<int> > newCommandQueue){m_commandQueue=newCommandQueue;emit commandQueueChanged(newCommandQueue);}
    void setCurrentPath(QList<int> newCurrentPath){m_currentPath=newCurrentPath;emit currentPathChanged(newCurrentPath);}
    void setLastStationOdometer(int newLastStationOdometer){m_lastStationOdometer=newLastStationOdometer;emit lastStationOdometerChanged(newLastStationOdometer);}
    void setNowOdometer(int newNowOdometer){m_nowOdometer=newNowOdometer;emit nowOdometerChanged(newNowOdometer);}
    void setLeftMotorEncoder(int newLeftMotorEncoder){m_leftMotorEncoder=newLeftMotorEncoder;emit leftMotorEncoderChanged(newLeftMotorEncoder);}
    void setRightMotorEncoder(int newRightMotorEncoder){m_rightMotorEncoder=newRightMotorEncoder;emit rightMotorEncoderChanged(newRightMotorEncoder);}
    void setLeftMotorSpeed(int newLeftMotorSpeed){m_leftMotorSpeed=newLeftMotorSpeed;emit leftMotorSpeedChanged(newLeftMotorSpeed);}
    void setRightMotorSpeed(int newRightMotorSpeed){m_rightMotorSpeed=newRightMotorSpeed;emit rightMotorSpeedChanged(newRightMotorSpeed);}
    void setLeftMotorVoltage(int newLeftMotorVoltage){m_leftMotorVoltage=newLeftMotorVoltage;emit leftMotorVoltageChanged(newLeftMotorVoltage);}
    void setRightMotorVoltage(int newRightMotorVoltage){m_rightMotorVoltage=newRightMotorVoltage;emit rightMotorVoltageChanged(newRightMotorVoltage);}
    void setLeftMotorCurrent(int newLeftMotorCurrent){m_leftMotorCurrent=newLeftMotorCurrent;emit leftMotorCurrentChanged(newLeftMotorCurrent);}
    void setRightMotorCurrent(int newRightMotorCurrent){m_rightMotorCurrent=newRightMotorCurrent;emit rightMotorCurrentChanged(newRightMotorCurrent);}
    void setMotorStatus(int newMotorStatus){m_motorStatus=newMotorStatus;emit motorStatusChanged(newMotorStatus);}
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
    void statusChanged(int newStatus);
    void batteryChanged(int newBattery);
    void isConnectedChanged(bool newIsConnected);
    void taskChanged(int newTask);
    void modeChanged(int newMode);
    void commandQueueChanged(QMap<int,QList<int> > newCommandQueue);
    void currentPathChanged(QList<int> newCurrentPath);
    void lastStationOdometerChanged(int newLastStationOdometer);
    void nowOdometerChanged(int newNowOdometer);
    void leftMotorEncoderChanged(int newLeftMotorEncoder);
    void rightMotorEncoderChanged(int newRightMotorEncoder);
    void leftMotorSpeedChanged(int newLeftMotorSpeed);
    void rightMotorSpeedChanged(int newRightMotorSpeed);
    void leftMotorVoltageChanged(int newLeftMotorVoltage);
    void rightMotorVoltageChanged(int newRightMotorVoltage);
    void leftMotorCurrentChanged(int newLeftMotorCurrent);
    void rightMotorCurrentChanged(int newRightMotorCurrent);
    void motorStatusChanged(int newMotorStatus);
    void systemVoltageChanged(int newSystemVoltage);
    void systemCurrentChanged(int newSystemCurrent);
    void frontObstructChanged(bool newFrontObstruct);
    void backObstructChanged(bool newBackObstruct);
    void currentOrderChanged(int newCurrentOrder);
    void currentQueueNumberChanged(int newCurrentQueueNumber);
    void defaultStationChanged(int newDefaultStation);
    void ipChanged(QString newIp);
public slots:
private:
    //member
    int m_id;
    int m_x;
    int m_y;
    double m_rotation;
    int m_lastStation;
    int m_nowStation;
    int m_nextStation;
    QString m_name;
    int m_speed;
    int m_status;
    int m_battery;
    bool m_isConnected;
    int m_task;
    int m_mode;
    QMap<int,QList<int> > m_commandQueue;//命令队列
    QList<int> m_currentPath;
    int m_lastStationOdometer;
    int m_nowOdometer;
    int m_leftMotorEncoder;
    int m_rightMotorEncoder;
    int m_leftMotorSpeed;
    int m_rightMotorSpeed;
    int m_leftMotorVoltage;
    int m_rightMotorVoltage;
    int m_leftMotorCurrent;
    int m_rightMotorCurrent;
    int m_motorStatus;
    int m_systemVoltage;
    int m_systemCurrent;
    bool m_frontObstruct;
    bool m_backObstruct;
    int m_currentOrder;
    int m_currentQueueNumber;
    int m_defaultStation;
    QString m_ip;
    QTcpSocket m_sock;

public:
    int queueNumber;
};

#endif // AGV_H
