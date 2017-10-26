#include "agvnetwork.h"

#include "util/global.h"

AgvNetWork::AgvNetWork(QObject *parent) : QObject(parent),isQuit(false)
{

}

AgvNetWork::~AgvNetWork()
{
    isQuit = true;
    m_agvIOCP.Stop();
    m_clientIOCP.Stop();
}

void AgvNetWork::sendToOne(int id,char *buf,int len)//发送给某个id的消息
{
    try{
        m_agvIOCP.doSend(agv_id_and_sockets.at(id),buf,len);
    }catch(const std::out_of_range& oor){

    }
    try{
        m_clientIOCP.doSend(client_id_and_sockets.at(id),buf,len);
    }catch(const std::out_of_range& oor){

    }
}

void AgvNetWork::sendToAll(char *buf,int len)//发送给所有的人的信息
{
    m_agvIOCP._DoSend(buf,len);
    m_clientIOCP._DoSend(buf,len);
}

void AgvNetWork::sendToSome(QList<int> ones,char *buf,int len)//发送给某些id的人的消息
{
    for(QList<int>::iterator itr = ones.begin();itr!=ones.end();++itr){
        int id = *itr;
        sendToOne(id,buf,len);
    }
}

void AgvNetWork::sendToAllAgv(char *buf,int len)
{
    m_agvIOCP._DoSend(buf,len);
}

void AgvNetWork::sendToAllClient(char *buf,int len)
{
    m_clientIOCP._DoSend(buf,len);
}

void AgvNetWork::onRecvAgvMsg(void *param,char *buf, int len)
{
    //TODO:
    if (param != NULL) {
        ((AgvNetWork *)param)->recvAgvMsgProcess(buf, len);
    }
}


void AgvNetWork::onRecvClientMsg(void *param,char *buf, int len)
{
    //TODO:
    if (param != NULL)
    {
        ((AgvNetWork *)param)->recvClientMsgProcess(buf, len);
    }
}

//定义登录消息
void AgvNetWork::recvAgvMsgProcess(char *buf, int len)
{
    qDebug() << "recv from agv length="<<len<<" and buf="<<buf;
    //sendToAll(buf,len);

    ///这里要更新小车的信息到agvcenter还有任务信息到taskcenter




}


void AgvNetWork::recvClientMsgProcess(char *buf, int len)
{
    qDebug() << "recv from client length="<<len<<" and buf="<<buf;
    sendToAll(buf,len);
    //定义所有的消息，以 0x88 0x77 0x66 开头  以0x11 0x22 0x33结尾
    if(len<=6)return ;

    if(buf[0]==0x88 && buf[1]==0x77 && buf[3]==0x66 && buf[len-3]==0x11 && buf[len-2]==0x22 && buf[len-1]==0x33){
        //接下来是内容
        //第一个字节是type
        switch (buf[3]) {
        case CLIENT_MSG_TYPE_LOGIN:

            break;
        case CLIENT_MSG_TYPE_INFO:

            break;
        case CLIENT_MSG_TYPE_WARNING:

            break;
        case CLIENT_MSG_TYPE_ERROR:

            break;
        case CLIENT_MSG_TYPE_REAL_TIME:

            break;
        default:
            break;
        }
    }
}


void AgvNetWork::serverSendFunc(void *param)
{
    AgvNetWork *pThis = (AgvNetWork*)param;
    pThis->serverSendProcess();
}
void AgvNetWork::serverSendProcess()
{
    while (!isQuit) {
//        QyhDataItem item;
//        if (interfaceSendQueue.try_dequeue(item))
//        {
//            //TODO:
//            if (item.size > 0 ){
//                m_IOCP._DoSend((char *)item.data, item.size);
//                qyhLog << "send client===>" << (char *)item.data<<endll;
//            }
//            free(item.data);
//        }
//        else {
//            qyhSleepMs(300);
//        }
    }
}

bool AgvNetWork::initServer()
{
    if(!initAgvServer())return false;
    return initClientServer();
}

bool AgvNetWork::initAgvServer()
{
    if (false == m_agvIOCP.LoadSocketLib())
    {
        qyhLog<<"加载Winsock 2.2失败，服务器端无法运行！"<<endll;
    }
    m_agvIOCP.SetPort(8989);
    m_agvIOCP.setOwner(this);
    if (false == m_agvIOCP.Start(onRecvAgvMsg))
    {
        qyhLog << "Agv iocp服务器启动失败！" << endll;
        return false;
    }

    return true;
}

bool AgvNetWork::initClientServer()
{
    if (false == m_clientIOCP.LoadSocketLib())
    {
        qyhLog<<"加载Winsock 2.2失败，服务器端无法运行！"<<endll;
    }
    m_clientIOCP.SetPort(8988);
    m_clientIOCP.setOwner(this);
    if (false == m_clientIOCP.Start(onRecvClientMsg))
    {
        qyhLog << "Agv iocp服务器启动失败！" << endll;
        return false;
    }

    //创建消息发送线程
    std::thread(serverSendFunc, this).detach();
    qyhLog << "server start ok" << endll;
    return true;
}

