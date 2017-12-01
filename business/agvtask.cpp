#include "agvtask.h"

AgvTask::AgvTask(QObject *parent) : QObject(parent),
    m_id(0),
    m_excuteCar(0),
    m_status(0),
    nodeAmount(0),
    nextTodoIndex(0),
    lastDoneIndex(-1),
    currentDoingIndex(-1)
{

}

bool AgvTask::isDone()
{
    return currentDoingIndex>=taskNodes.length();
//    for(int i=0;i<taskNodes.length();++i){
//        if(taskNodes[i]->status !=AGV_TASK_NODE_STATUS_DONE){
//            return false;
//        }
//    }
//    return true;
}

//int AgvTask::nextTodoNode()
//{
//    for(int i=0;i<taskNodes.length();++i){
//        if(taskNodes[i]->status == AGV_TASK_NODE_STATUS_UNDO){
//            return i;
//        }
//    }
//    return -1;
//}
//int AgvTask::currentDoingNode()
//{
//    for(int i=0;i<taskNodes.length();++i){
//        if(taskNodes[i]->status == AGV_TASK_NODE_STATUS_DOING){
//            return i;
//        }
//    }
//    return -1;
//}

//int AgvTask::lastDoneNode()
//{
//    for(int i=0;i<taskNodes.length();++i){
//        if(taskNodes[i]->status == AGV_TASK_NODE_STATUS_UNDO){
//            if(i>0)
//                return i-1;
//            else return -1;
//        }
//    }
//    return -1;
//}
