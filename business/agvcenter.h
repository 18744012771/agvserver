#ifndef AGVCENTER_H
#define AGVCENTER_H

#include <QObject>
#include <QList>
#include <qyhtcpclient.h>
#include "agv.h"

class AgvCenter : public QObject
{
    Q_OBJECT
public:
    explicit AgvCenter(QObject *parent = nullptr);
    QList<Agv *> getIdleAgvs();

    bool agvStartTask(int agvId,QList<int> path);

    void init();

    QByteArray taskControlCmd(int agvId,bool changeDirect);

    bool handControlCmd(int agvId,int agvHandType,int speed);

    bool load();//从数据库载入所有的agv

    bool save();//将agv保存到数据库

    void processOneMsg(int id,QByteArray oneMsg);

    void onAgvRead(const char *data,int len);

    void agvConnectCallBack();

    void agvDisconnectCallBack();

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

    QyhTcp::QyhTcpClient *tcpClient;
};

#endif // AGVCENTER_H
