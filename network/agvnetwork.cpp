#include "agvnetwork.h"

#include "util/global.h"

AgvNetWork::AgvNetWork(QObject *parent) : QObject(parent),isQuit(false)
{

}

AgvNetWork::~AgvNetWork()
{
    isQuit = true;
    //m_agvIOCP.Stop();
    m_clientIOCP.Stop();
}

void AgvNetWork::sendToOne(SOCKET sock,const char *buf, int len)//发送给某个id的消息
{
    //    try{
    //        m_agvIOCP.doSend(agv_id_and_sockets.at(id),buf,len);
    //    }catch(const std::out_of_range& oor){

    //    }
    try{
        m_clientIOCP.doSend(sock,buf,len);
    }catch(const std::out_of_range& oor){

    }
}

void AgvNetWork::sendToAll(char *buf,int len)//发送给所有的人的信息
{
    //m_agvIOCP._DoSend(buf,len);
    m_clientIOCP._DoSend(buf,len);
}

void AgvNetWork::sendToSome(QList<int> ones,char *buf,int len)//发送给某些id的人的消息
{
    for(QList<int>::iterator itr = ones.begin();itr!=ones.end();++itr){
        int id = *itr;
        sendToOne(id,buf,len);
    }
}

//void AgvNetWork::sendToAllAgv(char *buf,int len)
//{
//    //m_agvIOCP._DoSend(buf,len);
//}

//void AgvNetWork::sendToAllClient(char *buf,int len)
//{
//    m_clientIOCP._DoSend(buf,len);
//}

//void AgvNetWork::onRecvAgvMsg(void *param,char *buf, int len)
//{
//    //TODO:
//    if (param != NULL) {
//        ((AgvNetWork *)param)->recvAgvMsgProcess(buf, len);
//    }
//}


void AgvNetWork::onRecvClientMsg(void *param, char *buf, int len,SOCKET sock,const std::string &sIp,int port)
{
    //TODO:
    if (param != NULL)
    {
        ((AgvNetWork *)param)->recvClientMsgProcess(buf, len,sock,sIp,port);
    }
}

void AgvNetWork::onDisconnectClient(void *owner, SOCKET sock, const std::string &sIp, int port)
{
    //一个客户端掉线了
    //1.提出所有它订阅的东西
    g_msgCenter.removeAgvPositionSubscribe(sock);
    g_msgCenter.removeAgvStatusSubscribe(sock);
    //2.将在线状态修改
    int user_id = -1;
    for(std::list<LoginUserInfo>::iterator itr = loginUserIdSock.begin();itr!=loginUserIdSock.end();++itr){
        LoginUserInfo info = *itr;
        if(info.sock == sock){
            user_id = info.id;
            ///从已登录 移出
            loginUserIdSock.erase(itr);
            break;
        }
    }

    //设置在线状态
    if(user_id>0){
        QString updateSql = "update agv_user set signState = 0 where user_id=?";
        QStringList params;
        params<<QString("%1").arg(user_id);
        g_sql->exec(updateSql,params);
    }
}


void AgvNetWork::recvClientMsgProcess(char *buf, int len,SOCKET sock,const std::string &ip, int port)
{
    if(len<=0||buf==NULL)return ;

    //数据
    std::string ss(buf,len);

    //加入缓冲区
    if(client2serverBuffer.find(sock)==client2serverBuffer.end()){
        client2serverBuffer.insert(std::make_pair(sock,ss));
    }else{
        client2serverBuffer[sock] = client2serverBuffer[sock]+ss;
    }

    //然后对其进行拆包，把拆出来的包放入队列
    while(true){
        size_t start = client2serverBuffer[sock].find("<xml>");
        size_t end = client2serverBuffer[sock].find("</xml>",start);
        if(start!=std::string::npos&&end!=std::string::npos){
            //这个是一个包:
            std::string onPack = client2serverBuffer[sock].substr(start,end-start+6);
            if(onPack.length()>0){
                //进行入队
                QyhMsgDateItem item;
                item.data=malloc(onPack.length());
                memcpy(item.data,onPack.c_str(),onPack.length());
                item.ip = ip;
                item.port = port;
                item.size=onPack.length();
                item.sock=sock;
                g_user_msg_queue.enqueue(item);
            }
            //然后将之前的数据丢弃
            client2serverBuffer[sock] = client2serverBuffer[sock].substr(end+6);
        }else{
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
    //if(!initAgvServer())return false;
    return initClientServer();
}

//bool AgvNetWork::initAgvServer()
//{
//    if (false == m_agvIOCP.LoadSocketLib())
//    {
//        qyhLog<<"加载Winsock 2.2失败，服务器端无法运行！"<<endll;
//    }
//    m_agvIOCP.SetPort(8989);
//    m_agvIOCP.setOwner(this);
//    if (false == m_agvIOCP.Start(onRecvAgvMsg))
//    {
//        qyhLog << "Agv iocp服务器启动失败！" << endll;
//        return false;
//    }

//    return true;
//}

bool AgvNetWork::initClientServer()
{
    if (false == m_clientIOCP.LoadSocketLib())
    {
        qyhLog<<"加载Winsock 2.2失败，服务器端无法运行！"<<endll;
    }
    m_clientIOCP.SetPort(8988);
    m_clientIOCP.setOwner(this);
    if (false == m_clientIOCP.Start(onRecvClientMsg,onDisconnectClient))
    {
        qyhLog << "Agv iocp服务器启动失败！" << endll;
        return false;
    }

    //创建消息发送线程
    std::thread(serverSendFunc, this).detach();
    qyhLog << "server start ok" << endll;
    return true;
}

