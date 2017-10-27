#ifndef TASKCENTER_H
#define TASKCENTER_H

#include <QObject>
#include <QTimer>
#include "agvtask.h"
#include "agv.h"


class TaskCenter : public QObject
{
    Q_OBJECT
public:
    explicit TaskCenter(QObject *parent = nullptr);

    void init();

    //产生一个取货送货的任务,pickupStation是取货点，aimStation是送货点
    int makePickupTask(int pickupStation,int aimStation,int waitTypePick,int waitTimePick,int waitTypeAim,int waitTimeAim);

    //产生一个直接去到目的地的任务，目的地是aimStation
    int makeAimTask(int aimStation,int waitType,int waitTime);

    //产生一个固定某辆车去到目的地的任务，车辆是agvKey，目的地是aimStation
    int makeAgvAimTask(int agvKey,int aimStation,int waitType,int waitTime);

    //产生一个取货送货，并且完成后回到初始位置的 任务
    int makeAgvPickupReturnTask(int pickupStation,int aimStation,int waitTypePick,int waitTimePick,int waitTypeAim,int waitTimeAim);

    int queryTaskStatus(int taskId);//返回task的状态。
    int queryTaskCar(int taskId);//查询这个任务是那辆车执行的
    void cancelTask(int taskId);//取消一个任务
    void carArriveStation(int car,int station);

signals:

public slots:
    void unassignedTasksProcess();//未分配的任务
    void doingTaskProcess();//正在执行的任务(由于线路占用的问题，导致小车停在了某个位置，需要启动它)

//    void onPickUpFinish(int taskKey);
//    void onAimFinish(int taskKey);
private:
    //这里可以对任务进行扩展。将任务要做的事情做成一个不定的
    //对于C类任务(直接去往目的地).它会先被放入todoAimtask中，等待分配车辆执行。如果分配到车辆了，这个任务会放入doingtasks中
    //对于AB类任务(去A地装货，然后送到B地)，它会先被放入todoPickTasks中，等待分配车辆，如果分配到的车辆了，这个任务会放入doingtasks中，如果完成了装货，它会被放入todoAimTasks中，等待有可行线路去往目的地
    QList<AgvTask *> unassignedTasks;           //未分配的任务
    QList<AgvTask *> doingTasks;                //正在执行的任务
//    QList<AgvTask *> todoAimTasks;//直接到目的地的任务
//    QList<AgvTask *> todoPickTasks;//经过pickup的任务
//    QList<AgvTask *> doingTasks;//正在执行的任务

    void clear();
    int maxId;

    QTimer taskProcessTimer;

    int doneTasksAmount;
};

#endif // TASKCENTER_H
