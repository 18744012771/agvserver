#ifndef AGVCENTER_H
#define AGVCENTER_H

#include <QObject>
#include <QList>
#include <QMutex>
#include <QMap>
#include <qyhtcpclient.h>
#include "bean/agv.h"

class AgvCenter : public QObject
{
    Q_OBJECT
public:
    explicit AgvCenter(QObject *parent = nullptr);
    QList<Agv *> getIdleAgvs();

    bool agvStartTask(int agvId,QList<int> path);

    bool agvStopTask(int agvId);

    void init();

    QByteArray taskStopCmd(int agvId);

    QByteArray taskControlCmd(int agvId);
    //QByteArray taskControlCmd(int agvId,bool changeDirect);

    bool handControlCmd(int agvId,int agvHandType,int speed);

    bool load();//从数据库载入所有的agv

    bool save();//将agv保存到数据库

    //void processOneMsg(int id,QByteArray oneMsg);

    //void onAgvRead(const char *data,int len);

    void agvConnectCallBack();

    void agvDisconnectCallBack();

    QMap<int,Agv *> getAgvs()
    {
        QMap<int,Agv *> agvs;
        agvsMtx.lock();
        agvs = g_m_agvs;
        agvsMtx.unlock();
        return agvs;
    }

    void addAgv(Agv *agv)
    {
        agvsMtx.lock();
        g_m_agvs.insert(agv->id,agv);
        agvsMtx.unlock();
    }

    void removeAgv(int id)
    {
        agvsMtx.lock();
        g_m_agvs.remove(id);
        agvsMtx.unlock();

    }

signals:
    void carArriveStation(int agvId,int station);
public slots:


private:

    void updateOdometer(Agv *agv,int odometer);

    void updateStationOdometer(Agv *agv,int station,int odometer);


    QByteArray packet(int id,char code_mode,QByteArray content);
    QByteArray auto_instruct_stop(int rfid,int delay);
    QByteArray auto_instruct_forward(int rfid,int speed);
    QByteArray auto_instruct_backward(int rfid,int speed);
    QByteArray auto_instruct_turnleft(int rfid,int speed);
    QByteArray auto_instruct_turnright(int rfid,int speed);
    QByteArray auto_instruct_mp3_left(int rfid,int mp3Id);
    QByteArray auto_instruct_mp3_right(int rfid, int mp3Id);
    QByteArray auto_instruct_mp3_volume(int rfid, int volume);
    QByteArray auto_instruct_wait();

    //所有的bean集合
    QMap<int,Agv *> g_m_agvs;             //所有车辆们
    QMutex agvsMtx;
};

#endif // AGVCENTER_H
