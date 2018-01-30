#include "agvconnection.h"

AgvConnection::AgvConnection():tcpClient(NULL)
{

}

AgvConnection::~AgvConnection()
{
    if(tcpClient)
    {
        delete tcpClient;
        tcpClient = NULL;
    }
}

void AgvConnection::init(QString ip,int port,QyhTcp::QyhClientReadCallback r,QyhTcp::QyhClientConnectCallback c,QyhTcp::QyhClientDisconnectCallback d)
{
    if(tcpClient)
    {
        delete tcpClient;
        tcpClient = NULL;
    }
    tcpClient = QyhTcp::QyhTcpClient::create(ip.toStdString().c_str(),port,r,c,d);
}

bool AgvConnection::send(const char *data,int len)
{
    if(tcpClient)return tcpClient->sendToServer(data,len);
    return false;
}
