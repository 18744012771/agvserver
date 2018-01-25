#include "taskmaker.h"
#include "taskmakerworker.h"
#include "util/global.h"

TaskMaker::TaskMaker() : hasInit(false)
{
    TaskMakerWorker *worker = new TaskMakerWorker;
    worker->moveToThread(&workerThread);

    connect(&workerThread, &QThread::finished, worker, &QObject::deleteLater);

    connect(this, &TaskMaker::sigInit, worker, &TaskMakerWorker::init);
    connect(this, &TaskMaker::sigTaskAccept, worker, &TaskMakerWorker::taskAccept);
    connect(this, &TaskMaker::sigTaskStart, worker, &TaskMakerWorker::taskStart);
    connect(this, &TaskMaker::sigTaskErrorEmpty, worker, &TaskMakerWorker::taskErrorEmpty);
    connect(this, &TaskMaker::sigTaskErrorFull, worker, &TaskMakerWorker::taskErrorFull);
    connect(this, &TaskMaker::sigTaskFinish, worker, &TaskMakerWorker::taskFinish);

    connect(worker, &TaskMakerWorker::sigInitResult, this, &TaskMaker::onInitResults);
    connect(worker, &TaskMakerWorker::sigNewTask, this, &TaskMaker::onNewTask);
    workerThread.start();
}

TaskMaker::~TaskMaker()
{
    workerThread.quit();
    workerThread.wait();
}

bool TaskMaker::init()
{
    emit sigInit();
    while(true){
        if(hasInit)break;
        QyhSleep(100);
    }
    if(initResult){
        connect(&g_taskCenter,SIGNAL(sigTaskStart(int,int)),this,SLOT(onTaskStart(int,int)));
        connect(&g_taskCenter,SIGNAL(sigTaskFinish(int)),this,SLOT(onTaskFinish(int)));
    }
    return initResult;
}

void TaskMaker::onInitResults(bool b)
{
    initResult = b;
    hasInit = true;
}

void TaskMaker::onNewTask(int workNo,int jobType,int startStation,int endStation,int priority)
{
    int taskid = g_taskCenter.makePickupTask(startStation,endStation,endStation);
    if(taskid>0)
    {
        //说明成功进入队列
        m_taskId_workNo[taskid] = workNo;
        emit sigTaskAccept(workNo);
    }
}


void TaskMaker::onTaskStart(int taskId,int agv)
{
    if(m_taskId_workNo.contains(taskId)){
        emit sigTaskStart(m_taskId_workNo[taskId],agv);
    }
}

void TaskMaker::onTaskFinish(int taskId)
{
    if(m_taskId_workNo.contains(taskId)){
        emit sigTaskFinish(m_taskId_workNo[taskId]);
    }
}
