#ifndef TASKCENTER_H
#define TASKCENTER_H

#include <QObject>
#include <QTimer>
#include <QMutex>
#include "bean/task.h"
#include "bean/agv.h"


class TaskCenter : public QObject
{
    Q_OBJECT
public:
    explicit TaskCenter(QObject *parent = nullptr);

    void init();

    //产生一个固定某辆车去到目的地的任务，车辆是agvId，目的地是aimStation
    int makeAgvAimTask(int agvId, int aimStation,int priority = Task::PRIORITY_NORMAL);

    //产生一个直接去到目的地的任务[车辆随意]，目的地是aimStation
    int makeAimTask(int aimStation,int priority = Task::PRIORITY_NORMAL);

    //产生一个指定车辆 取货送货的任务,pickupStation是取货点，aimStation是送货点
    int makeAgvPickupTask(int agvId, int pickupStation, int aimStation,int standByStation,int priority = Task::PRIORITY_NORMAL);

    //产生一个取货送货的任务,pickupStation是取货点，aimStation是送货点
    int makePickupTask(int pickupStation,int aimStation,int standByStation,int priority = Task::PRIORITY_NORMAL);

    //产生一个循环任务【制定车辆执行一个任务】
    int makeLoopTask(int agvId, int pickupStation, int aimStation, int standByStation, int priority = Task::PRIORITY_NORMAL);

    int queryTaskStatus(int taskId);//返回task的状态。

    int cancelTask(int taskId);//取消一个任务

    Task *queryUndoTask(int taskId);

    Task *queryDoingTask(int taskId);

    Task *queryDoneTask(int taskId);

    QList<Task *> getUnassignedTasks(){return unassignedTasks;}
    QList<Task *> getDoingTasks(){return  doingTasks;}
signals:
    void sigTaskStart(int,int);
    void sigTaskFinish(int);
public slots:
    void carArriveStation(int car,int station);
private slots:
    void unassignedTasksProcess();//未分配的任务
    void doingTaskProcess();//正在执行的任务(由于线路占用的问题，导致小车停在了某个位置，需要启动它)

private:
    //这里可以对任务进行扩展。将任务要做的事情做成一个不定的
    //对于C类任务(直接去往目的地).它会先被放入todoAimtask中，等待分配车辆执行。如果分配到车辆了，这个任务会放入doingtasks中
    //对于AB类任务(去A地装货，然后送到B地)，它会先被放入todoPickTasks中，等待分配车辆，如果分配到的车辆了，这个任务会放入doingtasks中，如果完成了装货，它会被放入todoAimTasks中，等待有可行线路去往目的地
    QList<Task *> unassignedTasks;           //未分配的任务
    QMutex uTaskMtx;

    QList<Task *> doingTasks;                //正在执行的任务
    QMutex dTaskMtx;

    QTimer taskProcessTimer;

    int doneTasksAmount;
};

#endif // TASKCENTER_H
