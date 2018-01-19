#ifndef AGVTASK_H
#define AGVTASK_H

#include <QObject>
#include <QDateTime>
#include <QList>

enum{
    AGV_TASK_STATUS_UNEXIST = -3,//不存在
    AGV_TASK_STATUS_UNEXCUTE=-2,//未执行
    AGV_TASK_STATUS_EXCUTING=-1,//正在执行
    AGV_TASK_STATUS_DONE=0,//完成
    AGV_TASK_STATUS_FAIL=1,//失败
    AGV_TASK_STATSU_CANCEL=2//取消
};

enum{
    AGV_TASK_WAIT_TYPE_TIME = 0,    // 达到后等待时间
    AGV_TASK_WAIT_TYPE_SIGNAL = 1,  //到达后等待信号
    AGV_TASK_WAIT_TYPE_NOWAIT = 2   //到达就可以了，就是不等待
};

enum{
    AGV_TASK_NODE_STATUS_UNDO = 0,
    AGV_TASK_NODE_STATUS_DOING = 1,
    AGV_TASK_NODE_STATUS_DONE = 2,
};

class TaskNode{
public:
    TaskNode() {}
    TaskNode(const TaskNode &b) {
        id = b.id;
        status = b.status;
        queueNumber = b.queueNumber;
        aimStation = b.aimStation;
        waitType = b.waitType;
        waitTime = b.waitTime;
        if(b.arriveTime.isValid())
            arriveTime = b.arriveTime;
        if(b.leaveTime.isValid())
            leaveTime = b.leaveTime;
    }

    bool operator < (const TaskNode &b)
    {
        return id<b.id;
    }

    bool operator == (const TaskNode &b){
        return id == b.id;
    }

    int id = 0;
    int status = 0;  //0未执行  1正在执行  2执行完成了
    int queueNumber = 0; //在这个任务中的序列号
    int aimStation = 0;//要去的位置()如果是0，说明没有节点任务
    int waitType = AGV_TASK_WAIT_TYPE_TIME;//到达该位置后的等待方式
    int waitTime = 30;//到达该位置后的等待时间(秒)
    QDateTime arriveTime;//到达时间
    QDateTime leaveTime;//离开时间
};

class AgvTask
{
public:
    AgvTask(){}

    AgvTask(const AgvTask &b) {
        id = b.id;
        status = b.status;
        excuteCar = b.excuteCar;
        circle = b.circle;
        nextTodoIndex = b.nextTodoIndex;
        lastDoneIndex = b.lastDoneIndex;
        currentDoingIndex = b.currentDoingIndex;
        if(b.produceTime.isValid())
            produceTime = b.produceTime;
        if(b.doneTime.isValid())
            doneTime = b.doneTime;
        if(b.doTime.isValid())
            doTime = b.doTime;

        for(int i=0;i<b.taskNodes.length();++i)
        {
            TaskNode *n = new TaskNode(*(b.taskNodes.at(i)));
            taskNodes.append(n);
        }

        for(int i=0;i<b.taskNodesBackup.length();++i)
        {
            TaskNode *n = new TaskNode(*(b.taskNodesBackup.at(i)));
            taskNodesBackup.append(n);
        }

    }

    //对其进行排序时，采用从小到大排序，就是a<b 则a先执行
    bool operator < (const AgvTask &b)
    {
        if(priority==b.priority)
            return id<b.id;
        return priority>b.priority;
    }

    bool operator == (const AgvTask &b){
        return id == b.id;
    }

    int id = 0;
    QDateTime produceTime;
    QDateTime doneTime;
    QDateTime doTime;
    int excuteCar = 0;
    int status = AGV_TASK_STATUS_UNEXCUTE;
    bool circle = false;////是否是循环任务(如果是循环任务呢？)
    int nextTodoIndex = 0;
    int lastDoneIndex = -1;
    int currentDoingIndex = -1;
    int priority;/////优先级,数字越大则优先级越高

    QList<TaskNode *> taskNodes;
    QList<TaskNode *> taskNodesBackup;//循环任务的任务节点备份。取出来循环时，请重新赋值ID

    bool isDone(){return currentDoingIndex>=taskNodes.length();}


};

#endif // AGVTASK_H
