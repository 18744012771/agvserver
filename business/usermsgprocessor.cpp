#include "usermsgprocessor.h"
#include "util/global.h"

UserMsgProcessor::UserMsgProcessor(QObject *parent) : QThread(parent),isQuit(false)
{

}
UserMsgProcessor::~UserMsgProcessor(){
    isQuit = true;
}

void UserMsgProcessor::run()
{
    while(!isQuit){
        QyhDataItem item;
        if(g_user_msg_queue.try_dequeue(item)){
            if(item.data==NULL)continue;
            if(item.size<=0)
            {
                free(item.data);continue;
            }
            //处理数据
            QString ss = QString::fromLatin1((char *)item.data,item.size).trimmed();
            qDebug()<< "recv from user :"<<ss;
        }

        QyhSleep(50);
    }
}
void UserMsgProcessor::myquit()
{
    isQuit=true;
}
