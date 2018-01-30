#ifndef AGVCONNECTION_H
#define AGVCONNECTION_H

#include <qyhtcpclient.h>
#include <QString>

/*
 *和Agv的tcp连接
 *长连接
 *采用回调方式
 */

class AgvConnection
{
public:
    AgvConnection();
    ~AgvConnection();
    void init(QString ip,int port,QyhTcp::QyhClientReadCallback r,QyhTcp::QyhClientConnectCallback c,QyhTcp::QyhClientDisconnectCallback d);
    bool send(const char *data,int len);
private:
    QyhTcp::QyhTcpClient *tcpClient;
};

#endif // AGVCONNECTION_H
