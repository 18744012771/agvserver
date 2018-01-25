#ifndef AGVCENTER_H
#define AGVCENTER_H

#include <QObject>
#include <QList>
#include <QMutex>
#include <QMap>
#include "bean/agv.h"

class AgvCenter : public QObject
{
    Q_OBJECT
public:
    explicit AgvCenter(QObject *parent = nullptr);
    QList<Agv *> getIdleAgvs();

    bool agvStartTask(int agvId, QList<int> path, int type=0, int leftMidRight=0, int distance=0, int height=0);

    bool agvStopTask(int agvId);

    bool agvCancelTask(int agvId);

    void init();

    QByteArray taskStopCmd(int agvId);

    QByteArray taskControlCmd(int agvId);
    //QByteArray taskControlCmd(int agvId,bool changeDirect);

    bool handControlCmd(int agvId,int agvHandType,int speed);

    bool load();//从数据库载入所有的agv

    bool save();//将agv保存到数据库

    void agvConnectCallBack();

    void agvDisconnectCallBack();
signals:
    void carArriveStation(int agvId,int station);
public slots:


private:

    void updateOdometer(Agv *agv,int odometer);

    void updateStationOdometer(Agv *agv,int station,int odometer);


    QByteArray packet(QByteArray content);
    QByteArray auto_instruct_stop(int rfid,int delay);
    QByteArray auto_instruct_forward(int rfid,int speed);
    QByteArray auto_instruct_backward(int rfid,int speed);
    QByteArray auto_instruct_turnleft(int rfid,int speed);
    QByteArray auto_instruct_turnright(int rfid,int speed);
    QByteArray auto_instruct_mp3_left(int rfid,int mp3Id);
    QByteArray auto_instruct_mp3_right(int rfid, int mp3Id);
    QByteArray auto_instruct_mp3_volume(int rfid, int volume);
    QByteArray auto_instruct_wait();
};

#endif // AGVCENTER_H
