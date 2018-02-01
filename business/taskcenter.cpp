#include "taskcenter.h"
#include "util/global.h"

TaskCenter::TaskCenter(QObject *parent) : QObject(parent)
{

}

void TaskCenter::init()
{
    connect(g_hrgAgvCenter,SIGNAL(carArriveStation(int,int)),this,SLOT(carArriveStation(int,int)));
    connect(g_hrgAgvCenter,SIGNAL(pickFinish(int)),this,SLOT(onPickFinish(int)));
    connect(g_hrgAgvCenter,SIGNAL(putFinish(int)),this,SLOT(onPutFinish(int)));
    connect(g_hrgAgvCenter,SIGNAL(standByFinish(int)),this,SLOT(onStandByFinish(int)));
    //每隔一秒对尚未分配进行的任务进行分配
    taskProcessTimer.setInterval(1000);
    connect(&taskProcessTimer,SIGNAL(timeout()),this,SLOT(unassignedTasksProcess()));
    taskProcessTimer.start();
}

int TaskCenter::makeAgvAimTask(int agvId, int aimStation, int priority)
{
    if(agvId<=0||aimStation<=0)return -1;

    Task *newtask = new Task;

    //赋值
    newtask->produceTime = (QDateTime::currentDateTime());
    newtask->standByStation = aimStation;
    newtask->priority = priority;
    newtask->excuteCar = agvId;
    newtask->currentDoIndex = Task::INDEX_GOING_STANDBY;

    //插入记录
    QString insertSql = "INSERT INTO agv_task (task_produceTime,task_excuteCar,task_status,task_circle,task_priority,task_currentDoIndex,task_getGoodStation,task_getGoodDirect,task_getGoodDistance,task_putGoodStation,task_putGoodDirect,task_putGoodDistance,task_standByStation) VALUES (?,?,?,?,?,?,?,?,?,?,?,?,?);SELECT @@Identity;";
    QList<QVariant> params;
    params<<newtask->produceTime
         <<newtask->excuteCar
        <<newtask->status
       <<newtask->circle
      <<newtask->priority
     <<newtask->currentDoIndex
    <<newtask->getGoodStation
    <<newtask->getGoodDirect
    <<newtask->getGoodDistance
    <<newtask->putGoodStation
    <<newtask->putGoodDirect
    <<newtask->putGoodDistance
    <<newtask->standByStation;
    QList<QList<QVariant> > result = g_sql->query(insertSql,params);
    if(result.length()<=0||result.at(0).length()<=0)
    {
        delete newtask;
        return -1;
    }
    newtask->id = (result.at(0).at(0).toInt());


    uTaskMtx.lock();
    unassignedTasks.append(newtask);
    qSort(unassignedTasks.begin(),unassignedTasks.end(),agvTaskLessThan);
    uTaskMtx.unlock();
    return newtask->id;
}

//由最方便的车辆到达某个站点
int TaskCenter::makeAimTask(int aimStation,int priority)
{
    if(aimStation<=0)return -1;

    Task *newtask = new Task;

    //赋值
    newtask->produceTime = (QDateTime::currentDateTime());
    newtask->standByStation = aimStation;
    newtask->priority = priority;
    newtask->currentDoIndex = Task::INDEX_GOING_STANDBY;

    //插入记录
    QString insertSql = "INSERT INTO agv_task (task_produceTime,task_excuteCar,task_status,task_circle,task_priority,task_currentDoIndex,task_getGoodStation,task_getGoodDirect,task_getGoodDistance,task_putGoodStation,task_putGoodDirect,task_putGoodDistance,task_standByStation) VALUES (?,?,?,?,?,?,?,?,?,?,?,?,?);SELECT @@Identity;";
    QList<QVariant> params;
    params<<newtask->produceTime
         <<newtask->excuteCar
        <<newtask->status
       <<newtask->circle
      <<newtask->priority
     <<newtask->currentDoIndex
    <<newtask->getGoodStation
    <<newtask->getGoodDirect
    <<newtask->getGoodDistance
    <<newtask->putGoodStation
    <<newtask->putGoodDirect
    <<newtask->putGoodDistance
    <<newtask->standByStation;
    QList<QList<QVariant> > result = g_sql->query(insertSql,params);
    if(result.length()<=0||result.at(0).length()<=0)
    {
        delete newtask;
        return -1;
    }
    newtask->id = (result.at(0).at(0).toInt());

    uTaskMtx.lock();
    unassignedTasks.append(newtask);
    qSort(unassignedTasks.begin(),unassignedTasks.end(),agvTaskLessThan);
    uTaskMtx.unlock();
    return newtask->id;
}

int TaskCenter::makeAgvPickupTask(int agvId,int pickupStation,int aimStation,int standByStation,int priority)
{
    Task *newtask = new Task;

    //赋值
    newtask->produceTime = (QDateTime::currentDateTime());
    newtask->excuteCar = agvId;
    newtask->getGoodStation = pickupStation;
    newtask->putGoodStation = aimStation;
    newtask->standByStation = standByStation;
    newtask->priority = priority;
    newtask->currentDoIndex = Task::INDEX_GETTING_GOOD;

    //插入记录
    QString insertSql = "INSERT INTO agv_task (task_produceTime,task_excuteCar,task_status,task_circle,task_priority,task_currentDoIndex,task_getGoodStation,task_getGoodDirect,task_getGoodDistance,task_putGoodStation,task_putGoodDirect,task_putGoodDistance,task_standByStation) VALUES (?,?,?,?,?,?,?,?,?,?,?,?,?);SELECT @@Identity;";
    QList<QVariant> params;
    params<<newtask->produceTime
         <<newtask->excuteCar
        <<newtask->status
       <<newtask->circle
      <<newtask->priority
     <<newtask->currentDoIndex
    <<newtask->getGoodStation
    <<newtask->getGoodDirect
    <<newtask->getGoodDistance
    <<newtask->putGoodStation
    <<newtask->putGoodDirect
    <<newtask->putGoodDistance
    <<newtask->standByStation;
    QList<QList<QVariant> > result = g_sql->query(insertSql,params);
    if(result.length()<=0||result.at(0).length()<=0)
    {
        delete newtask;
        return -1;
    }
    newtask->id = (result.at(0).at(0).toInt());


    uTaskMtx.lock();
    unassignedTasks.append(newtask);
    qSort(unassignedTasks.begin(),unassignedTasks.end(),agvTaskLessThan);
    uTaskMtx.unlock();
    return newtask->id;
}


///产生一个任务，这个任务的参数可能有很多，暂时只有一个，就是目的地,返回一个任务的ID。根据这个ID。可以取消任务
int TaskCenter::makePickupTask(int pickupStation, int aimStation, int standByStation, int priority)
{
    Task *newtask = new Task;

    //赋值
    newtask->produceTime = (QDateTime::currentDateTime());
    newtask->getGoodStation = pickupStation;
    newtask->putGoodStation = aimStation;
    newtask->standByStation = standByStation;
    newtask->priority = priority;
    newtask->currentDoIndex = Task::INDEX_GETTING_GOOD;

    //插入记录
    QString insertSql = "INSERT INTO agv_task (task_produceTime,task_excuteCar,task_status,task_circle,task_priority,task_currentDoIndex,task_getGoodStation,task_getGoodDirect,task_getGoodDistance,task_putGoodStation,task_putGoodDirect,task_putGoodDistance,task_standByStation) VALUES (?,?,?,?,?,?,?,?,?,?,?,?,?);SELECT @@Identity;";
    QList<QVariant> params;
    params<<newtask->produceTime
         <<newtask->excuteCar
        <<newtask->status
       <<newtask->circle
      <<newtask->priority
     <<newtask->currentDoIndex
    <<newtask->getGoodStation
    <<newtask->getGoodDirect
    <<newtask->getGoodDistance
    <<newtask->putGoodStation
    <<newtask->putGoodDirect
    <<newtask->putGoodDistance
    <<newtask->standByStation;
    QList<QList<QVariant> > result = g_sql->query(insertSql,params);
    if(result.length()<=0||result.at(0).length()<=0)
    {
        delete newtask;
        return -1;
    }
    newtask->id = (result.at(0).at(0).toInt());

    uTaskMtx.lock();
    unassignedTasks.append(newtask);
    qSort(unassignedTasks.begin(),unassignedTasks.end(),agvTaskLessThan);
    uTaskMtx.unlock();
    return newtask->id;
}


int TaskCenter::makeLoopTask(int agvId, int pickupStation, int aimStation, int standByStation, int priority)
{
    //产生一个一直循环的任务
    Task *newtask = new Task;
    //赋值
    newtask->produceTime = (QDateTime::currentDateTime());
    newtask->excuteCar = agvId;
    newtask->getGoodStation = pickupStation;
    newtask->putGoodStation = aimStation;
    newtask->standByStation = standByStation;
    newtask->priority = priority;
    newtask->currentDoIndex = Task::INDEX_GETTING_GOOD;
    newtask->circle = true;

    //插入记录
    QString insertSql = "INSERT INTO agv_task (task_produceTime,task_excuteCar,task_status,task_circle,task_priority,task_currentDoIndex,task_getGoodStation,task_getGoodDirect,task_getGoodDistance,task_putGoodStation,task_putGoodDirect,task_putGoodDistance,task_standByStation) VALUES (?,?,?,?,?,?,?,?,?,?,?,?,?);SELECT @@Identity;";
    QList<QVariant> params;
    params<<newtask->produceTime
         <<newtask->excuteCar
        <<newtask->status
       <<newtask->circle
      <<newtask->priority
     <<newtask->currentDoIndex
    <<newtask->getGoodStation
    <<newtask->getGoodDirect
    <<newtask->getGoodDistance
    <<newtask->putGoodStation
    <<newtask->putGoodDirect
    <<newtask->putGoodDistance
    <<newtask->standByStation;
    QList<QList<QVariant> > result = g_sql->query(insertSql,params);
    if(result.length()<=0||result.at(0).length()<=0)
    {
        delete newtask;
        return -1;
    }
    newtask->id = (result.at(0).at(0).toInt());

    uTaskMtx.lock();
    unassignedTasks.append(newtask);
    qSort(unassignedTasks.begin(),unassignedTasks.end(),agvTaskLessThan);
    uTaskMtx.unlock();
    return newtask->id;
}


Task *TaskCenter::queryUndoTask(int taskId)
{
    Task *t = NULL;
    uTaskMtx.lock();
    for(int i=0;i<unassignedTasks.length();++i){
        if(unassignedTasks.at(i)->id == taskId){
            t = unassignedTasks.at(i);
            break;
        }
    }
    uTaskMtx.unlock();
    return t;
}

Task *TaskCenter::queryDoingTask(int taskId)
{
    Task *t = NULL;
    dTaskMtx.lock();
    for(int i=0;i<doingTasks.length();++i){
        if(doingTasks.at(i)->id == taskId){
            t = doingTasks.at(i);
        }
    }
    dTaskMtx.unlock();
    return t;
}

Task *TaskCenter::queryDoneTask(int taskId)
{
    //查找已完成的任务
    Task *result = NULL;
    QString querySql = "select id,task_produceTime,task_doTime,task_doneTime,task_excuteCar,task_status,task_circle,task_priority,task_currentDoIndex,task_getGoodStation,task_getGoodDirect,task_getGoodDistance,task_getStartTime,task_getFinishTime,task_putGoodStation,task_putGoodDirect,task_putGoodDistance,task_putStartTime,task_putFinishTime,task_standByStation,task_standByStartTime,task_standByFinishTime from agv_task where id= ?";
    QList<QVariant> param;
    param.append(taskId);
    QList<QList<QVariant>> queryresult = g_sql->query(querySql,param);
    if(queryresult.length()==0 ||queryresult.at(0).length()!=22)
        return result;
    //这个任务是存在的
    result = new Task;

    result->id = (queryresult.at(0).at(0).toInt());
    result->produceTime = (queryresult.at(0).at(1).toDateTime());
    result->doTime = (queryresult.at(0).at(2).toDateTime());
    result->doneTime = (queryresult.at(0).at(3).toDateTime());
    result->excuteCar = (queryresult.at(0).at(4).toInt());
    result->status = (queryresult.at(0).at(5).toInt());
    result->circle = (queryresult.at(0).at(6).toBool());
    result->priority = (queryresult.at(0).at(7).toInt());
    result->currentDoIndex = (queryresult.at(0).at(8).toInt());
    result->getGoodStation = (queryresult.at(0).at(9).toInt());
    result->getGoodDirect = (queryresult.at(0).at(10).toInt());
    result->getGoodDistance = (queryresult.at(0).at(11).toInt());
    result->getStartTime = (queryresult.at(0).at(12).toDateTime());
    result->getFinishTime = (queryresult.at(0).at(13).toDateTime());
    result->putGoodStation = (queryresult.at(0).at(14).toInt());
    result->putGoodDirect = (queryresult.at(0).at(15).toInt());
    result->putGoodDistance = (queryresult.at(0).at(16).toInt());
    result->putStartTime = (queryresult.at(0).at(17).toDateTime());
    result->putFinishTime = (queryresult.at(0).at(18).toDateTime());
    result->standByStation = (queryresult.at(0).at(19).toInt());
    result->standByStartTime = (queryresult.at(0).at(20).toDateTime());
    result->standByFinishTime = (queryresult.at(0).at(21).toDateTime());

    return result;
}

//返回task的状态。
int TaskCenter::queryTaskStatus(int taskId)
{
    //查找未分配的任务
    uTaskMtx.lock();
    for(int i=0;i<unassignedTasks.length();++i){
        if(unassignedTasks.at(i)->id == taskId){
            uTaskMtx.unlock();
            return Task::AGV_TASK_STATUS_UNEXCUTE;
        }
    }
    uTaskMtx.unlock();
    //查找正在执行的任务
    dTaskMtx.lock();
    for(int i=0;i<doingTasks.length();++i){
        if(doingTasks.at(i)->id == taskId){
            dTaskMtx.unlock();
            return Task::AGV_TASK_STATUS_EXCUTING;
        }
    }
    dTaskMtx.unlock();
    //查找已完成的任务
    QString querySql = "select task_status from agv_task where id= ?";
    QList<QVariant> param;
    param.append(taskId);
    QList<QList<QVariant> > queryresult = g_sql->query(querySql,param);
    if(queryresult.length()==0 ||queryresult.at(0).length()==0)
        return Task::AGV_TASK_STATUS_UNEXIST;
    return queryresult.at(0).at(0).toInt();
}

//取消一个任务
int TaskCenter::cancelTask(int taskId)
{
    //查找未分配的任务
    uTaskMtx.lock();
    for(int i=0;i<unassignedTasks.length();++i){
        if(unassignedTasks.at(i)->id == taskId){
            Task *task = unassignedTasks.at(i);
            //置为取消
            task->status = (Task::AGV_TASK_STATSU_CANCEL);
            //移出待分配的队列
            unassignedTasks.removeAt(i);
            //保存数据库:
            QString updateSql = "upadte agv_task set task_status=? where id = ?";
            QList<QVariant> args;
            args<<task->status<<task->id;
            if(!g_sql->exeSql(updateSql,args))
            {
                g_log->log(AGV_LOG_LEVEL_ERROR,"task with ID:"+QString("%1").arg(task->id)+" has been cancel,but save it to database fail!");
            }
            //释放
            delete task;
            uTaskMtx.unlock();
            return 1;
        }
    }
    uTaskMtx.unlock();

    //查找正在执行的任务
    dTaskMtx.lock();
    for(int i=0;i<doingTasks.length();++i)
    {
        if(doingTasks.at(i)->id == taskId){
            Task *task = doingTasks.at(i);

            ////1.告诉小车，任务取消了
            g_hrgAgvCenter->agvCancelTask(task->excuteCar);

            ////2.对任务进行状态设置
            //置为取消
            task->status = (Task::AGV_TASK_STATSU_CANCEL);
            //移出待分配的队列
            doingTasks.removeAt(i);
            //保存数据库:
            QString updateSql = "upadte agv_task set task_status=? where id = ?";
            QList<QVariant> args;
            args<<task->status<<task->id;
            if(!g_sql->exeSql(updateSql,args))
            {
                g_log->log(AGV_LOG_LEVEL_ERROR,"task with ID:"+QString("%1").arg(task->id)+" has been cancel,but save it to database fail!");
            }
            //释放
            delete task;
            dTaskMtx.unlock();
            return 2;
        }
    }
    dTaskMtx.unlock();
    return 0;
}

//释放道路占用
void TaskCenter::carArriveStation(int car,int station)
{
    if(!g_m_agvs.contains(car))return ;
    AgvAgent *agv =g_m_agvs[car];
    //达到的站点
    AgvStation sstation = g_agvMapCenter->getAgvStation(station);
    if(sstation.id<=0)return ;

    //小车是手动模式，那么就不管了
    if(agv->mode == AgvAgent::AGV_MODE_HAND){
        return ;
    }

    //置线路位,更新道路占用的问题
    QList<int> pppath = agv->currentPath;//当前任务的线路

    //1.该站点是否在线路上
    bool findStation = false;
    for(int i=0;i<pppath.length();++i)
    {
        int iLine = pppath.at(i);
        AgvLine line = g_agvMapCenter->getAgvLine(iLine);
        if(line.endStation == sstation.id){
            findStation = true;
            break;
        }
    }

    if(findStation)
    {
        //如果是路径上的站点，那么清楚之前经过的路径
        while(true)
        {
            QList<int>::iterator itr = pppath.begin();
            if(itr==pppath.end())break;
            int iLine = *itr;

            //删除经过的线路//记得后边更新到agv的path里边
            pppath.erase(itr);
            agv->currentPath = (pppath);

            //将反向的线路置为可用
            g_agvMapCenter->freeLineIfAgvOccu(iLine,car);

            AgvLine line = g_agvMapCenter->getAgvLine(iLine);
            //如果是最后经过的这条线路，退出循环
            if(line.endStation == sstation.id)
            {
                g_agvMapCenter->freeStationIfAgvOccu(line.startStation,car);
                break;
            }else{
                //将经过的站点的占用释放
                g_agvMapCenter->freeStationIfAgvOccu(line.startStation,car);
                g_agvMapCenter->freeStationIfAgvOccu(line.endStation,car);
            }
        }
    }
}


//取货OK，那么把任务设置成送货状态放回未分配队列
void  TaskCenter::onPickFinish(int agvId)
{
    AgvAgent *agv = g_m_agvs[agvId];
    Task *task =queryDoingTask(agv->task);
    if(task==NULL)return ;
    if(task->currentDoIndex != Task::INDEX_GETTING_GOOD)return ;

    dTaskMtx.lock();
    doingTasks.removeAll(task);
    dTaskMtx.unlock();

    task->currentDoIndex = Task::INDEX_PUTTING_GOOD;

    uTaskMtx.lock();
    unassignedTasks.append(task);
    uTaskMtx.unlock();
}

//放货OK，那么把任务设置成去到固定地点，放回未分配队列
void  TaskCenter::onPutFinish(int agvId)
{
    AgvAgent *agv = g_m_agvs[agvId];
    Task *task =queryDoingTask(agv->task);
    if(task==NULL)return ;
    if(task->currentDoIndex != Task::INDEX_PUTTING_GOOD)return ;

    dTaskMtx.lock();
    doingTasks.removeAll(task);
    dTaskMtx.unlock();

    task->currentDoIndex = Task::INDEX_GOING_STANDBY;

    uTaskMtx.lock();
    unassignedTasks.append(task);
    uTaskMtx.unlock();
}

//任务完成了！
void  TaskCenter::onStandByFinish(int agvId)
{
    AgvAgent *agv = g_m_agvs[agvId];
    Task *task =queryDoingTask(agv->task);
    if(task==NULL)return ;
    if(task->currentDoIndex != Task::INDEX_GOING_STANDBY)return ;

    if(task->circle){
        dTaskMtx.lock();
        doingTasks.removeAll(task);
        dTaskMtx.unlock();

        task->currentDoIndex = Task::INDEX_GETTING_GOOD;

        uTaskMtx.lock();
        unassignedTasks.append(task);
        uTaskMtx.unlock();

    }else{
        dTaskMtx.lock();
        doingTasks.removeAll(task);
        dTaskMtx.unlock();
        delete task;
        task = NULL;
    }
}

//这里不怕unassignedTasksProcess和doingTaskProcess中两个锁死锁，是以为它俩是同在主线程中，所以不必担心死锁问题
void TaskCenter::unassignedTasksProcess()
{
    //遍历所有的未分配的任务，对他们和空闲车辆进行匹配。找到最合适的后，执行去
    uTaskMtx.lock();
    for(int mmm=0;mmm<unassignedTasks.length();++mmm)
    {
        Task *ttask = unassignedTasks.at(mmm);

        int aimStation = 0;
        if(ttask->currentDoIndex==Task::INDEX_GETTING_GOOD){
            aimStation=ttask->getGoodStation;
        }else if(ttask->currentDoIndex==Task::INDEX_PUTTING_GOOD){
            aimStation = ttask->putGoodStation;
        }else{
            aimStation = ttask->standByStation;
        }

        AgvAgent *bestCar = NULL;
        int minDis = distance_infinity;
        QList<int> path;
        int tempDis = distance_infinity;

        if(ttask->excuteCar>0){//固定车辆去执行该任务
            if(!g_m_agvs.contains(ttask->excuteCar))continue;
            AgvAgent *excutecar = g_m_agvs[ttask->excuteCar];
            if(excutecar==NULL)continue;
            if(excutecar->status!=AgvAgent::AGV_STATUS_IDLE)continue;
            QList<int> result;

            if(excutecar->nowStation>0){
                result = g_agvMapCenter->getBestPath(excutecar->id,excutecar->lastStation,excutecar->nowStation,aimStation,tempDis,false);
            }else{
                result = g_agvMapCenter->getBestPath(excutecar->id,excutecar->lastStation,excutecar->nextStation, aimStation,tempDis,false);
            }
            if(result.length()>0&&tempDis!=distance_infinity){
                bestCar = excutecar;
                minDis = tempDis;
                path=result;
            }
        }else{
            //寻找最优车辆去执行任务
            QList<AgvAgent *> idleAgvs = g_hrgAgvCenter->getIdleAgvs();
            if(idleAgvs.length()<=0)//暂时没有可用车辆，直接退出对未分配的任务的操作
                continue ;
            QList<AgvAgent *>::iterator ppos;
            for(ppos = idleAgvs.begin();ppos!=idleAgvs.end();++ppos)
            {
                AgvAgent *agv = *ppos;
                QList<int> result;
                if(agv->nowStation>0){
                    result = g_agvMapCenter->getBestPath(agv->id,agv->lastStation,agv->nowStation, aimStation,tempDis,false);
                }else{
                    result = g_agvMapCenter->getBestPath(agv->id,agv->lastStation,agv->nextStation, aimStation,tempDis,false);
                }
                if(result.length()>0&&tempDis!=distance_infinity)
                {
                    //一个可用线路的结果//当然并不一定是最优的线路
                    if(tempDis < minDis){
                        bestCar = agv;
                        minDis = tempDis;
                        path = result;
                    }
                }
            }
        }

        //判断是否找到了最优的车辆和最优的线路
        if(bestCar!=NULL && minDis != distance_infinity && path.length()>0)
        {
            //这个任务要派给这个车了！接下来的事情是这些：
            //TODO:!!!要求一下操作可以回滚，因为车辆可能不接受该任务！！！！！！！！！！！！！！

            //将终点，占领
            g_agvMapCenter->setStationOccuAgv(aimStation,bestCar->id);

            //将起点，释放 这里释放起点其实是不合适的，应该在小车启动的时候，释放这个位置,虽然这里就差几行
            g_agvMapCenter->freeStationIfAgvOccu(bestCar->nowStation,bestCar->id);

            //if(g_m_stations[bestCar->nowStation]->occuAgv == bestCar->id)g_m_stations[bestCar->nowStation]->occuAgv = (0);

            //对任务属性进行赋值
            ttask->doTime = (QDateTime::currentDateTime());
            ttask->status = (Task::AGV_TASK_STATUS_EXCUTING);
            ttask->excuteCar = (bestCar->id);

            //对线路属性进行赋值         //4.把线路的反方向线路定为占用
            for(int i=0;i<path.length();++i){
                g_agvMapCenter->setReverseOccuAgv(path[i],(bestCar->id));
            }
            //对车子属性进行赋值        //5.把这个车辆置为 非空闲,对车辆的其他信息进行更新
            bestCar->status = (AgvAgent::AGV_STATUS_TASKING);
            bestCar->task = (ttask->id);
            bestCar->currentPath = (path);
            //将任务移动到正在执行的任务//6.把这个任务定为doing。
            unassignedTasks.removeAt(mmm);
            mmm--;
            dTaskMtx.lock();
            doingTasks.append(ttask);
            dTaskMtx.unlock();
            //要看返回的结果的！！！！！！ 如果失败了，要回滚上述所有操作！太难了
            //TODO:
            g_hrgAgvCenter->agvStartTask(bestCar,ttask);

            emit sigTaskStart(ttask->id,ttask->excuteCar);
        }
    }
    uTaskMtx.unlock();
}


//void TaskCenter::doingTaskProcess()
//{
//    //对完成装货的车辆进行轮训，看是否启动了去往目的地
//    //检查任务到达目标点位后，等待时间或者信号是否得到。
//    //如果得到了，那就表示完成，退出正在执行的队列，如果是pickup完成了，放入todoAim队列中
//    static int duration  = 0;//每隔5秒任务进行一次重发.
//    bool resent = false;
//    if(++duration >5){
//        duration = 0;
//        resent = true;
//    }
//    dTaskMtx.lock();
//    for(int i=0;i<doingTasks.length();++i)
//    {
//        Task* task = doingTasks.at(i);



//        //        if(task->currentDoingIndex!=-1 &&task->currentDoingIndex<task->taskNodes.length())
//        //        {
//        //            TaskNode *doingNode = task->taskNodes[task->currentDoingIndex];
//        //            if(resent)
//        //            {
//        //                g_hrgAgvCenter->taskControlCmd(task->excuteCar);
//        //            }
//        //            //判断这个节点任务是否到达
//        //            if(doingNode->arriveTime.isValid())
//        //            {
//        //                //已经到达
//        //                //说明到达了这个节点的目的地
//        //                if(doingNode->waitType==AGV_TASK_WAIT_TYPE_TIME && doingNode->arriveTime.secsTo(QDateTime::currentDateTime()) >= doingNode->waitTime)
//        //                {
//        //                    //如果是等待时间，并且时间等到了
//        //                    //将这个节点移动到done里边
//        //                    doingNode->status = AGV_TASK_NODE_STATUS_DONE;//这个节点任务已经完成了

//        //                    //计算新的线路给这个任务的下一条 节点任务
//        //                    if(task->nextTodoIndex >= task->taskNodes.length())
//        //                    {
//        //                        //这个大任务已经完成了
//        //                        //置任务状态
//        //                        task->doneTime = (QDateTime::currentDateTime());
//        //                        task->status = (AGV_TASK_STATUS_DONE);

//        //                        emit sigTaskFinish(task->id);

//        //                        //置车辆状态
//        //                        if(g_m_agvs.contains(task->excuteCar)){
//        //                            if(g_m_agvs[task->excuteCar]->status == Agv::AGV_STATUS_TASKING){
//        //                                g_m_agvs[task->excuteCar]->status =Agv::AGV_STATUS_IDLE;
//        //                            }
//        //                        }

//        //                        if(!task->circle){
//        //                            QString updateSql = "upadte agv_task set task_status=?,task_doneTime=? where id = ?";
//        //                            QList<QVariant> args;
//        //                            args<<task->status<<task->doneTime<<task->id;
//        //                            if(!g_sql->exeSql(updateSql,args))
//        //                            {
//        //                                g_log->log(AGV_LOG_LEVEL_ERROR,"task with ID:"+QString("%1").arg(task->id)+" has been cancel,but save it to database fail!");
//        //                            }
//        //                            //将任务从doing列表移动到done中
//        //                            doingTasks.removeAt(i--);
//        //                        }else{
//        //                            task->status = Task::AGV_TASK_STATUS_UNEXCUTE;
//        //                            task->nextTodoIndex = 0;
//        //                            task->lastDoneIndex = -1;
//        //                            task->currentDoingIndex = -1;

//        //                            for(int i=0;i<task->taskNodesBackup.length();++i)
//        //                            {
//        //                                task->taskNodes.append(new TaskNode(*(task->taskNodesBackup.at(i))));
//        //                            }

//        //                            //将它从doing放入undo中
//        //                            //将任务从doing列表移动到done中
//        //                            doingTasks.removeAt(i--);
//        //                            uTaskMtx.lock();
//        //                            unassignedTasks.append(task);
//        //                            qSort(unassignedTasks.begin(),unassignedTasks.end(),agvTaskLessThan);
//        //                            uTaskMtx.unlock();
//        //                        }
//        //                    }else{
//        //                        //计算到下个节点任务的目的地的路径
//        //                        //给它一个下一个节点任务
//        //                        TaskNode *nextNode = task->taskNodes[task->nextTodoIndex];
//        //                        if(task->excuteCar>0 && g_m_agvs.contains(task->excuteCar)){//固定车辆去执行该任务
//        //                            int minDis = distance_infinity;
//        //                            Agv *excutecar = g_m_agvs[task->excuteCar];
//        //                            QList<int> result;

//        //                            if(excutecar->nowStation>0){
//        //                                result = g_agvMapCenter->getBestPath(excutecar->id,excutecar->lastStation,excutecar->nowStation, nextNode->aimStation,minDis,false);
//        //                            }else{
//        //                                result = g_agvMapCenter->getBestPath(excutecar->id,excutecar->lastStation,excutecar->nextStation, nextNode->aimStation,minDis,false);
//        //                            }
//        //                            if(result.length()>0&&minDis!=distance_infinity)
//        //                            {
//        //                                //将终点，占领
//        //                                g_m_stations[nextNode->aimStation]->occuAgv = (excutecar->id);//(什么时候释放呢？？)。重点来了
//        //                                //将起点，释放//TODO:这里释放起点其实是不合适的，应该在小车启动的时候，释放这个位置,虽然这里就差几行
//        //                                if(g_m_stations.contains(excutecar->nowStation))
//        //                                    if(g_m_stations[excutecar->nowStation]->occuAgv == excutecar->id)
//        //                                        g_m_stations[excutecar->nowStation]->occuAgv = (0);
//        //                                //对任务属性进行赋值
//        //                                task->status = (AGV_TASK_STATUS_EXCUTING);
//        //                                //更新上一个节点的离开时间
//        //                                if(task->lastDoneIndex!=-1 && task->lastDoneIndex < task->taskNodes.length())
//        //                                {
//        //                                    task->taskNodes[task->lastDoneIndex]->leaveTime = QDateTime::currentDateTime();
//        //                                }
//        //                                //对线路属性进行赋值
//        //                                //4.把线路的反方向线路定为不可用
//        //                                for(int i=0;i<result.length();++i){
//        //                                    g_agvMapCenter->setReverseOccuAgv(result[i],(excutecar->id));
//        //                                }
//        //                                //对车子属性进行赋值        //5.把这个车辆置为 非空闲,对车辆的其他信息进行更新
//        //                                excutecar->myStatus = (AGV_STATUS_TASKING);
//        //                                excutecar->currentPath = (result);
//        //                                //TODO 要看返回的结果的！！！！！！ 如果失败了，要回滚上述所有操作！
//        //                                g_hrgAgvCenter->agvStartTask(excutecar->id,result);

//        //                                //如果成功了，那么 将node设置为doing
//        //                                //将未执行的节点，设置为正在执行
//        //                                task->lastDoneIndex = task->currentDoingIndex;
//        //                                task->currentDoingIndex= task->nextTodoIndex;
//        //                                task->nextTodoIndex++;
//        //                                nextNode->status = AGV_TASK_NODE_STATUS_DOING;
//        //                            }
//        //                        }
//        //                    }
//        //                }else{
//        //                    //等待signal的不管先
//        //                    //TODO
//        //                }
//        //            }

//        //        }else if(task->nextTodoIndex != -1 && task->nextTodoIndex<task->taskNodes.length()){
//        //            //计算到下个节点任务的目的地的路径
//        //            //给它一个下一个节点任务
//        //            TaskNode *nextNode = task->taskNodes[task->nextTodoIndex];
//        //            if(task->excuteCar>0 && g_m_agvs.contains(task->excuteCar))
//        //            {//固定车辆去执行该任务
//        //                int minDis = distance_infinity;
//        //                Agv *excutecar = g_m_agvs[task->excuteCar];
//        //                QList<int> result;
//        //                if(excutecar->nowStation>0){
//        //                    result = g_agvMapCenter->getBestPath(excutecar->id,excutecar->lastStation,excutecar->nowStation, nextNode->aimStation,minDis,false);
//        //                }else{
//        //                    result = g_agvMapCenter->getBestPath(excutecar->id,excutecar->lastStation,excutecar->nextStation, nextNode->aimStation,minDis,false);
//        //                }
//        //                if(result.length()>0&&minDis!=distance_infinity){
//        //                    //将终点，占领
//        //                    g_m_stations[nextNode->aimStation]->occuAgv = (excutecar->id);//TODO:(什么时候释放呢？？)。重点来了
//        //                    //将起点，释放//TODO:这里释放起点其实是不合适的，应该在小车启动的时候，释放这个位置,虽然这里就差几行
//        //                    if(g_m_stations.contains(excutecar->nowStation))
//        //                        if(g_m_stations[excutecar->nowStation]->occuAgv == excutecar->id)
//        //                            g_m_stations[excutecar->nowStation]->occuAgv = (0);
//        //                    //对任务属性进行赋值
//        //                    task->status = (AGV_TASK_STATUS_EXCUTING);
//        //                    //更新上一个节点的离开时间
//        //                    if(task->lastDoneIndex!=-1 && task->lastDoneIndex<task->taskNodes.length()){
//        //                        task->taskNodes[task->lastDoneIndex]->leaveTime = QDateTime::currentDateTime();
//        //                    }
//        //                    //对线路属性进行赋值
//        //                    //4.把线路的反方向线路定为不可用
//        //                    for(int i=0;i<result.length();++i){
//        //                        g_agvMapCenter->setReverseOccuAgv(result[i],(excutecar->id));
//        //                    }
//        //                    //对车子属性进行赋值        //5.把这个车辆置为 非空闲,对车辆的其他信息进行更新
//        //                    excutecar->myStatus = (AGV_STATUS_TASKING);
//        //                    excutecar->currentPath = (result);
//        //                    //TODO:要看返回的结果的！！！！！！ 如果失败了，要回滚上述所有操作！
//        //                    g_hrgAgvCenter->agvStartTask(excutecar->id,result);

//        //                    //如果成功了，那么 将node设置为doing
//        //                    //将未执行的节点，设置为正在执行
//        //                    task->lastDoneIndex = task->currentDoingIndex;
//        //                    task->currentDoingIndex = task->nextTodoIndex;
//        //                    task->nextTodoIndex++;
//        //                    nextNode->status = AGV_TASK_NODE_STATUS_DOING;
//        //                }
//        //            }
//        //        }else
//        //        {

//        //            //这个任务完成了
//        //            //置任务状态
//        //            task->doneTime = (QDateTime::currentDateTime());
//        //            task->status = (AGV_TASK_STATUS_DONE);

//        //            //置车辆状态
//        //            if(g_m_agvs.contains(task->excuteCar))
//        //            {
//        //                if(g_m_agvs[task->excuteCar]->status == Agv::AGV_STATUS_TASKING)
//        //                {
//        //                    g_m_agvs[task->excuteCar]->status = Agv::AGV_STATUS_IDLE;
//        //                }
//        //            }

//        //            if(!task->circle){
//        //                emit sigTaskFinish(task->id);
//        //                QString updateSql = "upadte agv_task set task_status=?,task_doneTime=? where id = ?";
//        //                QList<QVariant> args;
//        //                args<<task->status<<task->doneTime<<task->id;
//        //                if(!g_sql->exeSql(updateSql,args))
//        //                {
//        //                    g_log->log(AGV_LOG_LEVEL_ERROR,"task with ID:"+QString("%1").arg(task->id)+" has been cancel,but save it to database fail!");
//        //                }
//        //                //将任务从doing列表移动到done中
//        //                doingTasks.removeAt(i--);
//        //            }else{
//        //                task->status = AGV_TASK_STATUS_UNEXCUTE;
//        //                task->nextTodoIndex = 0;
//        //                task->lastDoneIndex = -1;
//        //                task->currentDoingIndex = -1;
//        //                //清空taskNodes
//        //                qDeleteAll(task->taskNodes);
//        //                task->taskNodes.clear();

//        //                for(int i=0;i<task->taskNodesBackup.length();++i)
//        //                {
//        //                    task->taskNodes.append(new TaskNode(*(task->taskNodesBackup.at(i))));
//        //                }

//        //                //将它从doing放入undo中
//        //                //将任务从doing列表移动到done中
//        //                doingTasks.removeAt(i--);

//        //                uTaskMtx.lock();
//        //                unassignedTasks.append(task);
//        //                qSort(unassignedTasks.begin(),unassignedTasks.end(),agvTaskLessThan);
//        //                uTaskMtx.unlock();
//        //            }
//        //        }
//    }
//    dTaskMtx.unlock();
//}


