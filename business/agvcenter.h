#ifndef AGVCENTER_H
#define AGVCENTER_H

#include <QObject>
#include <QList>
#include <QMutex>
#include <QMap>
#include "bean/agvagent.h"
class Task;

class AgvCenter : public QObject
{
    Q_OBJECT
public:
    explicit AgvCenter(QObject *parent = nullptr);
    QList<AgvAgent *> getIdleAgvs();

    bool agvStartTask(AgvAgent *agv, Task *task);

    bool agvStopTask(int agvId);

    bool agvCancelTask(int agvId);

    void init();

    bool load();//从数据库载入所有的agv

    bool save();//将agv保存到数据库

    void agvConnectCallBack();

    void agvDisconnectCallBack();

    void doExcute(QList<AgvOrder> orders);


    void updateOdometer(int odometer,AgvAgent *agv);

    void updateStationOdometer(int rfid, int odometer,AgvAgent *agv);

    void onFinish(AgvAgent *agv);

    void onError(int code, AgvAgent *agv);

    void onInterupt(AgvAgent *agv);

signals:
    void carArriveStation(int agvId,int station);

    void pickFinish(int agvId);
    void putFinish(int agvId);
    void standByFinish(int agvId);
public slots:


private:
};

#endif // AGVCENTER_H
