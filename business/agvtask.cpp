#include "agvtask.h"

AgvTask::AgvTask(QObject *parent) : QObject(parent),
    m_id(0),
    m_excuteCar(0),
    m_status(0),
    nodeAmount(0)
{

}

bool AgvTask::isDone()
{
    for(int i=0;i<taskNodes.length();++i){
        if(taskNodes.at(i)->status !=2){
            return false;
        }
    }
    return true;
}

TaskNode* AgvTask::nextTodoNode()
{
    for(int i=0;i<taskNodes.length();++i){
        if(taskNodes.at(i)->status == 0){
            return taskNodes.at(i);
        }
    }
    return NULL;
}
TaskNode *AgvTask::currentDoingNode()
{
    for(int i=0;i<taskNodes.length();++i){
        if(taskNodes.at(i)->status == 1){
            return taskNodes.at(i);
        }
    }
    return NULL;
}
