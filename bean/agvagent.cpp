#include "agvagent.h"
#include "util/common.h"

AgvAgent::AgvAgent():
    cmdQueue(NULL),
    connection(NULL)
{

}

AgvAgent::~AgvAgent()
{
    if(cmdQueue)delete cmdQueue;
    if(connection)delete connection;
}

//开始任务
void AgvAgent::startTask(QList<AgvOrder>& ord)
{
    if(cmdQueue)
    {
        cmdQueue->setQueue(ord);
    }
}

//停止、取消任务
void AgvAgent::stopTask()
{
    if(cmdQueue)
    {
        cmdQueue->clear();
    }
}


void AgvAgent::onQueueFinish()
{
    if(taskFinish!=nullptr){
        taskFinish(this);
    }
}

bool AgvAgent::init(QString _ip, int _port,TaskFinishCallback _taskFinish,TaskErrorCallback _taskError,TaskInteruptCallback _taskInteruput,UpdateMCallback _updateM,UpdateMRCallback _updateMR)
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
    connection = new AgvConnection;
    QyhTcp::QyhClientReadCallback r =  std::bind( &AgvAgent::onRecv, this, std::placeholders::_1,std::placeholders::_2);
    QyhTcp::QyhClientConnectCallback c =  std::bind( &AgvAgent::onConnect, this);
    QyhTcp::QyhClientDisconnectCallback d =  std::bind( &AgvAgent::onDisconnect, this);
    connection->init(_ip,_port,r,c,d);

    //创建队列处理
    AgvCmdQueue::ToSendCallback s = std::bind(&AgvConnection::send,connection,std::placeholders::_1,std::placeholders::_2);
    AgvCmdQueue::FinishCallback f = std::bind(&AgvAgent::onQueueFinish,this);
    cmdQueue = new AgvCmdQueue;
    cmdQueue->init(s,f);

    return true;
}

void  AgvAgent::onRecv(const char *data,int len)
{
    if(data==NULL||len<=0)return ;
    static QByteArray buff;
    buff+= QByteArray(data,len);
    while(true){
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

void AgvAgent::processOnePack(QByteArray qba)
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

void  AgvAgent::onConnect()
{
    //TODO:
}
void  AgvAgent::onDisconnect()
{
    //TODO:
}
