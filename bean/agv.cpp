#include "agv.h"
#include "util/common.h"

Agv::Agv(QObject *parent) : QObject(parent),
    cmdQueue(NULL),
    connection(NULL)
{

}


Agv::~Agv()
{
    if(cmdQueue)delete cmdQueue;
    if(connection)delete connection;
}

//开始任务
void Agv::startTask(QList<AgvOrder>& ord)
{
    if(cmdQueue)
    {
        cmdQueue->setQueue(ord);
    }
}

//停止、取消任务
void Agv::stopTask()
{
    if(cmdQueue)
    {
        cmdQueue->clear();
    }
}


void Agv::onQueueFinish()
{
    if(taskFinish!=nullptr){
        taskFinish(this);
    }
}

void Agv::onSend(const char *data,int len)
{
    if(connection && connection->isWritable())
    {
        connection->write(data,len);
    }
}

bool Agv::init(QString _ip, int _port,TaskFinishCallback _taskFinish,TaskErrorCallback _taskError,TaskInteruptCallback _taskInteruput,UpdateMCallback _updateM,UpdateMRCallback _updateMR)
{
    ip = _ip;
    port = _port;
    taskFinish = _taskFinish;
    taskError = _taskError;
    taskInteruput = _taskInteruput;
    updateM = _updateM;
    updateMR = _updateMR;
    if(cmdQueue){
        delete cmdQueue;
        cmdQueue = NULL;
    }
    if(connection){
        delete connection;
        connection = NULL;
    }

    //创建连接
    connection = new QTcpSocket;
    connect(connection,SIGNAL(connected()),this,SLOT(onConnect()));
    connect(connection,SIGNAL(disconnected()),this,SLOT(onDisconnect()));
    connect(connection,SIGNAL(readyRead()),this,SLOT(onRecv()));
    connection->connectToHost(_ip,_port);

    //创建队列处理
    AgvCmdQueue::ToSendCallback s = std::bind(&Agv::onSend,this,std::placeholders::_1,std::placeholders::_2);
    AgvCmdQueue::FinishCallback f = std::bind(&Agv::onQueueFinish,this);
    cmdQueue = new AgvCmdQueue;
    cmdQueue->init(s,f);

    return true;
}

void  Agv::onRecv()
{
    QByteArray qba = connection->readAll();
    static QByteArray buff;
    buff+= qba;
    while(true)
    {
        int start = buff.indexOf(0x66);
        int end = buff.indexOf(0x88);
        if(start>=0&&end>=0){
            //这个是一个包:
            QByteArray onePack = buff.mid(start,end-start+1);
            if(onePack.length()>0){
                processOnePack(onePack);
            }
            //然后将之前的数据丢弃
            buff = buff.right(buff.length()-end-1);
        }else{
            break;
        }
    }
}

void Agv::processOnePack(QByteArray qba)
{
    int kk = (int)(qba.at(1));
    //qDebug()<<"qba.length=="<<qba.length();
    if(kk != qba.length()-1) return ;//除去包头
    //上报的命令
    char *str = qba.data();
    str+=2;//跳过包头和包长

    mileage = getInt32FromByte(str);
    str+=4;

    currentRfid = getInt32FromByte(str);
    //qDebug()<<"current rfid="<<currentRfid;
    str+=4;

    nextRfid = getInt32FromByte(str);
    str+=4;

    current = getInt16FromByte(str);
    str+=2;

    voltage = getInt16FromByte(str);
    str+=2;

    positionMagneticStripe =  getInt16FromByte(str);
    str+=2;

    pcbTemperature = getInt8FromByte(str);
    str+=1;

    motorTemperature = getInt8FromByte(str);
    str+=1;

    cpu = getInt8FromByte(str);
    str+=1;

    speed = getInt8FromByte(str);
    str+=1;

    angle = getInt8FromByte(str);
    str+=1;

    height = getInt8FromByte(str);
    str+=1;

    error_no = getInt8FromByte(str);
    str+=1;

    mode = getInt8FromByte(str);
    str+=1;

    recvQueueNumber = getInt8FromByte(str);
    str+=1;

    orderCount = getInt8FromByte(str);
    str+=1;


    CRC = getInt8FromByte(str);
    str+=1;

    //校验CRC
    str = qba.data();
    unsigned char c = checkSum((unsigned char *)(str+2),kk-4);
    if(c != CRC)
    {
        //TODO:
        //校验不合格
    }

    //更新小车状态
    if(mode == AGV_MODE_HAND){
        if(currentTaskId > 0)
        {

            //1.通知cmdqueue，取消任务
            cmdQueue->clear();

            //2.通知上面的，取消任务了
            if(taskInteruput!= nullptr){
                taskInteruput(this);
            }

            //3.任务置空? 交给上面完成吧

            //emit taskCancel(currentTaskId);
        }
        status = AGV_STATUS_HANDING;
    }

    //更新错误状态
    if(error_no !=0 && currentTaskId > 0)
    {
        //1.通知cmdqueue，取消任务
        cmdQueue->clear();
        //任务过程中发生错误
        if(taskError!=nullptr){
            taskError(error_no,this);
        }
    }

    //更新rfid       //更新里程计
    if(currentRfid!=lastRfid)
    {
        lastRfid = currentRfid;
        lastStationOdometer = mileage;

        if(updateMR!=nullptr){
            updateMR(currentRfid,mileage,this);
        }
    }else{
        if(updateM!=nullptr){
            updateM(mileage,this);
        }
    }

    if(cmdQueue){
        cmdQueue->onOrderQueueChanged(recvQueueNumber,orderCount);
    }


}

void  Agv::onConnect()
{
    //TODO:
}
void  Agv::onDisconnect()
{
    //TODO:
}
