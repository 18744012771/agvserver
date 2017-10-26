#ifndef AGVNETWORK_H
#define AGVNETWORK_H

#include <QObject>
#include <map>
using std::map;
#include "IOCPModel.h"

enum agv_msg_type{
    AGV_MSG_TYPE_LOGIN = 0,//登陆
    AGV_MSG_TYPE_INFO,//信息
    AGV_MSG_TYPE_WARNING,//警告
    AGV_MSG_TYPE_ERROR,//错误
    AGV_MSG_TYPE_REAL_TIME,//实时的消息，要求立即处理
};

enum client_msg_type{
    CLIENT_MSG_TYPE_LOGIN = 0,//登陆
    CLIENT_MSG_TYPE_INFO,//信息
    CLIENT_MSG_TYPE_WARNING,//警告
    CLIENT_MSG_TYPE_ERROR,//错误
    CLIENT_MSG_TYPE_REAL_TIME,//实时的消息，要求立即处理
};

class AgvNetWork : public QObject
{
    Q_OBJECT
public:
    explicit AgvNetWork(QObject *parent = nullptr);
    ~AgvNetWork();

    void sendToOne(int id,char *buf,int len);//发送给某个id的消息
    void sendToAll(char *buf,int len);//发送给所有的人的信息
    void sendToSome(QList<int> ones,char *buf,int len);//发送给某些id的人的消息
    void sendToAllAgv(char *buf,int len);
    void sendToAllClient(char *buf,int len);

    //读取的回调函数
    static void onRecvAgvMsg(void *param,char *buf, int len);
    static void onRecvClientMsg(void *param,char *buf, int len);

    void recvAgvMsgProcess(char *buf, int len);
    void recvClientMsgProcess(char *buf, int len);

    //发送消息线程
    static void serverSendFunc(void *param);
    void serverSendProcess();

    bool initServer();
signals:

public slots:

private:
    CIOCPModel m_agvIOCP;
    CIOCPModel m_clientIOCP;
    volatile bool isQuit;

    //初始化
    bool initAgvServer();
    bool initClientServer();
    map<int,int> agv_id_and_sockets;
    map<int,int> client_id_and_sockets;
};

#endif // AGVNETWORK_H
