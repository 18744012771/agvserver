#include "msgcenter.h"

#include "util/global.h"
#include "util/common.h"
#include "usermsgprocessor.h"
#include "agvpositionpublisher.h"


MsgCenter::MsgCenter(QObject *parent) : QObject(parent),
    positionPublisher(NULL),
    statusPublisher(NULL),
    taskPublisher(NULL),
    fileUploadServer(NULL)
{

}

void MsgCenter::init()
{
    //启动8个线程，同时处理来自client的消息。
    for(int i=0;i<8;++i){
        UserMsgProcessor *workerThread = new UserMsgProcessor(this);
        workerThread->start();
    }
    //启动订阅 小车状态信息的线程
    if(statusPublisher){
        delete statusPublisher;
        statusPublisher=NULL;
    }
    statusPublisher = new AgvStatusPublisher(this);
    statusPublisher->start();

    //启动订阅 小车位置信息的线程
    if(positionPublisher){
        delete positionPublisher;
        positionPublisher=NULL;
    }
    positionPublisher = new AgvPositionPublisher(this);
    positionPublisher->start();

    if(taskPublisher){
        delete taskPublisher;
        taskPublisher = NULL;
    }
    taskPublisher = new AgvTaskPublisher(this);
    taskPublisher->start();

    //启动文件上传服务器
    fileUploadServer = new FileTransferServer;
    fileUploadServer->init();
    connect(fileUploadServer,SIGNAL(onUploadFinish(std::string,int,QString,QByteArray)),this,SLOT(onUploadFinish(std::string,int,QString,QByteArray)));
}

MsgCenter::~MsgCenter()
{
    if(positionPublisher)delete positionPublisher;
    if(statusPublisher)delete statusPublisher;
}

bool MsgCenter::addAgvPostionSubscribe(int subscribe)
{
    if(!positionPublisher)return false;
    positionPublisher->addSubscribe(subscribe);
    return true;
}

bool MsgCenter::removeAgvPositionSubscribe(int subscribe)
{
    if(!positionPublisher)return false;
    positionPublisher->removeSubscribe(subscribe);
    return true;
}

bool MsgCenter::addAgvStatusSubscribe(int subscribe)
{
    if(!statusPublisher)return false;
    statusPublisher->addSubscribe(subscribe);
    return true;
}

bool MsgCenter::removeAgvStatusSubscribe(int subscribe)
{
    if(!statusPublisher)return false;
    statusPublisher->removeSubscribe(subscribe);
    return true;
}

bool MsgCenter::addAgvTaskSubscribe(int subscribe)
{
    if(!taskPublisher)return false;
    taskPublisher->addSubscribe(subscribe);
    return true;
}

bool MsgCenter::removeAgvTaskSubscribe(int subscribe)
{
    if(!taskPublisher)return false;
    taskPublisher->removeSubscribe(subscribe);
    return true;
}

void MsgCenter::uploadFile(std::string _ip,int _port,QString _filename,int _length)
{
    //上传文件
    fileUploadServer->startUpload(_ip,_port,_filename,_length);
}

QString MsgCenter::downloadFile(std::string _ip, int _port, int &_length)
{
    fileUploadServer->startDonwload(_ip,_port,_length);
    return fileUploadServer->getFileName();
}

void MsgCenter::onUploadFinish(std::string _ip,int _port,QString _filename,QByteArray _data)
{
    //do nothing
}
