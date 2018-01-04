#include "taskmakerworker.h"
#include <QDateTime>
#include <assert.h>

TaskMakerWorker::TaskMakerWorker(QObject *parent) : QObject(parent),
    sqlServer(NULL),
    checkTimer(NULL)
{
}

bool TaskMakerWorker::init()
{
    sqlServer = new SqlServer;
    if(!sqlServer->createConnection()){
        emit sigInitResult(false);
        return false;
    }
    checkTimer = new QTimer;
    checkTimer->setInterval(1000);
    connect(checkTimer,&QTimer::timeout,this,&TaskMakerWorker::check);
    checkTimer->start();
    emit sigInitResult(true);
    return true;
}

//检查是否有新的任务进来
void TaskMakerWorker::check()
{
    QString querySql = "select wrk_no,JOB_TYPE,s_stn,t_stn,Priority,JOB_STATUS,io_time,FINISH_TIME,ALLOTED_TIME,AGV_Num from PDC_AGV_WORK where JOB_STATUS = 0 order by Priority desc,io_time asc;";
    QList<QVariant> params;
    QList<QList<QVariant>> result = sqlServer->query(querySql,params);
    if(result.length()>0)
    {
        for(int i=0;i<result.length();++i)
        {
            QList<QVariant> taskInfoList = result.at(i);
            assert(taskInfoList.length() == 10);
            TaskInfo info;
            //产生一个新任务！
            info.workNo = taskInfoList.at(0).toInt();
            info.jobType = taskInfoList.at(1).toInt();
            info.startStation = taskInfoList.at(2).toInt();
            info.endStation = taskInfoList.at(3).toInt();
            info.priority = taskInfoList.at(4).toInt();

            info.jobStatus = taskInfoList.at(5).toInt();
            info.ioTime = taskInfoList.at(6).toDateTime();
            info.finishTime = taskInfoList.at(7).toDateTime();
            info.allotedTime = taskInfoList.at(8).toDateTime();
            info.agvNo = taskInfoList.at(9).toInt();
            taskinfos[info.workNo] = info;
            emit sigNewTask(info.workNo,info.jobType,info.startStation,info.endStation,info.priority);
        }
    }
}

//产生任务成功，就说明任务接收了
void TaskMakerWorker::taskAccept(int work_no)
{
    taskinfos[work_no].jobStatus = 1;
    taskinfos[work_no].ioTime = QDateTime::currentDateTime();
    //更新任务状态
    QString updateSql = "update PDC_AGV_WORK set JOB_STATUS = 1,io_time=? where wrk_no=?;";
    QList<QVariant> params;
    params<<taskinfos[work_no].ioTime<<work_no;
    sqlServer->exeSql(updateSql,params);

    //插入到LOG里
    QString insertSql = "insert into PDC_AGV_WORK_LOG(wrk_no,JOB_TYPE,JOB_STATUS,s_stn,t_stn,io_time,FINISH_TIME,ALLOTED_TIME,AGV_Num,Priority) values(?,?,?,?,?,?,?,?,?);";
    params.clear();
    params<<taskinfos[work_no].workNo
         <<taskinfos[work_no].jobType
        <<taskinfos[work_no].jobStatus
       <<taskinfos[work_no].startStation
      <<taskinfos[work_no].endStation
     <<taskinfos[work_no].ioTime
    <<taskinfos[work_no].finishTime
    <<taskinfos[work_no].allotedTime
    <<taskinfos[work_no].agvNo
    <<taskinfos[work_no].priority;
    sqlServer->exeSql(insertSql,params);
}

//如果任务开始执行了，那么就任务start了
void TaskMakerWorker::taskStart(int work_no,int agv)
{
    taskinfos[work_no].jobStatus = 2;
    taskinfos[work_no].agvNo = agv;
    taskinfos[work_no].allotedTime = QDateTime::currentDateTime();
    QString updateSql = "update PDC_AGV_WORK set JOB_STATUS = ?,ALLOTED_TIME=?,AGV_Num=? where wrk_no=?;";
    QList<QVariant> params;
    params<<taskinfos[work_no].jobStatus<<taskinfos[work_no].allotedTime<<taskinfos[work_no].agvNo<<taskinfos[work_no].workNo;
    sqlServer->exeSql(updateSql,params);

    //更新到LOG里
    QString insertSql = "update PDC_AGV_WORK_LOG set JOB_TYPE=?,JOB_STATUS=?,s_stn=?,t_stn=?,io_time=?,FINISH_TIME=?,ALLOTED_TIME=?,AGV_Num=?,Priority=? wherewrk_no=?;";
    params.clear();
    params<<taskinfos[work_no].jobType
         <<taskinfos[work_no].jobStatus
        <<taskinfos[work_no].startStation
       <<taskinfos[work_no].endStation
      <<taskinfos[work_no].ioTime
     <<taskinfos[work_no].finishTime
    <<taskinfos[work_no].allotedTime
    <<taskinfos[work_no].agvNo
    <<taskinfos[work_no].priority
    <<taskinfos[work_no].workNo;
    sqlServer->exeSql(insertSql,params);
}

//出现两种错误
void TaskMakerWorker::taskErrorEmpty(int work_no)  //取货取空
{
    taskinfos[work_no].jobStatus = 3;
    QString updateSql = "update PDC_AGV_WORK set JOB_STATUS = ? where wrk_no=?;";
    QList<QVariant> params;
    params<<taskinfos[work_no].jobStatus<<taskinfos[work_no].workNo;
    sqlServer->exeSql(updateSql,params);

    //更新到LOG里
    QString insertSql = "update PDC_AGV_WORK_LOG set JOB_TYPE=?,JOB_STATUS=?,s_stn=?,t_stn=?,io_time=?,FINISH_TIME=?,ALLOTED_TIME=?,AGV_Num=?,Priority=? wherewrk_no=?;";
    params.clear();
    params<<taskinfos[work_no].jobType
         <<taskinfos[work_no].jobStatus
        <<taskinfos[work_no].startStation
       <<taskinfos[work_no].endStation
      <<taskinfos[work_no].ioTime
     <<taskinfos[work_no].finishTime
    <<taskinfos[work_no].allotedTime
    <<taskinfos[work_no].agvNo
    <<taskinfos[work_no].priority
    <<taskinfos[work_no].workNo;
    sqlServer->exeSql(insertSql,params);
}

void TaskMakerWorker::taskErrorFull(int work_no)   //放货发现满了
{
    taskinfos[work_no].jobStatus = 4;
    QString updateSql = "update PDC_AGV_WORK set JOB_STATUS = ? where wrk_no=?;";
    QList<QVariant> params;
    params<<taskinfos[work_no].jobStatus<<taskinfos[work_no].workNo;
    sqlServer->exeSql(updateSql,params);

    //更新到LOG里
    QString insertSql = "update PDC_AGV_WORK_LOG set JOB_TYPE=?,JOB_STATUS=?,s_stn=?,t_stn=?,io_time=?,FINISH_TIME=?,ALLOTED_TIME=?,AGV_Num=?,Priority=? wherewrk_no=?;";
    params.clear();
    params<<taskinfos[work_no].jobType
         <<taskinfos[work_no].jobStatus
        <<taskinfos[work_no].startStation
       <<taskinfos[work_no].endStation
      <<taskinfos[work_no].ioTime
     <<taskinfos[work_no].finishTime
    <<taskinfos[work_no].allotedTime
    <<taskinfos[work_no].agvNo
    <<taskinfos[work_no].priority
    <<taskinfos[work_no].workNo;
    sqlServer->exeSql(insertSql,params);
}

//任务完成
void TaskMakerWorker::taskFinish(int work_no)
{
    taskinfos[work_no].jobStatus = 5;
    taskinfos[work_no].finishTime = QDateTime::currentDateTime();
    QString updateSql = "update PDC_AGV_WORK set JOB_STATUS = ?,FINISH_TIME=? where wrk_no=?;";
    QList<QVariant> params;
    params<<taskinfos[work_no].jobStatus<<taskinfos[work_no].workNo;
    sqlServer->exeSql(updateSql,params);

    //更新到LOG里
    QString insertSql = "update PDC_AGV_WORK_LOG set JOB_TYPE=?,JOB_STATUS=?,s_stn=?,t_stn=?,io_time=?,FINISH_TIME=?,ALLOTED_TIME=?,AGV_Num=?,Priority=? wherewrk_no=?;";
    params.clear();
    params<<taskinfos[work_no].jobType
         <<taskinfos[work_no].jobStatus
        <<taskinfos[work_no].startStation
       <<taskinfos[work_no].endStation
      <<taskinfos[work_no].ioTime
     <<taskinfos[work_no].finishTime
    <<taskinfos[work_no].allotedTime
    <<taskinfos[work_no].agvNo
    <<taskinfos[work_no].priority
    <<taskinfos[work_no].workNo;
    sqlServer->exeSql(insertSql,params);
}
