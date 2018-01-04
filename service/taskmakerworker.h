#ifndef TASKMAKERWORKER_H
#define TASKMAKERWORKER_H

#include <QObject>
#include <QTimer>
#include <QDateTime>

#include "sql/sqlserver.h"

class TaskInfo
{
public:
    TaskInfo(){}
    TaskInfo(const TaskInfo& b){
        workNo=b.workNo;
        jobType=b.jobType;
        jobStatus=b.jobStatus;
        startStation=b.startStation;
        endStation=b.endStation;
        ioTime=b.ioTime;
        finishTime=b.finishTime;
        allotedTime=b.allotedTime;
        agvNo=b.agvNo;
        priority=b.priority;
    }

    bool operator ==(const TaskInfo& b)
    {
        return workNo == b.workNo;
    }

    bool operator <(const TaskInfo& b)
    {
        return workNo < b.workNo;
    }
    int workNo;//作业代号
    int jobType;//作业类型
    int jobStatus;//作业状态
    int startStation;//起始位置点
    int endStation;//目标位置点
    QDateTime ioTime;//作业产生时间
    QDateTime finishTime;//作业完成时间
    QDateTime allotedTime;//分配时间
    int agvNo;//车号
    int priority;//优先级
};

class TaskMakerWorker : public QObject
{
    Q_OBJECT
public:
    explicit TaskMakerWorker(QObject *parent = nullptr);
    bool init();
signals:
    //上报产生一个新任务
    void sigNewTask(int workNo,int jobType,int startStation,int endStation,int priority);
    void sigInitResult(bool);
public slots:

    //检查是否有新的任务进来
    void check();

    //产生任务成功，就说明任务接收了
    void taskAccept(int work_no);

    //如果任务开始执行了，那么就任务start了
    void taskStart(int work_no,int agv);

    //出现两种错误
    void taskErrorEmpty(int work_no);  //取货取空
    void taskErrorFull(int work_no);   //放货发现满了

    //任务完成
    void taskFinish(int work_no);
private:
    QTimer *checkTimer;
    SqlServer *sqlServer;
    QMap<int,TaskInfo> taskinfos;
};

#endif // TASKMAKERWORKER_H
