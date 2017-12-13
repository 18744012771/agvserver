#ifndef FILEUPLOADSERVER_H
#define FILEUPLOADSERVER_H

#include <QObject>
#include <QFile>
#include <QMap>
#include <QMutex>
#include <qyhtcpserver.h>

//用于传输背景图片【将来再优化封装，现在先进行简单的实现】
class FileTransferServer : public QObject
{
    Q_OBJECT
public:

    struct FILE_UPLOAD_NODE
    {
        FILE_UPLOAD_NODE():length(0)
        {}
        FILE_UPLOAD_NODE(const FILE_UPLOAD_NODE &b){
            client_ip=b.client_ip;
            client_port=b.client_port;
            data=b.data;
            name=b.name;
            length=b.length;
        }
        bool operator <(const FILE_UPLOAD_NODE &b){
            return client_ip<b.client_ip;
        }
        std::string client_ip;//连接的客户端的ip
        int client_port;//连接的客户端的端口
        QByteArray data;//接收的数据
        QString name;//文件名称
        int length;//文件大小
    };

    //文件上传的tcp server
    explicit FileTransferServer(QObject *parent = nullptr);

    //初始化tcp server
    void init();

    //有客户端连接的回调函数
    void onConnect(QyhTcp::CLIENT_NODE node);

    //有客户端断开连接的回调函数
     void onDisconnect(QyhTcp::CLIENT_NODE node);

    //读取数据的回调函数
    void onRead(const char *data, int len, QyhTcp::CLIENT_NODE node);

    //开始上传文件 [参数 ip/port是client端的IP和端口, filename文件名称, length文件长度]
    void startUpload(std::string _ip,int _port, QString _filename, int length);

    //停止上传
    void stopUpload(std::string _ip, int _port);

    //开始下载 [下载的文件名称什么的都不用，因为我们知道只有一个图片是给他们下载的]
    void startDonwload(std::string _ip, int port, int &_length);

    //停止下载
    void stopDownload(std::string _ip,int port);

    QString getFileName(){return fileName;}
signals:
    //文件上传完成
    void onUploadFinish(std::string _ip,int _port,QString _filename,QByteArray _data);

    //下载完成
    void onDownloadFinish(std::string _ip,int _port);
public slots:

private:
    QyhTcp::QyhTcpServer *server;
private:
    //连接的客户端
    QMutex clientsMutex;
    QList<QyhTcp::CLIENT_NODE> clients;

    QMutex fileUploadersMutex;
    QList<FILE_UPLOAD_NODE> fileUploaders;

    //文件的数据
    QByteArray fileData;
    //文件的名称
    QString fileName;
};

#endif // FILEUPLOADSERVER_H
