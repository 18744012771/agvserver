#include "taskcenter.h"
#include "util/global.h"
/////TODO:吧任务记录数据库
TaskCenter::TaskCenter(QObject *parent) : QObject(parent)
{

}

void TaskCenter::clear(){
    taskProcessTimer.stop();
    qDeleteAll(unassignedTasks);
    qDeleteAll(doingTasks);
    unassignedTasks.clear();
    doingTasks.clear();
}

void TaskCenter::init()
{
    clear();

    connect(&g_hrgAgvCenter,SIGNAL(carArriveStation(int,int)),this,SLOT(carArriveStation(int,int)));
    //每隔一秒对尚未分配进行的任务进行分配
    taskProcessTimer.setInterval(1000);
    connect(&taskProcessTimer,SIGNAL(timeout()),this,SLOT(unassignedTasksProcess()));
    connect(&taskProcessTimer,SIGNAL(timeout()),this,SLOT(doingTaskProcess()));
    taskProcessTimer.start();
}


void TaskCenter::unassignedTasksProcess()
{
    //遍历所有的未分配的任务，对他们和空闲车辆进行匹配。找到最合适的后，执行去
    for(int mmm=0;mmm<unassignedTasks.length();++mmm)
    {
        AgvTask *ttask = unassignedTasks.at(mmm);

        if(ttask->isDone()){
            //这个任务已经完成了，那么就 从未分配放入到完成中。(这个任务还未开始就结束了！)
            unassignedTasks.removeAt(mmm);
            //TODO:保存数据库
            --mmm;
            continue;
        }

        Agv *bestCar = NULL;
        int minDis = distance_infinity;
        QList<int> path;
        int tempDis = distance_infinity;
        TaskNode *nextNode = ttask->taskNodes[ttask->nextTodoIndex];
        if(ttask->excuteCar()>0){//固定车辆去执行该任务
            Agv *excutecar = g_m_agvs[ttask->excuteCar()];
            if(excutecar->myStatus!=AGV_STATUS_IDLE)continue;
            QList<int> result;
            if(excutecar->nowStation>0){
                result = g_agvMapCenter.getBestPath(excutecar->id,excutecar->lastStation,excutecar->nowStation,nextNode->aimStation,tempDis,false);
            }else{
                result = g_agvMapCenter.getBestPath(excutecar->id,excutecar->lastStation,excutecar->nextStation, nextNode->aimStation,tempDis,false);
            }
            if(result.length()>0&&tempDis!=distance_infinity){
                bestCar = excutecar;
                minDis = tempDis;
                path=result;
            }
        }else{
            //寻找最优车辆去执行任务
            QList<Agv *> idleAgvs = g_hrgAgvCenter.getIdleAgvs();
            if(idleAgvs.length()<=0)//暂时没有可用车辆，直接退出对未分配的任务的操作
                continue ;
            QList<Agv *>::iterator ppos;
            for(ppos = idleAgvs.begin();ppos!=idleAgvs.end();++ppos)
            {
                Agv *agv = *ppos;
                QList<int> result;
                if(agv->nowStation>0){
                    result = g_agvMapCenter.getBestPath(agv->id,agv->lastStation,agv->nowStation, nextNode->aimStation,tempDis,false);
                }else{
                    result = g_agvMapCenter.getBestPath(agv->id,agv->lastStation,agv->nextStation, nextNode->aimStation,tempDis,false);
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
            g_m_stations[nextNode->aimStation]->occuAgv = (bestCar->id);//TODO:(什么时候释放呢？？)。重点来了
            //将起点，释放//TODO:这里释放起点其实是不合适的，应该在小车启动的时候，释放这个位置,虽然这里就差几行
            if(g_m_stations[bestCar->nowStation]->occuAgv == bestCar->id)g_m_stations[bestCar->nowStation]->occuAgv = (0);

            //对任务属性进行赋值
            ttask->setDoTime(QDateTime::currentDateTime());
            ttask->setStatus(AGV_TASK_STATUS_EXCUTING);
            ttask->setExcuteCar(bestCar->id);
            //TODO:对任务节点进行赋值
            //更新上一个节点的离开时间
            if(ttask->lastDoneIndex!=-1){
                /////////ttask->lastDoneNode()->status = AGV_TASK_NODE_STATUS_DONE;
                ttask->taskNodes[ttask->lastDoneIndex]->leaveTime = QDateTime::currentDateTime();
            }

            //对线路属性进行赋值         //4.把线路的反方向线路定为不可用
            for(int i=0;i<path.length();++i){
                int reverseLineKey = g_reverseLines[path[i] ];
                //将这条线路的可用性置为false
                g_m_lines[reverseLineKey]->occuAgv=(bestCar->id);
            }
            //对车子属性进行赋值        //5.把这个车辆置为 非空闲,对车辆的其他信息进行更新
            bestCar->myStatus = (AGV_STATUS_TASKING);
            bestCar->task = (ttask->id());
            bestCar->currentPath = (path);
            //将任务移动到正在执行的任务//6.把这个任务定为doing。
            unassignedTasks.removeAt(mmm);
            mmm--;
            doingTasks.append(ttask);

            //TODO:要看返回的结果的！！！！！！ 如果失败了，要回滚上述所有操作！
            g_hrgAgvCenter.agvStartTask(bestCar->id,path);

            //如果成功了，那么 将node设置为doing
            //将未执行的节点，设置为正在执行
            ttask->lastDoneIndex = ttask->currentDoingIndex;
            ttask->currentDoingIndex = ttask->nextTodoIndex;
            ttask->nextTodoIndex+=1;
            nextNode->status = AGV_TASK_NODE_STATUS_DOING;
        }
    }
}

int TaskCenter::makeAgvAimTask(int agvKey,int aimStation,int waitType,int waitTime)
{
    if(agvKey<=0||aimStation<=0)return -1;

    //只有在make task的时候回 insert into agv——task。其他时候，全部都是update的！
    //这里只知道 任务的状态是未执行，产生时间是现在。直行车辆不知道
    AgvTask *newtask = new AgvTask;
    newtask->setProduceTime(QDateTime::currentDateTime());
    QString insertSql = "INSERT INTO agv_task (task_produceTime,task_status,task_excuteCar) VALUES (?,?,?);SELECT @@Identity;";
    QStringList params;
    params<<newtask->produceTime().toString(DATE_TIME_FORMAT)<<QString("%1").arg(AGV_TASK_STATUS_UNEXCUTE)<<QString("%1").arg(agvKey);
    QList<QStringList> result = g_sql->query(insertSql,params);
    if(result.length()<=0||result.at(0).length()<=0)
    {
        delete newtask;
        return -1;
    }
    newtask->setId(result.at(0).at(0).toInt());
    newtask->setExcuteCar(agvKey);

    TaskNode *node = new TaskNode;
    node->aimStation=aimStation;
    node->waitType=waitType;
    node->waitTime=waitTime;
    node->queueNumber=0;
    node->status = AGV_TASK_NODE_STATUS_UNDO;
    insertSql = "INSERT INTO agv_task_node(task_node_status,task_node_queuenumber,task_node_aimStation,task_node_waitType,task_node_waitTime,task_node_taskId) VALUES (?,?,?,?,?,?);SELECT @@Identity;";
    params.clear();
    params<<QString("%1").arg(node->status)<<QString("%1").arg(node->queueNumber)<<QString("%1").arg(node->aimStation)<<QString("%1").arg(node->waitType)<<QString("%1").arg(node->waitTime)<<QString("%1").arg(newtask->id());
    result = g_sql->query(insertSql,params);
    if(result.length()<=0||result.at(0).length()<=0){
        delete node;
        delete newtask;
        return -2;////!!!!!!!!!!!!!!!!!!!!!!!!!!
    }
    node->id = (result.at(0).at(0).toInt());
    newtask->taskNodes.append(node);


    unassignedTasks.append(newtask);
    return newtask->id();
}

//由最方便的车辆到达某个站点
int TaskCenter::makeAimTask(int aimStation,int waitType,int waitTime)
{
    //只有在make task的时候回 insert into agv——task。其他时候，全部都是update的！
    //这里只知道 任务的状态是未执行，产生时间是现在。直行车辆不知道
    AgvTask *newtask = new AgvTask;
    newtask->setProduceTime(QDateTime::currentDateTime());
    QString insertSql = "INSERT INTO agv_task (task_produceTime,task_status) VALUES (?,?);SELECT @@Identity;";
    QStringList params;
    params<<newtask->produceTime().toString(DATE_TIME_FORMAT)<<QString("%1").arg(AGV_TASK_STATUS_UNEXCUTE);
    QList<QStringList> result = g_sql->query(insertSql,params);
    if(result.length()<=0||result.at(0).length()<=0)
    {
        delete newtask;
        return -1;
    }
    newtask->setId(result.at(0).at(0).toInt());


    TaskNode *node = new TaskNode;
    node->aimStation=aimStation;
    node->waitType=waitType;
    node->waitTime=waitTime;
    node->queueNumber=0;
    node->status = AGV_TASK_NODE_STATUS_UNDO;
    insertSql = "INSERT INTO agv_task_node(task_node_status,task_node_queuenumber,task_node_aimStation,task_node_waitType,task_node_waitTime,task_node_taskId) VALUES (?,?,?,?,?,?);SELECT @@Identity;";
    params.clear();
    params<<QString("%1").arg(node->status)<<QString("%1").arg(node->queueNumber)<<QString("%1").arg(node->aimStation)<<QString("%1").arg(node->waitType)<<QString("%1").arg(node->waitTime)<<QString("%1").arg(newtask->id());
    result = g_sql->query(insertSql,params);
    if(result.length()<=0||result.at(0).length()<=0){
        delete node;
        delete newtask;
        return -2;////!!!!!!!!!!!!!!!!!!!!!!!!!!
    }
    node->id = (result.at(0).at(0).toInt());
    newtask->taskNodes.append(node);


    unassignedTasks.append(newtask);
    return newtask->id();
}
int TaskCenter::makeAgvPickupTask(int agvId,int pickupStation,int aimStation,int waitTypePick,int waitTimePick,int waitTypeAim,int waitTimeAim)
{
    AgvTask *newtask = new AgvTask;
    newtask->setProduceTime(QDateTime::currentDateTime());
    //只有在make task的时候回 insert into agv——task。其他时候，全部都是update的！
    //这里只知道 任务的状态是未执行，产生时间是现在。直行车辆不知道
    QString insertSql = "INSERT INTO agv_task (task_produceTime,task_status,task_excuteCar) VALUES (?,?,?);SELECT @@Identity;";
    QStringList params;
    params<<newtask->produceTime().toString(DATE_TIME_FORMAT)<<QString("%1").arg(AGV_TASK_STATUS_UNEXCUTE)<<QString("%1").arg(agvId);

    QList<QStringList> result = g_sql->query(insertSql,params);
    if(result.length()<=0||result.at(0).length()<=0)
    {
        delete newtask;
        return -1;
    }
    newtask->setId(result.at(0).at(0).toInt());
    newtask->setExcuteCar(agvId);
    TaskNode *node_pickup = new TaskNode;
    node_pickup->aimStation=pickupStation;
    node_pickup->waitType=waitTypePick;
    node_pickup->waitTime=waitTimePick;
    node_pickup->queueNumber=0;
    node_pickup->status = AGV_TASK_NODE_STATUS_UNDO;
    insertSql = "INSERT INTO agv_task_node(task_node_status,task_node_queuenumber,task_node_aimStation,task_node_waitType,task_node_waitTime,task_node_taskId) VALUES (?,?,?,?,?,?);SELECT @@Identity;";
    params.clear();
    params<<QString("%1").arg(node_pickup->status)<<QString("%1").arg(node_pickup->queueNumber)<<QString("%1").arg(node_pickup->aimStation)<<QString("%1").arg(node_pickup->waitType)<<QString("%1").arg(node_pickup->waitTime)<<QString("%1").arg(newtask->id());
    result = g_sql->query(insertSql,params);
    if(result.length()<=0||result.at(0).length()<=0){
        //删除task
        QString deleteSql = "delete from agv_task where id = ?;";
        params.clear();
        params<<QString("%1").arg(newtask->id());
        g_sql->exeSql(deleteSql,params);
        delete node_pickup;
        delete newtask;
        return -2;////!!!!!!!!!!!!!!!!!!!!!!!!!!
    }
    node_pickup->id = (result.at(0).at(0).toInt());
    newtask->taskNodes.append(node_pickup);

    TaskNode *node_aim = new TaskNode;
    node_aim->aimStation=aimStation;
    node_aim->waitType=waitTypeAim;
    node_aim->waitTime=waitTimeAim;
    node_aim->queueNumber=1;
    node_aim->status = AGV_TASK_NODE_STATUS_UNDO;
    insertSql = "INSERT INTO agv_task_node(task_node_status,task_node_queuenumber,task_node_aimStation,task_node_waitType,task_node_waitTime,task_node_taskId) VALUES (?,?,?,?,?,?);SELECT @@Identity;";
    params.clear();
    params<<QString("%1").arg(node_aim->status)<<QString("%1").arg(node_aim->queueNumber)<<QString("%1").arg(node_aim->aimStation)<<QString("%1").arg(node_aim->waitType)<<QString("%1").arg(node_aim->waitTime)<<QString("%1").arg(newtask->id());
    result = g_sql->query(insertSql,params);
    if(result.length()<=0||result.at(0).length()<=0){
        //删除task
        QString deleteSql = "delete from agv_task where id = ?;";
        params.clear();
        params<<QString("%1").arg(newtask->id());
        g_sql->exeSql(deleteSql,params);
        //删除第一个节点
        deleteSql = "delete from agv_task_node where id = ?;";
        params.clear();
        params<<QString("%1").arg(node_pickup->id);
        g_sql->exeSql(deleteSql,params);
        delete node_aim;
        delete newtask;
        return -3;////!!!!!!!!!!!!!!!!!!!!!!!!!!
    }
    node_aim->id = (result.at(0).at(0).toInt());
    newtask->taskNodes.append(node_aim);

    unassignedTasks.append(newtask);
    return newtask->id();
}

///产生一个任务，这个任务的参数可能有很多，暂时只有一个，就是目的地,返回一个任务的ID。根据这个ID。可以取消任务
int TaskCenter::makePickupTask(int pickupStation,int aimStation,int waitTypePick,int waitTimePick,int waitTypeAim,int waitTimeAim)
{
    AgvTask *newtask = new AgvTask;
    newtask->setProduceTime(QDateTime::currentDateTime());
    //只有在make task的时候回 insert into agv——task。其他时候，全部都是update的！
    //这里只知道 任务的状态是未执行，产生时间是现在。直行车辆不知道
    QString insertSql = "INSERT INTO agv_task (task_produceTime,task_status) VALUES (?,?);SELECT @@Identity;";
    QStringList params;
    params<<newtask->produceTime().toString(DATE_TIME_FORMAT)<<QString("%1").arg(AGV_TASK_STATUS_UNEXCUTE);

    QList<QStringList> result = g_sql->query(insertSql,params);
    if(result.length()<=0||result.at(0).length()<=0)
    {
        delete newtask;
        return -1;
    }
    newtask->setId(result.at(0).at(0).toInt());

    TaskNode *node_pickup = new TaskNode;
    node_pickup->aimStation=pickupStation;
    node_pickup->waitType=waitTypePick;
    node_pickup->waitTime=waitTimePick;
    node_pickup->queueNumber=0;
    node_pickup->status = AGV_TASK_NODE_STATUS_UNDO;
    insertSql = "INSERT INTO agv_task_node(task_node_status,task_node_queuenumber,task_node_aimStation,task_node_waitType,task_node_waitTime,task_node_taskId) VALUES (?,?,?,?,?,?);SELECT @@Identity;";
    params.clear();
    params<<QString("%1").arg(node_pickup->status)<<QString("%1").arg(node_pickup->queueNumber)<<QString("%1").arg(node_pickup->aimStation)<<QString("%1").arg(node_pickup->waitType)<<QString("%1").arg(node_pickup->waitTime)<<QString("%1").arg(newtask->id());
    result = g_sql->query(insertSql,params);
    if(result.length()<=0||result.at(0).length()<=0){
        //删除task
        QString deleteSql = "delete from agv_task where id = ?;";
        params.clear();
        params<<QString("%1").arg(newtask->id());
        g_sql->exeSql(deleteSql,params);
        delete node_pickup;
        delete newtask;
        return -2;////!!!!!!!!!!!!!!!!!!!!!!!!!!
    }
    node_pickup->id = (result.at(0).at(0).toInt());
    newtask->taskNodes.append(node_pickup);

    TaskNode *node_aim = new TaskNode;
    node_aim->aimStation=aimStation;
    node_aim->waitType=waitTypeAim;
    node_aim->waitTime=waitTimeAim;
    node_aim->queueNumber=1;
    node_aim->status = AGV_TASK_NODE_STATUS_UNDO;
    insertSql = "INSERT INTO agv_task_node(task_node_status,task_node_queuenumber,task_node_aimStation,task_node_waitType,task_node_waitTime,task_node_taskId) VALUES (?,?,?,?,?,?);SELECT @@Identity;";
    params.clear();
    params<<QString("%1").arg(node_aim->status)<<QString("%1").arg(node_aim->queueNumber)<<QString("%1").arg(node_aim->aimStation)<<QString("%1").arg(node_aim->waitType)<<QString("%1").arg(node_aim->waitTime)<<QString("%1").arg(newtask->id());
    result = g_sql->query(insertSql,params);
    if(result.length()<=0||result.at(0).length()<=0){
        //删除task
        QString deleteSql = "delete from agv_task where id = ?;";
        params.clear();
        params<<QString("%1").arg(newtask->id());
        g_sql->exeSql(deleteSql,params);
        //删除第一个节点
        deleteSql = "delete from agv_task_node where id = ?;";
        params.clear();
        params<<QString("%1").arg(node_pickup->id);
        g_sql->exeSql(deleteSql,params);
        delete node_aim;
        delete newtask;
        return -3;////!!!!!!!!!!!!!!!!!!!!!!!!!!
    }
    node_aim->id = (result.at(0).at(0).toInt());
    newtask->taskNodes.append(node_aim);

    unassignedTasks.append(newtask);
    return newtask->id();

}

AgvTask *TaskCenter::queryUndoTask(int taskId)
{
    for(int i=0;i<unassignedTasks.length();++i){
        if(unassignedTasks.at(i)->id() == taskId){
            return unassignedTasks.at(i);
        }
    }
    return NULL;
}

AgvTask *TaskCenter::queryDoingTask(int taskId)
{
    for(int i=0;i<doingTasks.length();++i){
        if(doingTasks.at(i)->id() == taskId){
            return doingTasks.at(i);
        }
    }
    return NULL;
}

AgvTask *TaskCenter::queryDoneTask(int taskId)
{
    //查找已完成的任务
    AgvTask *result = NULL;
    QString querySql = "select id,task_produceTime,task_doneTime,task_doTime,task_excuteCar,task_status from agv_task where id= ?";
    QStringList param;
    param.append(QString("%1").arg(taskId));
    QList<QStringList> queryresult = g_sql->query(querySql,param);
    if(queryresult.length()==0 ||queryresult.at(0).length()!=6)
        return result;
    //这个任务是存在的
    result = new AgvTask;
    result->setId(queryresult.at(0).at(0).toInt());
    result->setProduceTime(QDateTime::fromString(queryresult.at(0).at(1),DATE_TIME_FORMAT));
    result->setDoneTime(QDateTime::fromString(queryresult.at(0).at(2),DATE_TIME_FORMAT));
    result->setDoTime(QDateTime::fromString(queryresult.at(0).at(3),DATE_TIME_FORMAT));
    result->setExcuteCar(queryresult.at(0).at(4).toInt());
    result->setStatus(queryresult.at(0).at(5).toInt());

    //查询任务的节点信息
    querySql = "select task_node_status,task_node_queueNumber,task_node_aimStation,task_node_waitType,task_node_waitTime,task_node_arriveTime,task_node_leaveTime from agv_task_node where taskid= ? order by queueNumber";
    queryresult = g_sql->query(querySql,param);
    if(queryresult.length()==0 ||queryresult.at(0).length()!=7)
        return result;

    for(int i=0;i<queryresult.length();++i)
    {
        QStringList qsl = queryresult.at(i);
        if(qsl.length()!=7)continue;
        TaskNode *n = new TaskNode;
        n->status = qsl.at(0).toInt();
        n->queueNumber = qsl.at(1).toInt();
        n->aimStation = qsl.at(2).toInt();
        n->waitType = qsl.at(3).toInt();
        n->waitTime = qsl.at(4).toInt();
        n->arriveTime = QDateTime::fromString( qsl.at(5),DATE_TIME_FORMAT);
        n->leaveTime = QDateTime::fromString( qsl.at(6),DATE_TIME_FORMAT);
        result->taskNodes.push_back(n);
    }

    return result;
}

//AgvTask *TaskCenter::queryTask(int taskId)
//{
//    for(int i=0;i<unassignedTasks.length();++i){
//        if(unassignedTasks.at(i)->id() == taskId){
//            return unassignedTasks.at(i);
//        }
//    }
//    for(int i=0;i<doingTasks.length();++i){
//        if(doingTasks.at(i)->id() == taskId){
//            return doingTasks.at(i);
//        }
//    }
//    //查找已完成的任务
//    AgvTask *result = NULL;
//    QString querySql = "select id,produceTime,doneTime,doTime,excuteCar,status from agv_task where id= ?";
//    QStringList param;
//    param.append(QString("%1").arg(taskId));
//    QList<QStringList> queryresult = g_sql->query(querySql,param);
//    if(queryresult.length()==0 ||queryresult.at(0).length()!=6)
//        return result;
//    //这个任务是存在的
//    result = new AgvTask;
//    result->setId(queryresult.at(0).at(0).toInt());
//    result->setProduceTime(QDateTime::fromString(queryresult.at(0).at(1),DATE_TIME_FORMAT));
//    result->setDoneTime(QDateTime::fromString(queryresult.at(0).at(2),DATE_TIME_FORMAT));
//    result->setDoTime(QDateTime::fromString(queryresult.at(0).at(3),DATE_TIME_FORMAT));
//    result->setExcuteCar(queryresult.at(0).at(4).toInt());
//    result->setStatus(queryresult.at(0).at(5).toInt());

//    //查询任务的节点信息
//    querySql = "select status,queueNumber,aimStation,waitType,waitTime,arriveTime,leaveTime from agv_task_node where taskid= ? order by queueNumber";
//    queryresult = g_sql->query(querySql,param);
//    if(queryresult.length()==0 ||queryresult.at(0).length()!=7)
//        return result;

//    for(int i=0;i<queryresult.length();++i)
//    {
//        QStringList qsl = queryresult.at(i);
//        if(qsl.length()!=7)continue;
//        TaskNode n;
//        n.status = qsl.at(0).toInt();
//        n.queueNumber = qsl.at(1).toInt();
//        n.aimStation = qsl.at(2).toInt();
//        n.waitType = qsl.at(3).toInt();
//        n.waitTime = qsl.at(4).toInt();
//        n.arriveTime = QDateTime::fromString( qsl.at(5),DATE_TIME_FORMAT);
//        n.leaveTime = QDateTime::fromString( qsl.at(6),DATE_TIME_FORMAT);
//        if(n.status == 0)//未执行的节点
//            result->taskNodesTodo.push_back(n);
//        else if(n.status == 1)//正在执行
//            result->taskNodeDoing = n;
//        else if(n.status == 2)//执行过了的节点
//            result->taskNodeDone.push_back(n);
//    }

//    return result;
//}

//返回task的状态。
int TaskCenter::queryTaskStatus(int taskId)
{
    //查找未分配的任务
    for(int i=0;i<unassignedTasks.length();++i){
        if(unassignedTasks.at(i)->id() == taskId){
            return     AGV_TASK_STATUS_UNEXCUTE;
        }
    }
    //查找正在执行的任务
    for(int i=0;i<doingTasks.length();++i){
        if(doingTasks.at(i)->id() == taskId){
            return AGV_TASK_STATUS_EXCUTING;
        }
    }
    //查找已完成的任务
    QString querySql = "select task_status from agv_task where id= ?";
    QStringList param;
    param.append(QString("%1").arg(taskId));
    QList<QStringList> queryresult = g_sql->query(querySql,param);
    if(queryresult.length()==0 ||queryresult.at(0).length()==0)
        return AGV_TASK_STATUS_UNEXIST;
    return queryresult.at(0).at(0).toInt();
}

//查询这个任务是那辆车执行的
int TaskCenter::queryTaskCar(int taskId)
{
    //查找未分配的任务
    for(int i=0;i<unassignedTasks.length();++i){
        if(unassignedTasks.at(i)->id() == taskId){
            return unassignedTasks.at(i)->excuteCar();
        }
    }
    //查找正在执行的任务
    for(int i=0;i<doingTasks.length();++i){
        if(doingTasks.at(i)->id() == taskId){
            return doingTasks.at(i)->excuteCar();
        }
    }
    //查找已完成的任务
    QString querySql = "select task_excuteCar from agv_task where id= ?";
    QStringList param;
    param.append(QString("%1").arg(taskId));
    QList<QStringList> queryresult = g_sql->query(querySql,param);
    if(queryresult.length()==0 ||queryresult.at(0).length()==0)
        return 0;
    return queryresult.at(0).at(0).toInt();
}

//取消一个任务
int TaskCenter::cancelTask(int taskId)
{
    //查找未分配的任务
    for(int i=0;i<unassignedTasks.length();++i){
        if(unassignedTasks.at(i)->id() == taskId){
            AgvTask *task = unassignedTasks.at(i);
            //置为取消
            task->setStatus(AGV_TASK_STATSU_CANCEL);
            //移出待分配的队列
            unassignedTasks.removeAt(i);
            //保存数据库TODO:
            //TODO:
            //saveTaskToDatabase(task);
            //释放
            delete task;
            return 1;
        }
    }


    //查找正在执行的任务
    for(int i=0;i<doingTasks.length();++i){
        if(doingTasks.at(i)->id() == taskId){
            //1.告诉小车，任务取消了
            //TODO
            //2.对任务进行状态设置
            AgvTask *task = doingTasks.at(i);
            //置为取消
            task->setStatus(AGV_TASK_STATSU_CANCEL);
            //移出待分配的队列
            doingTasks.removeAt(i);
            //保存数据库TODO:
            //TODO
            //saveTaskToDatabase(task);
            //设置小车路径为空
            QList<int> nullPath;
            g_m_agvs[task->excuteCar()]->task = (0);
            g_m_agvs[task->excuteCar()]->currentPath = (nullPath);
            if(g_m_agvs[task->excuteCar()]->myStatus == AGV_STATUS_TASKING)
                g_m_agvs[task->excuteCar()]->myStatus = (AGV_STATUS_IDLE);
            //释放
            delete task;
            return 2;
        }
    }
    return 0;
}

bool TaskCenter::saveTaskToDatabase(AgvTask *task)
{
    //    QString insertSql = "insert into agv_task (task_node_produceTime,task_node_doneTime,doTime,excuteCar,status)values(?,?,?,?,?);";
    //    QStringList params;
    //    params<<task->produceTime().toString()<<task->doneTime().toString()<<QString("%1").arg( task->excuteCar())<<QString("%1").arg(task->status());
    //    if(!g_sql->exeSql(insertSql,params))return false;
    //TODO: 保存路径节点！！！


    return true;
}

void TaskCenter::carArriveStation(int car,int station)
{
    //小车
    if(!g_m_agvs.contains(car))return ;
    if(!g_m_stations.contains(station))return ;
    Agv *agv = g_m_agvs[car];
    //达到的站点
    AgvStation *sstation = g_m_stations[station];
    if(sstation==NULL){return ;}

    //小车是手动模式，那么就不管了
    if(agv->mode == AGV_MODE_HAND){
        return ;
    }

    //小车当前的任务
    AgvTask *ttask = NULL;
    for(QList<AgvTask *>::iterator itr= doingTasks.begin();itr!=doingTasks.end();++itr){
        AgvTask *taskTemp = *itr;
        if(taskTemp->id() == agv->task)
        {
            ttask = taskTemp;
            break;
        }
    }

    //小车并没有任务
    if(ttask==NULL){return ;}

    //置线路位,更新道路占用的问题
    QList<int> pppath = agv->currentPath;//当前任务的线路

    //1.该站点是否在线路上
    bool findStation = false;
    for(int i=0;i<pppath.length();++i)
    {
        int iLine = pppath.at(i);
        if(g_m_lines[iLine]->endStation == sstation->id){
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
            int reverseLineKey = g_reverseLines[iLine];
            AgvLine *rLine = g_m_lines[reverseLineKey];
            rLine->occuAgv = (0);

            AgvLine *line = g_m_lines[iLine];
            //如果是最后经过的这条线路，退出循环
            if(line->endStation == sstation->id)
            {
                if(g_m_stations[line->startStation]->occuAgv == car){
                    g_m_stations[line->startStation]->occuAgv = (0);
                }
                break;
            }else{
                //将经过的站点的占用释放
                if(g_m_stations[line->startStation]->occuAgv == car){
                    g_m_stations[line->startStation]->occuAgv = (0);
                }
                if(g_m_stations[line->endStation]->occuAgv == car){
                    g_m_stations[line->endStation]->occuAgv = (0);
                }
            }
        }

        //更新path后,将小车的任务内容进行更新
        //如果达到终点
        if(agv->currentPath.length() == 0){
            //到达当前 task_node的 目的地，设置当前任务节点的到达时间
            if(ttask->currentDoingIndex!=-1 && ttask->currentDoingIndex<ttask->taskNodes.length()){
                qDebug() << "ttask->currentDoingIndex="<<ttask->currentDoingIndex;

                QDateTime pp;
                qDebug() <<"pp="<< pp;

                qDebug() << "task="<<ttask->taskNodes[ttask->currentDoingIndex]->arriveTime;

                pp = QDateTime::currentDateTime();
                qDebug() <<"pp="<< pp;

                ttask->taskNodes[ttask->currentDoingIndex]->arriveTime=QDateTime::currentDateTime();
                qDebug() <<"task="<< ttask->taskNodes[ttask->currentDoingIndex]->arriveTime;

                qDebug()<<"";
            }
        }
        //更新发给小车的内容
        g_hrgAgvCenter.taskControlCmd(car,false);
    }
}


void TaskCenter::doingTaskProcess()
{
    //对完成装货的车辆进行轮训，看是否启动了去往目的地
    //检查任务到达目标点位后，等待时间或者信号是否得到。
    //如果得到了，那就表示完成，退出正在执行的队列，如果是pickup完成了，放入todoAim队列中
    for(int i=0;i<doingTasks.length();++i){
        AgvTask* task = doingTasks.at(i);
        if(task->currentDoingIndex!=-1 &&task->currentDoingIndex<task->taskNodes.length())
        {
            TaskNode *doingNode = task->taskNodes[task->currentDoingIndex];
            //判断这个节点任务是否到达
            if(doingNode->arriveTime.isValid())
            {
                //已经到达
                //说明到达了这个节点的目的地
                if(doingNode->waitType==AGV_TASK_WAIT_TYPE_TIME && doingNode->arriveTime.secsTo(QDateTime::currentDateTime()) >= doingNode->waitTime)
                {
                    //如果是等待时间，并且时间等到了
                    //将这个节点移动到done里边
                    doingNode->status = AGV_TASK_NODE_STATUS_DONE;//这个节点任务已经完成了

                    //计算新的线路给这个任务的下一条 节点任务
                    if(task->nextTodoIndex >= task->taskNodes.length())
                    {
                        //这个大任务已经完成了
                        //TODO
                        //置任务状态
                        task->setDoneTime(QDateTime::currentDateTime());
                        task->setStatus(AGV_TASK_STATUS_DONE);

                        //保存任务到数据库 //TODO

                        //置车辆状态
                        if(g_m_agvs.contains(task->excuteCar())){
                            if(g_m_agvs[task->excuteCar()]->myStatus == AGV_STATUS_TASKING){
                                g_m_agvs[task->excuteCar()]->myStatus = AGV_STATUS_IDLE;
                            }
                        }

                        //将任务从doing列表移动到done中
                        doingTasks.removeAt(i--);
                    }else{
                        //计算到下个节点任务的目的地的路径
                        //给它一个下一个节点任务
                        TaskNode *nextNode = task->taskNodes[task->nextTodoIndex];
                        if(task->excuteCar()>0 && g_m_agvs.contains(task->excuteCar())){//固定车辆去执行该任务
                            int minDis = distance_infinity;
                            Agv *excutecar = g_m_agvs[task->excuteCar()];
                            QList<int> result;

                            if(excutecar->nowStation>0){
                                result = g_agvMapCenter.getBestPath(excutecar->id,excutecar->lastStation,excutecar->nowStation, nextNode->aimStation,minDis,false);
                            }else{
                                result = g_agvMapCenter.getBestPath(excutecar->id,excutecar->lastStation,excutecar->nextStation, nextNode->aimStation,minDis,false);
                            }
                            if(result.length()>0&&minDis!=distance_infinity)
                            {
                                //将终点，占领
                                g_m_stations[nextNode->aimStation]->occuAgv = (excutecar->id);//TODO:(什么时候释放呢？？)。重点来了
                                //将起点，释放//TODO:这里释放起点其实是不合适的，应该在小车启动的时候，释放这个位置,虽然这里就差几行
                                if(g_m_stations.contains(excutecar->nowStation))
                                    if(g_m_stations[excutecar->nowStation]->occuAgv == excutecar->id)
                                        g_m_stations[excutecar->nowStation]->occuAgv = (0);
                                //对任务属性进行赋值
                                task->setStatus(AGV_TASK_STATUS_EXCUTING);
                                //更新上一个节点的离开时间
                                if(task->lastDoneIndex!=-1 && task->lastDoneIndex < task->taskNodes.length())
                                {
                                    task->taskNodes[task->lastDoneIndex]->leaveTime = QDateTime::currentDateTime();
                                }
                                //对线路属性进行赋值
                                //4.把线路的反方向线路定为不可用
                                for(int i=0;i<result.length();++i){
                                    int reverseLineKey = g_reverseLines[result[i] ];
                                    //将这条线路的可用性置为false
                                    g_m_lines[reverseLineKey]->occuAgv=(excutecar->id);
                                }
                                //对车子属性进行赋值        //5.把这个车辆置为 非空闲,对车辆的其他信息进行更新
                                excutecar->myStatus = (AGV_STATUS_TASKING);
                                excutecar->currentPath = (result);
                                //TODO:要看返回的结果的！！！！！！ 如果失败了，要回滚上述所有操作！
                                g_hrgAgvCenter.agvStartTask(excutecar->id,result);

                                //如果成功了，那么 将node设置为doing
                                //将未执行的节点，设置为正在执行
                                task->lastDoneIndex = task->currentDoingIndex;
                                task->currentDoingIndex= task->nextTodoIndex;
                                task->nextTodoIndex++;
                                nextNode->status = AGV_TASK_NODE_STATUS_DOING;
                            }
                        }
                    }
                }else{
                    //等待signal的不管先
                    //TODO
                }
            }

        }else if(task->nextTodoIndex != -1 && task->nextTodoIndex<task->taskNodes.length()){
            //计算到下个节点任务的目的地的路径
            //给它一个下一个节点任务
            TaskNode *nextNode = task->taskNodes[task->nextTodoIndex];
            if(task->excuteCar()>0 && g_m_agvs.contains(task->excuteCar()))
            {//固定车辆去执行该任务
                int minDis = distance_infinity;
                Agv *excutecar = g_m_agvs[task->excuteCar()];
                QList<int> result;
                if(excutecar->nowStation>0){
                    result = g_agvMapCenter.getBestPath(excutecar->id,excutecar->lastStation,excutecar->nowStation, nextNode->aimStation,minDis,false);
                }else{
                    result = g_agvMapCenter.getBestPath(excutecar->id,excutecar->lastStation,excutecar->nextStation, nextNode->aimStation,minDis,false);
                }
                if(result.length()>0&&minDis!=distance_infinity){
                    //将终点，占领
                    g_m_stations[nextNode->aimStation]->occuAgv = (excutecar->id);//TODO:(什么时候释放呢？？)。重点来了
                    //将起点，释放//TODO:这里释放起点其实是不合适的，应该在小车启动的时候，释放这个位置,虽然这里就差几行
                    if(g_m_stations.contains(excutecar->nowStation))
                        if(g_m_stations[excutecar->nowStation]->occuAgv == excutecar->id)
                            g_m_stations[excutecar->nowStation]->occuAgv = (0);
                    //对任务属性进行赋值
                    task->setStatus(AGV_TASK_STATUS_EXCUTING);
                    //更新上一个节点的离开时间
                    if(task->lastDoneIndex!=-1 && task->lastDoneIndex<task->taskNodes.length()){
                        task->taskNodes[task->lastDoneIndex]->leaveTime = QDateTime::currentDateTime();
                    }
                    //对线路属性进行赋值
                    //4.把线路的反方向线路定为不可用
                    for(int i=0;i<result.length();++i){
                        int reverseLineKey = g_reverseLines[result[i] ];
                        //将这条线路的可用性置为false
                        g_m_lines[reverseLineKey]->occuAgv=(excutecar->id);
                    }
                    //对车子属性进行赋值        //5.把这个车辆置为 非空闲,对车辆的其他信息进行更新
                    excutecar->myStatus = (AGV_STATUS_TASKING);
                    excutecar->currentPath = (result);
                    //TODO:要看返回的结果的！！！！！！ 如果失败了，要回滚上述所有操作！
                    g_hrgAgvCenter.agvStartTask(excutecar->id,result);

                    //如果成功了，那么 将node设置为doing
                    //将未执行的节点，设置为正在执行
                    task->lastDoneIndex = task->currentDoingIndex;
                    task->currentDoingIndex = task->nextTodoIndex;
                    task->nextTodoIndex++;
                    nextNode->status = AGV_TASK_NODE_STATUS_DOING;
                }
            }
        }else
        {
            //这个任务完成了
            //置任务状态
            task->setDoneTime(QDateTime::currentDateTime());
            task->setStatus(AGV_TASK_STATUS_DONE);

            //保存任务到数据库 //TODO

            //置车辆状态
            if(g_m_agvs.contains(task->excuteCar()))
            {
                if(g_m_agvs[task->excuteCar()]->myStatus == AGV_STATUS_TASKING)
                {
                    g_m_agvs[task->excuteCar()]->myStatus = AGV_STATUS_IDLE;
                }
            }
            //将任务从doing列表移动到done中
            doingTasks.removeAt(i--);
        }
    }
}


