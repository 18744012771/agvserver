#include "agv.h"
#include "util/common.h"

Agv::Agv(QObject *parent) : QObject(parent),tcpClient(NULL)
{

}

void Agv::init(QString _ip,int _port)
{
    ip=_ip;
    port=_port;

    if(tcpClient){
        delete tcpClient;
        tcpClient=NULL;
    }

    tcpClient = new QTcpSocket;

    connect(tcpClient,SIGNAL(connected()),this,SLOT(connectCallBack()));
    connect(tcpClient,SIGNAL(disconnected()),this,SLOT(disconnectCallBack()));
    connect(tcpClient,SIGNAL(readyRead()),this,SLOT(onAgvRead()));
    connect(tcpClient,SIGNAL(stateChanged(QAbstractSocket::SocketState)),this,SLOT(connectStateChanged(QAbstractSocket::SocketState)));
    tcpClient->connectToHost(ip,port);

    orderTimer.setInterval(200);
    connect(&orderTimer,SIGNAL(timeout()),this,SLOT(onCheckOrder()));
    orderTimer.start();
}

void Agv::onCheckOrder()
{
    if(orders.length()<=0)return ;

    //下发命令？
    //TODO:

}

void Agv::onAgvRead()
{
    static QByteArray buff;
    buff += tcpClient->readAll();
    //截取一条消息
    //然后对其进行拆包，把拆出来的包放入队列
    while(true){
        int start = buff.indexOf(0x66);
        int end = buff.indexOf(0x33);
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
    if(qba.startsWith(0x66) && qba.endsWith(0x33))
    {
        int kk = (int)(qba.at(1));
        if(kk != qba.length()-1) return ;//除去包头
        //上报的命令
        char *str = qba.data();
        str+=2;//跳过包头和包长

        mileage = getInt32FromByte(str);
        str+=4;

        currentRfid = getInt32FromByte(str);
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

        currentQueueNumber = getInt8FromByte(str);
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
            if(currentTaskId > 0){
                //任务被取消了!
                emit taskCancel(currentTaskId);
            }
            status = AGV_STATUS_HANDING;
        }

        //更新错误状态
        if(error_no !=0 && currentTaskId > 0)
        {
            //任务过程中发生错误
            //TODO:
            //1.更新队列

            //2.发射信号
            emit taskError(currentTaskId);
        }

        //更新rfid       //更新里程计
        if(currentRfid!=lastRfid)
        {
            lastRfid = currentRfid;
            lastStationOdometer = mileage;
            emit updateRfidAndOdometer(currentRfid,mileage);
        }else{
            emit updateOdometer(mileage);
        }

        //更新命令
        if(currentQueueNumber == queueNumber-1)
        {
            //判断当前序列的命令的第几个
        }

    }else{
        //可能是开机或者运行每小时上报的 网关
    }

}

void Agv::connectCallBack()
{
    if(status==AGV_STATUS_UNCONNECT)status = AGV_STATUS_IDLE;
    emit sigconnect();
}


void Agv::disconnectCallBack()
{
    status = AGV_STATUS_UNCONNECT;
    emit sigdisconnect();
}

void Agv::connectStateChanged(QAbstractSocket::SocketState s)
{

    if(s==QAbstractSocket::ClosingState ||s==QAbstractSocket::UnconnectedState )
    {
        tcpClient->connectToHost(ip,port);
    }
}

bool Agv::send(const char *data,int len)
{
    if(tcpClient && tcpClient->isWritable()){
        return len == tcpClient->write(data,len);
    }
    return false;
}
