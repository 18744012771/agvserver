#include "taskcenter.h"
#include "util/global.h"

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
    //载入最大的ID
    QString queryTaskMaxIdSql = "select max(id) from agv_task";
    QStringList params;
    QList<QStringList> result = g_sql->query(queryTaskMaxIdSql,params);
    if(result.length()>=0){
        maxId = 0;
    }else{
        if(result[0].length()<=0){
            maxId = 0;
        }else{
            maxId = result[0][0].toInt();
        }
    }

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

        if(ttask->taskNodesTodo.length()<=0){
            //这个任务已经完成了，那么就 从未分配放入到完成中。(这个任务还未开始就结束了！)
            unassignedTasks.removeAt(mmm);
            --mmm;
            continue;
        }

        Agv *bestCar = NULL;
        int minDis = distance_infinity;
        QList<int> path;
        int tempDis = distance_infinity;
        if(ttask->excuteCar()>0){//固定车辆去执行该任务
            Agv *excutecar = g_m_agvs[ttask->excuteCar()];
            if(excutecar->status()!=AGV_STATUS_IDLE)continue;
            QList<int> result;
            if(excutecar->nowStation()>0){
                result = g_agvMapCenter.getBestPath(excutecar->id(),excutecar->lastStation(),excutecar->nowStation(), ttask->taskNodesTodo[0].aimStation,tempDis,true);
            }else{
                result = g_agvMapCenter.getBestPath(excutecar->id(),excutecar->lastStation(),excutecar->nextStation(), ttask->taskNodesTodo[0].aimStation,tempDis,true);
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
                if(agv->nowStation()>0){
                    result = g_agvMapCenter.getBestPath(agv->id(),agv->lastStation(),agv->nowStation(), ttask->taskNodesTodo[0].aimStation,tempDis,true);
                }else{
                    result = g_agvMapCenter.getBestPath(agv->id(),agv->lastStation(),agv->nextStation(), ttask->taskNodesTodo[0].aimStation,tempDis,true);
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
            g_m_stations[ttask->taskNodesTodo[0].aimStation]->setOccuAgv(bestCar->id());//TODO:(什么时候释放呢？？)。重点来了
            //将起点，释放//TODO:这里释放起点其实是不合适的，应该在小车启动的时候，释放这个位置,虽然这里就差几行
            if(g_m_stations[bestCar->nowStation()]->occuAgv() == bestCar->id())g_m_stations[bestCar->nowStation()]->setOccuAgv(0);

            //对任务属性进行赋值
            ttask->setDoTime(QDateTime::currentDateTime());
            ttask->setStatus(AGV_TASK_STATUS_EXCUTING);
            ttask->setExcuteCar(bestCar->id());

            //对线路属性进行赋值         //4.把线路的反方向线路定为不可用
            for(int i=0;i<path.length();++i){
                int reverseLineKey = g_reverseLines[path[i] ];
                //将这条线路的可用性置为false
                g_m_lines[reverseLineKey]->setOccuAgv(bestCar->id());
            }
            //对车子属性进行赋值        //5.把这个车辆置为 非空闲,对车辆的其他信息进行更新
            bestCar->setStatus(AGV_STATUS_TASKING);
            bestCar->setTask(ttask->id());
            bestCar->setCurrentPath(path);
            //将任务移动到正在执行的任务//6.把这个任务定为doing。
            unassignedTasks.removeAt(mmm);
            mmm--;
            doingTasks.append(ttask);

            //TODO:要看返回的结果的！！！！！！ 如果失败了，要回滚上述所有操作！
            g_hrgAgvCenter.agvStartTask(bestCar->id(),path);

            //如果成功了，那么 将node设置为doing
            TaskNode node =  ttask->taskNodesTodo.at(0);
            ttask->taskNodeDoing = node;
            ttask->taskNodesTodo.removeAt(0);
        }
    }
}


////对于C类任务(直接去往目的地).它会先被放入todoAimtask中，等待分配车辆执行。如果分配到车辆了，这个任务会放入doingtasks中
////对于AB类任务(去A地装货，然后送到B地)，它会先被放入todoPickTasks中，等待分配车辆，如果分配到的车辆了，这个任务会放入doingtasks中，如果完成了装货，它会被放入todoAimTasks中，等待有可行线路去往目的地
//void TaskCenter::todoAimTaskProcess()
//{
//    //优先轮询D类的任务，就是指派指定车辆去某地的
//    for(int mmm=0;mmm<todoAimTasks.length();++mmm)
//    {
//        AgvTask *ttask = todoAimTasks.at(mmm);
//        if(ttask->type() == AGV_TASK_TYPE_D)
//        {
//            //查询指派车辆是否空闲
//            HrgAgv *bestCar;
//            for(QList<QObject *>::iterator itr =g_hrgagvs.begin();itr!=g_hrgagvs.end();++itr)
//            {
//                HrgAgv *agvPtr = (HrgAgv *)*itr;
//                if(agvPtr->key() == ttask->excuteCar()){
//                    bestCar = agvPtr;
//                    break;
//                }
//            }
//            if(bestCar==NULL || bestCar->status()!= HRG_AGV_STATUS_IDLE)continue;

//            //如果车辆可用的话，查询可用线路
//            int tempDis = distance_infinity;
//            QList<int> path = g_agvMapCenter.getBestLinePath(bestCar->lastStation(),bestCar->nowStation(), ttask->aimStation(),tempDis);
//            qDebug() << ".......................................";
//            qDebug() <<"agv car key = "<<bestCar->key();
//            qDebug() <<"lines=";
//            for(int i=0;i<path.length();++i){
//                qDebug() << " "<<path.at(i);
//            }
//            if(path.length()>0&&tempDis!=distance_infinity)
//            {
//                //如果线路可用的话，那么执行去
//                //将终点，占领
//                g_agvMapCenter.getStation(ttask->aimStation())->setOccupied(true);//TODO:(什么时候释放呢？？)。重点来了
//                //将起点，释放
//                g_agvMapCenter.getStation(bestCar->nowStation())->setOccupied(false);
//                //车辆赋值
//                bestCar->setStatus(HRG_AGV_STATUS_TASKING);
//                bestCar->setTask(ttask->key());
//                //任务赋值
//                ttask->setDoTime(QDateTime::currentDateTime());
//                ttask->setStatus(AGV_TASK_STATUS_EXCUTING);
//                ttask->setExcuteCar(bestCar->key());
//                ttask->setPathAim(path);
//                //线路赋值
//                for(int i=0;i<path.length();++i){
//                    int reverseLineKey = g_reverseLines[path[i] ];
//                    AgvLine *rLine = g_agvMapCenter.getLine(reverseLineKey);
//                    rLine->setReverseOccupy(true);
//                }
//                //站点赋值
//                //任务从todo移入doing中
//                todoAimTasks.removeAt(mmm);
//                mmm--;
//                doingTasks.append(ttask);
//                //TODO:这里启动车辆任务
//                g_hrgAgvCenter.agvStartTask(bestCar->key(),path);
//            }
//        }
//    }
//    for(int mmm=0;mmm<todoAimTasks.length();++mmm)
//    {
//        AgvTask *ttask = todoAimTasks.at(mmm);
//        HrgAgv *bestCar = NULL;
//        int minDis = distance_infinity;
//        QList<int> path;



//        if(ttask->type() == AGV_TASK_TYPE_C){
//            //查找最优车辆和线路
//            QList<HrgAgv *> idleAgvs = g_hrgAgvCenter.getIdleAgvs();
//            if(idleAgvs.length()<=0)//暂时没有可用车辆，直接退出对未分配的任务的操作
//                return ;

//            QList<HrgAgv *>::iterator ppos;
//            for(ppos = idleAgvs.begin();ppos!=idleAgvs.end();++ppos)
//            {
//                HrgAgv *agv = *ppos;
//                int tempDis = distance_infinity;
//                QList<int> result = g_agvMapCenter.getBestLinePath(agv->lastStation(),agv->nowStation(), ttask->aimStation(),tempDis);
//                qDebug() << ".......................................";
//                qDebug() <<"agv car key = "<<agv->key();
//                qDebug() <<"lines=";
//                for(int i=0;i<result.length();++i){
//                    qDebug() << " "<<result.at(i);
//                }
//                qDebug() << ".......................................";
//                if(result.length()>0&&tempDis!=distance_infinity)
//                {
//                    //一个可用线路的结果//当然并不一定是最优的线路
//                    if(tempDis < minDis){
//                        bestCar = agv;
//                        minDis = tempDis;
//                        path = result;
//                    }
//                }
//            }

//            //判断是否找到了最优的车辆和最优的线路
//            if(bestCar!=NULL && minDis != distance_infinity && path.length()>0)
//            {
//                //输出一下结果
//                qDebug() << QStringLiteral( "车辆")<<bestCar->key()<<QStringLiteral( " 线路");
//                for(int i=0;i<path.length();++i){
//                    qDebug() << path.at(i);
//                }
//                //这个任务要派给这个车了！接下来的事情是这些：
//                //将终点，占领
//                g_agvMapCenter.getStation(ttask->aimStation())->setOccupied(true);//TODO:(什么时候释放呢？？)。重点来了
//                //将起点，释放
//                g_agvMapCenter.getStation(bestCar->nowStation())->setOccupied(false);
//                //对任务属性进行赋值
//                ttask->setDoTime(QDateTime::currentDateTime());
//                ttask->setStatus(AGV_TASK_STATUS_EXCUTING);
//                ttask->setExcuteCar(bestCar->key());
//                ttask->setPathAim(path);
//                //对线路属性进行赋值         //4.把线路的反方向线路定为不可用
//                for(int i=0;i<path.length();++i){
//                    int reverseLineKey = g_reverseLines[path[i] ];
//                    //将这条线路的可用性置为false
//                    AgvLine *rLine = g_agvMapCenter.getLine(reverseLineKey);
//                    rLine->setReverseOccupy(true);
//                }
//                //对车子属性进行赋值        //5.把这个车辆置为 非空闲,对车辆的其他信息进行更新
//                bestCar->setStatus(HRG_AGV_STATUS_TASKING);
//                bestCar->setTask(ttask->key());
//                //将任务移动到正在执行的任务//6.把这个任务定为doing。
//                todoAimTasks.removeAt(mmm);
//                mmm--;
//                doingTasks.append(ttask);

//                //TODO:这里启动车辆任务
//                g_hrgAgvCenter.agvStartTask(bestCar->key(),path);
//            }

//        }else if(ttask->type() == AGV_TASK_TYPE_A_B && !ttask->goPickGoAim()){
//            //已经到达取货点完成取货，现在要去送货点。那么，车辆固定的，查找最优线路
//            for(QList<QObject *>::iterator itr= g_hrgagvs.begin();itr!=g_hrgagvs.end();++itr){
//                HrgAgv *agvTemp = (HrgAgv *)*itr;
//                if(agvTemp->key() == ttask->excuteCar()){
//                    bestCar = agvTemp;
//                    break;
//                }
//            }
//            if(bestCar==NULL)continue;
//            //查找线路
//            path = g_agvMapCenter.getBestLinePath(bestCar->lastStation(),bestCar->nowStation(), ttask->aimStation(),minDis);
//            qDebug() << "";
//            qDebug() << ".......................................";
//            qDebug() <<"agv car key = "<<bestCar->key();
//            qDebug() <<"minDis = "<<minDis;
//            qDebug() <<"lines=";
//            for(int i=0;i<path.length();++i){
//                qDebug() << " "<<path.at(i);
//            }
//            qDebug() << ".......................................";
//            qDebug() << "";
//            if(path.length()>0&&minDis!=distance_infinity)
//            {
//                //找到了线路。那么执行
//                //对任务属性进行赋值
//                //这个值已经在取货的时候设置了ttask->setDoTime(QDateTime::currentDateTime());
//                //同上 ttask->setStatus(AGV_TASK_STATUS_EXCUTING);
//                //同上 ttask->setExcuteCar(bestCar->key());
//                //将终点，占领
//                g_agvMapCenter.getStation(ttask->aimStation())->setOccupied(true);//TODO:(什么时候释放呢？？)。重点来了
//                //将起点，释放
//                g_agvMapCenter.getStation(bestCar->nowStation())->setOccupied(false);

//                ttask->setPathAim(path);
//                ttask->setGoPickGoAim(false);
//                //对线路属性进行赋值         //4.把线路的反方向线路定为不可用
//                for(int i=0;i<path.length();++i){
//                    int reverseLineKey = g_reverseLines[path[i] ];
//                    AgvLine *rLine = g_agvMapCenter.getLine(reverseLineKey);
//                    rLine->setReverseOccupy(true);
//                }
//                //将任务移动到正在执行的任务//6.把这个任务定为doing。
//                todoAimTasks.removeAt(mmm);
//                mmm--;
//                doingTasks.append(ttask);

//                //TODO:这里启动车辆任务
//                g_hrgAgvCenter.agvStartTask(bestCar->key(),path);
//            }
//        }
//    }
//}

int TaskCenter::makeAgvAimTask(int agvKey,int aimStation,int waitType,int waitTime)
{
    if(agvKey<=0||aimStation<=0)return -1;

    AgvTask *newtask = new AgvTask;
    TaskNode node;
    node.aimStation=aimStation;
    node.waitType=waitType;
    node.waitTime=waitTime;
    newtask->taskNodesTodo.append(node);
    newtask->setExcuteCar(agvKey);
    newtask->setId(++maxId);

    unassignedTasks.append(newtask);
    return newtask->id();
}

//由最方便的车辆到达某个站点
int TaskCenter::makeAimTask(int aimStation,int waitType,int waitTime)
{
    AgvTask *newtask = new AgvTask;
    TaskNode node;
    node.aimStation=aimStation;
    node.waitType=waitType;
    node.waitTime=waitTime;
    newtask->taskNodesTodo.append(node);
    newtask->setId(++maxId);

    unassignedTasks.append(newtask);
    return newtask->id();
}

///产生一个任务，这个任务的参数可能有很多，暂时只有一个，就是目的地,返回一个任务的ID。根据这个ID。可以取消任务
int TaskCenter::makePickupTask(int pickupStation,int aimStation,int waitTypePick,int waitTimePick,int waitTypeAim,int waitTimeAim)
{
    AgvTask *newtask = new AgvTask;

    TaskNode nodePickup;

    nodePickup.aimStation=pickupStation;
    nodePickup.waitType=waitTypePick;
    nodePickup.waitTime=waitTimePick;
    newtask->taskNodesTodo.append(nodePickup);

    TaskNode nodeAim;
    nodeAim.aimStation=aimStation;
    nodeAim.waitType=waitTypeAim;
    nodeAim.waitTime=waitTimeAim;
    newtask->taskNodesTodo.append(nodeAim);

    newtask->setId(++maxId);

    unassignedTasks.append(newtask);
    return newtask->id();
}

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
    QString querySql = "select status from agv_task where id= ?";
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
    QString querySql = "select excuteCar from agv_task where id= ?";
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
            saveTaskToDatabase(task);
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
            saveTaskToDatabase(task);
            //设置小车路径为空
            QList<int> nullPath;
            g_m_agvs[task->excuteCar()]->setTask(0);
            g_m_agvs[task->excuteCar()]->setCurrentPath(nullPath);
            if(g_m_agvs[task->excuteCar()]->status() == AGV_STATUS_TASKING)
                g_m_agvs[task->excuteCar()]->setStatus(AGV_STATUS_IDLE);
            //释放
            delete task;
            return 2;
        }
    }
    return 0;
}

bool TaskCenter::saveTaskToDatabase(AgvTask *task)
{
    QString insertSql = "insert into agv_task (produceTime,doneTime,doTime,excuteCar,status)values(?,?,?,?,?);";
    QStringList params;
    params<<task->produceTime().toString()<<task->doneTime().toString()<<QString("%1").arg( task->excuteCar())<<QString("%1").arg(task->status());
    if(!g_sql->exec(insertSql,params))return false;
    //TODO: 保存路径节点！！！


    return true;
}

void TaskCenter::carArriveStation(int car,int station)
{
    //小车
    Agv *agv = g_m_agvs[car];
    if(agv==NULL){return ;}
    //达到的站点
    AgvStation *sstation = g_m_stations[station];
    if(sstation==NULL){return ;}

    //小车是手动模式，那么就不管了
    if(agv->mode() == AGV_MODE_HAND){
        return ;
    }

    //小车当前的任务
    AgvTask *ttask = NULL;
    for(QList<AgvTask *>::iterator itr= doingTasks.begin();itr!=doingTasks.end();++itr){
        AgvTask *taskTemp = *itr;
        if(taskTemp->id() == agv->task())
        {
            ttask = taskTemp;
            break;
        }
    }

    //小车并没有任务
    if(ttask==NULL){return ;}

    //置线路位,更新道路占用的问题
    QList<int> pppath = agv->currentPath();//当前任务的线路
    int endStationId = ttask->taskNodeDoing.aimStation;//当前节点任务的终点

    //1.该站点是否在线路上
    bool findStation = false;
    for(int i=0;i<pppath.length();++i)
    {
        int iLine = pppath.at(i);
        if(g_m_lines[iLine]->endStation() == sstation->id()){
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
            agv->setCurrentPath(pppath);

            //将反向的线路置为可用
            int reverseLineKey = g_reverseLines[iLine];
            AgvLine *rLine = g_m_lines[reverseLineKey];
            rLine->setOccuAgv(car);

            AgvLine *line = g_m_lines[iLine];
            //如果是最后经过的这条线路，退出循环
            if(line->endStation() == sstation->id())
            {
                break;
            }else{
                //将经过的站点的占用释放
                if(g_m_stations[line->startStation()]->occuAgv() == car){
                    g_m_stations[line->startStation()]->setOccuAgv(0);
                }
                if(g_m_stations[line->endStation()]->occuAgv() == car){
                    g_m_stations[line->endStation()]->setOccuAgv(0);
                }
            }
        }

        //更新path后,将小车的任务内容进行更新
        //如果达到终点
        if(agv->currentPath().length() == 0){
            //到达当前 task_node的 目的地
            ttask->taskNodeDoing.arriveTime=QDateTime::currentDateTime();
        }
        //更新发给小车的内容
        g_msgCenter.taskControlCmd(car,false);
    }
}



void TaskCenter::doingTaskProcess()
{
    //对完成装货的车辆进行轮训，看是否启动了去往目的地
    //检查任务到达目标点位后，等待时间或者信号是否得到。
    //如果得到了，那就表示完成，退出正在执行的队列，如果是pickup完成了，放入todoAim队列中
    //qDebug() << QStringLiteral("被挂起任务数:")<<todoAimTasks.length()+todoPickTasks.length()<< QStringLiteral(" 正在执行的任务数:")<<doingTasks.length()<< QStringLiteral(" 已完成的任务数:")<<doneTasksAmount;
    for(int i=0;i<doingTasks.length();++i){
        AgvTask* task = doingTasks.at(i);
        if(task->taskNodesTodo.length()>0){
            //判断这个节点任务是否到达
            TaskNode node = task->taskNodesTodo.at(0);
            if(!node.arriveTime.isValid() && !node.arriveTime.isNull()){
                //已经到达
                //说明到达了这个节点的目的地
                if(node.waitType==AGV_TASK_WAIT_TYPE_TIME && node.arriveTime.secsTo(QDateTime::currentDateTime()) >= node.waitTime){
                    //如果是等待时间，并且时间等到了
                    //将这个节点移动到done里边
                    task->taskNodesTodo.removeAt(0);
                    task->taskNodesDone.append(node);
                    //计算新的线路给这个任务的下一条 节点任务
                    if(task->taskNodesTodo.length()==0){
                        //这个大任务已经完成了
                    }else{
                        //计算到下个节点任务的目的地的路径

                    }
                }
            }

        }else{
            //这个任务完成了
        }
    }
}


