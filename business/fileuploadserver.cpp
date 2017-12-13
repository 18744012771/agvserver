#include "fileuploadserver.h"
#include "util/global.h"

FileTransferServer::FileTransferServer(QObject *parent) : QObject(parent),server(NULL),fileName("")
{
}

void FileTransferServer::init()
{
    fileUploaders.clear();
    QyhTcp::QyhServerConnectCallback _connectcallback = std::bind(&FileTransferServer::onConnect, this, std::placeholders::_1);
    QyhTcp::QyhServerDisconnectCallback _disconnectcallback = std::bind(&FileTransferServer::onDisconnect, this, std::placeholders::_1);
    QyhTcp::QyhServerReadCallback _readcallback = std::bind(&FileTransferServer::onRead, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);
    if(server)
    {
        delete server;
    }

    //从数据库载入ID最大的文件的 数据
    QString querySql = "select a.bkg_name,a.bkg_data from agv_bkg a where a.id in(select max(b.id) from agv_bkg b where b.id=a.id);";
    QList<QVariant> params;
    QList<QList<QVariant> > result = g_sql->query(querySql,params);
    if(result.length()>0){
        fileName = result.at(0).at(0).toString();
        fileData = result.at(0).at(1).toByteArray();
    }

    server =QyhTcp::QyhTcpServer::create(2121,_readcallback,_connectcallback,_disconnectcallback);
}

//有客户端连接的回调函数
void FileTransferServer::onConnect(QyhTcp::CLIENT_NODE node)
{
    clientsMutex.lock();
    clients.push_back(node);
    clientsMutex.unlock();
}

//有客户端断开连接的回调函数
void FileTransferServer::onDisconnect(QyhTcp::CLIENT_NODE node)
{
    clientsMutex.lock();
    clients.removeAll(node);
    clientsMutex.unlock();
    fileUploadersMutex.lock();
    for(QList<FILE_UPLOAD_NODE>::iterator itr = fileUploaders.begin();itr!=fileUploaders.end();++itr)
    {
        if(itr->client_ip == node.ip && itr->client_port == node.port)
        {
            //未上传完成就断开连接的，认为放弃上传了
            if(itr->data.length()!=itr->length){
                fileUploaders.erase(itr);
            }
            break;
        }
    }
    fileUploadersMutex.unlock();
}

void FileTransferServer::onRead(const char *data,int len,QyhTcp::CLIENT_NODE node)
{
    fileUploadersMutex.lock();
    for(QList<FILE_UPLOAD_NODE>::iterator itr = fileUploaders.begin();itr!=fileUploaders.end();++itr)
    {
        if(itr->client_ip == node.ip && itr->client_port == node.port)
        {
            //写入数据
            itr->data.append(data,len);
            if(itr->data.length()>=itr->length)
            {
                //接收数据完成
                emit onUploadFinish(itr->client_ip,itr->client_port,itr->name,itr->data);

                QString insertSql = "insert into agv_bkg(bkg_name,bkg_data,bkg_upload_time) values(?,?,?)";
                QList<QVariant> params;
                params<<itr->name<<itr->data<<QDateTime::currentDateTime();
                if(g_sql->exeSql(insertSql,params)){
                    //执行保存数据库
                    fileName = itr->name;
                    fileData = itr->data;
                }

                //完成后，去除该节点
                fileUploaders.erase(itr);
                break;
            }
        }
    }
    fileUploadersMutex.unlock();
}

//开始上传
void FileTransferServer::startUpload(std::string _ip, int _port, QString _filename, int length)
{
    FILE_UPLOAD_NODE node;
    node.client_ip = _ip;
    node.client_port = _port;
    node.name = _filename;
    node.length = length;
    fileUploadersMutex.lock();
    fileUploaders.insert(_port,node);
    fileUploadersMutex.unlock();
}

//停止上传
void FileTransferServer::stopUpload(std::string _ip, int _port)
{
    fileUploadersMutex.lock();
    for(QList<FILE_UPLOAD_NODE>::iterator itr = fileUploaders.begin();itr!=fileUploaders.end();++itr)
    {
        if(itr->client_ip == _ip && itr->client_port == _port)
        {
            fileUploaders.erase(itr);
            break;
        }
    }
    fileUploadersMutex.unlock();
}

//开始下载
void FileTransferServer::startDonwload(std::string _ip, int port, int &_length)
{
    _length = fileData.length();
    if(fileName.length()>0 && fileData.length()>0){
        QyhTcp::CLIENT_NODE n;
        clientsMutex.lock();
        for(QList<QyhTcp::CLIENT_NODE>::iterator itr=clients.begin();itr!=clients.end();++itr)
        {
            if(itr->ip == _ip && itr->port == port){
                n = (*itr);
                break;
            }
        }
        clientsMutex.unlock();

        //这个发送过程会造成消息阻塞
        if(n.socket>0){
            server->sendToOne(fileData.data(),fileData.length(),n);
        }
    }
}

//停止下载
void FileTransferServer::stopDownload(std::string _ip,int port)
{
    //因为一次性发送的，似乎无法停止呀。。。。
}
