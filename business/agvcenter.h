#ifndef AGVCENTER_H
#define AGVCENTER_H

#include <QObject>
#include <QList>
#include <QMutex>
#include <QMap>
#include "bean/agv.h"
class Task;

class AgvCenter : public QObject
{
    Q_OBJECT
public:
    explicit AgvCenter(QObject *parent = nullptr);
    QList<Agv *> getIdleAgvs();

    bool agvStartTask(Agv *agv, Task *task);

    bool agvStopTask(int agvId);

    bool agvCancelTask(int agvId);

    void init();

    bool load();//从数据库载入所有的agv

    bool save();//将agv保存到数据库

    void agvConnectCallBack();

    void agvDisconnectCallBack();
signals:
    void carArriveStation(int agvId,int station);

    void pickFinish(int agvId);
    void putFinish(int agvId);
    void standByFinish(int agvId);
public slots:

    void updateOdometer(int odometer);

    void updateStationOdometer(int rfid, int odometer);

    void onPickFinish();

    void onPutFinish();

    void onStandByFinish();
private:
};

#endif // AGVCENTER_H
