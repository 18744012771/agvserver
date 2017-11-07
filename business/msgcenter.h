#ifndef MSGCENTER_H
#define MSGCENTER_H

#include <QObject>
#include <QThread>
#include "agvpositionpublisher.h"
#include "agvstatuspublisher.h"

//这里将会启动一个CPU个数*2的线程，用于处理用户的数据
//保证并发量和响应时间

enum AGV_HAND_TYPE{
    AGV_HAND_TYPE_STOP = 0,//停止移动
    AGV_HAND_TYPE_FORWARD = 0x1,//前进
    AGV_HAND_TYPE_BACKWARD = 0x2,//后退
    AGV_HAND_TYPE_TURNLEFT = 0x3,//左转
    AGV_HAND_TYPE_TURNRIGHT = 0x4,//右转
};

class MsgCenter : public QObject
{
    Q_OBJECT
public:
    explicit MsgCenter(QObject *parent = nullptr);
    ~MsgCenter();
    void init();
    bool addAgvPostionSubscribe(int subscribe);
    bool removeAgvPositionSubscribe(int subscribe);
    bool addAgvStatusSubscribe(int subscribe,int agvId);
    bool removeAgvStatusSubscribe(int subscribe,int agvId = 0);
    bool addLogSubscribe(int subscribe,int agvId);
    bool removeLogSubscribe(int subscribe,int agvId = 0);
    QByteArray taskControlCmd(int agvId,bool changeDirect);
    bool handControlCmd(int agvId,int agvHandType,int speed);
signals:

public slots:

private:
    QByteArray auto_instruct_stop(int rfid,int delay);
    QByteArray auto_instruct_forward(int rfid,int speed);
    QByteArray auto_instruct_backward(int rfid,int speed);
    QByteArray auto_instruct_turnleft(int rfid,int speed);
    QByteArray auto_instruct_turnright(int rfid,int speed);
    QByteArray auto_instruct_mp3_left(int rfid,int mp3Id);
    QByteArray auto_instruct_mp3_right(int rfid, int mp3Id);
    QByteArray auto_instruct_mp3_volume(int rfid, int volume);
    QByteArray auto_instruct_wait();

    AgvPositionPublisher *positionPublisher;
    AgvStatusPublisher *statusPublisher;

    QByteArray packet(char code_mode,QByteArray content);
};

#endif // MSGCENTER_H
