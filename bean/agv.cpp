#include "agv.h"
#include "util/common.h"
#include "util/global.h"

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
}

void Agv::onCheckOrder()
{
    if(orders.length()<=0)return ;

    if(recvQueueNumber!=sendQueueNumber){
        //TODO:
        //从头开始发送
        ordersIndex = 0;
        return;
    }

    //如果刚开始,小车并没有要执行的事情
    if(orderCount == 0)
    {
        //下发指令
        sendOrder();
    }
}

void Agv::onAgvRead()
{
    static QByteArray buff;
    QByteArray qba =  tcpClient->readAll();
    qDebug()<<"recv:"<<qba.length();
    return ;
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
        if(recvQueueNumber == sendQueueNumber)
        {
            if(orderCount>=1){
                sendOrder();
            }
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

void Agv::doStop()
{
    orderTimer.stop();
    orders.clear();
    ordersIndex = 0;
    currentPath.clear();
}

//前提是path已经赋值
void Agv::doPick()
{
    //生成pick的序列给agv.
    orders.clear();
    ordersIndex = 0;
    if(currentPath.length()<=0)return ;

    //获取当前任务
    Task *task = g_taskCenter->queryDoingTask(currentTaskId);
    if(task==NULL)return;
    if(task->getGoodStation<=0)return;

    ///////////////////0.放下叉子
    AgvOrder aor0;
    aor0.rfid = AgvOrder::RFID_CODE_EMPTY;
    aor0.order = AgvOrder::ORDER_UP_DOWN;
    aor0.param = 0;
    orders.push_back(aor0);

    ///////////////////1.找到之前的线路
    AgvLine *lastLine = NULL;

    for(QMap<int,AgvLine *>::iterator itr = g_m_lines.begin();itr!=g_m_lines.end();++itr){
        if(nowStation!=0){
            if(itr.value()->endStation==nowStation && itr.value()->startStation == lastStation){
                lastLine = itr.value();
            }
        }else{
            if(itr.value()->endStation==nextStation && itr.value()->startStation == lastStation){
                lastLine = itr.value();
            }
        }
    }

    ///////////////////2.计算到达终点之前的线路的左中右信息，放入orders中
    for(int i=0;i<currentPath.length()-1;++i)
    {
        AgvLine *line = g_m_lines[currentPath.at(i)];
        AgvStation *station = g_m_stations[line->endStation];
        AgvOrder aor2;
        aor2.rfid = station->rfid;
        aor2.order = AgvOrder::ORDER_FORWARD_STRIPE;
        if(lastLine!=NULL)
        {
            PATH_LEFT_MIDDLE_RIGHT p;
            p.lastLine = lastLine->id;
            p.nextLine = line->id;
            if(g_m_lmr.contains(p))
            {
                if(g_m_lmr[p] == PATH_LMR_LEFT){
                    aor2.order = AgvOrder::ORDER_TURN_LEFT;
                }else if(g_m_lmr[p] == PATH_LMR_RIGHT){
                    aor2.order = AgvOrder::ORDER_TURN_RIGHT;
                }
            }
        }

        if(aor2.order == AgvOrder::ORDER_FORWARD_STRIPE){
            aor2.param=200;
        }else{
            aor2.param=0;
        }
        orders.push_back(aor2);
        lastLine = line;
    }

    ///////////////////2.提升到取货高度
    AgvOrder aor22;
    aor22.rfid =  AgvOrder::RFID_CODE_EMPTY;
    aor22.order = AgvOrder::ORDER_UP_DOWN;
    aor22.param = task->getGoodHeight;
    orders.push_back(aor22);


    ///////////////////3.达到终点，调整方向前进
    AgvLine *line = g_m_lines[currentPath.length()-1];
    AgvStation *station = g_m_stations[line->endStation];
    if(task->getGoodDirect = Task::GET_PUT_DIRECT_FORWARD){
        AgvOrder aor3;
        aor3.rfid = station->rfid;
        aor3.order = AgvOrder::ORDER_FORWARD;
        aor3.param = task->getGoodDistance;
        orders.push_back(aor3);
    }else if(task->getGoodDirect = Task::GET_PUT_DIRECT_LEFT){

        AgvOrder aor31;
        aor31.rfid = station->rfid;
        aor31.order = AgvOrder::ORDER_TURN_LEFT;
        aor31.param =90;
        orders.push_back(aor31);

        AgvOrder aor32;
        aor32.rfid = AgvOrder::RFID_CODE_EMPTY;
        aor32.order = AgvOrder::ORDER_FORWARD;
        aor32.param =task->getGoodDistance;
        orders.push_back(aor32);
    }else if(task->getGoodDirect = Task::GET_PUT_DIRECT_RIGHT){
        AgvOrder aor31;
        aor31.rfid = station->rfid;
        aor31.order = AgvOrder::ORDER_TURN_RIGHT;
        aor31.param =90;
        orders.push_back(aor31);

        AgvOrder aor32;
        aor32.rfid = AgvOrder::RFID_CODE_EMPTY;
        aor32.order = AgvOrder::ORDER_FORWARD;
        aor32.param = task->getGoodDistance;
        orders.push_back(aor32);
    }

    ///////////////////4.提升
    AgvOrder aor4;
    aor4.rfid =  AgvOrder::RFID_CODE_EMPTY;
    aor4.order = AgvOrder::ORDER_UP_DOWN;
    aor4.param = task->getGoodHeight+PICK_PUT_HEIGHT;
    orders.push_back(aor4);

    ///////////////////5.后退回来
    AgvOrder aor5;
    aor5.rfid = station->rfid;
    aor5.order = AgvOrder::ORDER_BACKWARD;
    aor5.param =task->getGoodDistance;
    orders.push_back(aor5);

    //////////////////6.转回来
    if(task->getGoodDirect = Task::GET_PUT_DIRECT_LEFT){
        AgvOrder aor6;
        aor6.rfid = station->rfid;
        aor6.order = AgvOrder::ORDER_TURN_RIGHT;
        aor6.param =0;
        orders.push_back(aor6);
    }else if(task->getGoodDirect = Task::GET_PUT_DIRECT_RIGHT){
        AgvOrder aor6;
        aor6.rfid = station->rfid;
        aor6.order = AgvOrder::ORDER_TURN_LEFT;
        aor6.param =0;
        orders.push_back(aor6);
    }

    //////////////////7.放下叉子
    AgvOrder aor7;
    aor7.rfid = AgvOrder::RFID_CODE_EMPTY;
    aor7.order = AgvOrder::ORDER_UP_DOWN;
    aor7.param = PICK_PUT_HEIGHT;
    orders.push_back(aor7);

    agvDoing = AGV_DOING_PICKING;
    orderTimer.start();
}


//前提是path已经赋值
void Agv::doPut()
{
    //生成put的序列给agv
    orders.clear();
    if(currentPath.length()<=0)return ;

    //获取当前任务
    Task *task = g_taskCenter->queryDoingTask(currentTaskId);
    if(task==NULL)return;
    if(task->putGoodStation<=0)return;

    ///////////////////1.找到之前的线路
    AgvLine *lastLine = NULL;

    for(QMap<int,AgvLine *>::iterator itr = g_m_lines.begin();itr!=g_m_lines.end();++itr){
        if(nowStation!=0){
            if(itr.value()->endStation==nowStation && itr.value()->startStation == lastStation){
                lastLine = itr.value();
            }
        }else{
            if(itr.value()->endStation==nextStation && itr.value()->startStation == lastStation){
                lastLine = itr.value();
            }
        }
    }

    ///////////////////2.计算到达终点之前的线路的左中右信息，放入orders中
    for(int i=0;i<currentPath.length()-1;++i)
    {
        AgvLine *line = g_m_lines[currentPath.at(i)];
        AgvStation *station = g_m_stations[line->endStation];
        AgvOrder aor2;
        aor2.rfid = station->rfid;
        aor2.order = AgvOrder::ORDER_FORWARD_STRIPE;
        if(lastLine!=NULL)
        {
            PATH_LEFT_MIDDLE_RIGHT p;
            p.lastLine = lastLine->id;
            p.nextLine = line->id;
            if(g_m_lmr.contains(p))
            {
                if(g_m_lmr[p] == PATH_LMR_LEFT){
                    aor2.order = AgvOrder::ORDER_TURN_LEFT;
                }else if(g_m_lmr[p] == PATH_LMR_RIGHT){
                    aor2.order = AgvOrder::ORDER_TURN_RIGHT;
                }
            }
        }

        if(aor2.order == AgvOrder::ORDER_FORWARD_STRIPE){
            aor2.param=200;
        }else{
            aor2.param=0;
        }
        orders.push_back(aor2);
        lastLine = line;
    }

    ///////////////////2.提升到放货高度
    AgvOrder aor22;
    aor22.rfid =  AgvOrder::RFID_CODE_EMPTY;
    aor22.order = AgvOrder::ORDER_UP_DOWN;
    aor22.param = task->putGoodHeight+PICK_PUT_HEIGHT;
    orders.push_back(aor22);


    ///////////////////3.达到终点，调整方向前进
    AgvLine *line = g_m_lines[currentPath.length()-1];
    AgvStation *station = g_m_stations[line->endStation];
    if(task->putGoodDirect = Task::GET_PUT_DIRECT_FORWARD){
        AgvOrder aor3;
        aor3.rfid = station->rfid;
        aor3.order = AgvOrder::ORDER_FORWARD;
        aor3.param =task->putGoodDistance;
        orders.push_back(aor3);
    }else if(task->putGoodDirect = Task::GET_PUT_DIRECT_LEFT){
        AgvOrder aor31;
        aor31.rfid = station->rfid;
        aor31.order = AgvOrder::ORDER_TURN_LEFT;
        aor31.param =90;
        orders.push_back(aor31);

        AgvOrder aor32;
        aor32.rfid = AgvOrder::RFID_CODE_EMPTY;
        aor32.order = AgvOrder::ORDER_FORWARD;
        aor32.param =task->putGoodDistance;
        orders.push_back(aor32);
    }else if(task->putGoodDirect = Task::GET_PUT_DIRECT_RIGHT){
        AgvOrder aor31;
        aor31.rfid = station->rfid;
        aor31.order = AgvOrder::ORDER_TURN_RIGHT;
        aor31.param =90;
        orders.push_back(aor31);

        AgvOrder aor32;
        aor32.rfid = AgvOrder::RFID_CODE_EMPTY;
        aor32.order = AgvOrder::ORDER_FORWARD;
        aor32.param =task->putGoodDistance+PICK_PUT_HEIGHT;
        orders.push_back(aor32);
    }

    ///////////////////4.下降
    AgvOrder aor4;
    aor4.rfid =  AgvOrder::RFID_CODE_EMPTY;
    aor4.order = AgvOrder::ORDER_UP_DOWN;
    aor4.param = task->putGoodHeight;;
    orders.push_back(aor4);

    ///////////////////5.后退回来
    AgvOrder aor5;
    aor5.rfid = station->rfid;
    aor5.order = AgvOrder::ORDER_BACKWARD;
    aor5.param =task->putGoodDistance;
    orders.push_back(aor5);

    //////////////////6.转回来
    if(task->putGoodDirect = Task::GET_PUT_DIRECT_LEFT){
        AgvOrder aor6;
        aor6.rfid = station->rfid;
        aor6.order = AgvOrder::ORDER_TURN_RIGHT;
        aor6.param =0;
        orders.push_back(aor6);
    }else if(task->putGoodDirect = Task::GET_PUT_DIRECT_RIGHT){
        AgvOrder aor6;
        aor6.rfid = station->rfid;
        aor6.order = AgvOrder::ORDER_TURN_LEFT;
        aor6.param =0;
        orders.push_back(aor6);
    }

    //////////////////7.放下叉子
    AgvOrder aor7;
    aor7.rfid = AgvOrder::RFID_CODE_EMPTY;
    aor7.order = AgvOrder::ORDER_UP_DOWN;
    aor7.param =0;
    orders.push_back(aor7);

    agvDoing = AGV_DOING_PUTTING;
    orderTimer.start();
}

//前提是path已经赋值
void Agv::doStandBy()
{
    //生成standby的序列给agv
    //生成put的序列给agv
    orders.clear();
    ordersIndex = 0;
    if(currentPath.length()<=0)return ;

    //获取当前任务
    Task *task = g_taskCenter->queryDoingTask(currentTaskId);
    if(task==NULL)return;
    if(task->standByStation<=0)return;

    ///////////////////1.找到之前的线路
    AgvLine *lastLine = NULL;

    for(QMap<int,AgvLine *>::iterator itr = g_m_lines.begin();itr!=g_m_lines.end();++itr){
        if(nowStation!=0){
            if(itr.value()->endStation==nowStation && itr.value()->startStation == lastStation){
                lastLine = itr.value();
            }
        }else{
            if(itr.value()->endStation==nextStation && itr.value()->startStation == lastStation){
                lastLine = itr.value();
            }
        }
    }

    ///////////////////2.计算到达终点之前的线路的左中右信息，放入orders中
    for(int i=0;i<currentPath.length()-1;++i)
    {
        AgvLine *line = g_m_lines[currentPath.at(i)];
        AgvStation *station = g_m_stations[line->endStation];
        AgvOrder aor2;
        aor2.rfid = station->rfid;
        aor2.order = AgvOrder::ORDER_FORWARD_STRIPE;
        if(lastLine!=NULL)
        {
            PATH_LEFT_MIDDLE_RIGHT p;
            p.lastLine = lastLine->id;
            p.nextLine = line->id;
            if(g_m_lmr.contains(p))
            {
                if(g_m_lmr[p] == PATH_LMR_LEFT){
                    aor2.order = AgvOrder::ORDER_TURN_LEFT;
                }else if(g_m_lmr[p] == PATH_LMR_RIGHT){
                    aor2.order = AgvOrder::ORDER_TURN_RIGHT;
                }
            }
        }

        if(aor2.order == AgvOrder::ORDER_FORWARD_STRIPE){
            aor2.param=200;
        }else{
            aor2.param=0;
        }
        orders.push_back(aor2);
        lastLine = line;
    }

    agvDoing = AGV_DOING_STANDING;
    orderTimer.start();
}

void Agv::sendOrder()
{
    if(ordersIndex>0){
        if(lastSendOrderAmount==0){
            //完成了
            if(agvDoing == AGV_DOING_PICKING){
                emit pickFinish();
            }else if(agvDoing == AGV_DOING_PUTTING)
            {
                emit putFinish();
            }else if(agvDoing == AGV_DOING_STANDING){
                emit standByFinish();
                orderTimer.stop();
            }
        }else{
            if(lastSendOrderAmount>orderCount)
                ordersIndex -= (lastSendOrderAmount-orderCount-1);
        }
    }

    if(ordersIndex>=orders.length())return ;
    //根据orders封装一个发送的命令
    QByteArray content;

    //工作方式
    content.append(AGV_PACK_SEND_CODE_DISPATCH_MODE);

    //命令编号
    content.append(++sendQueueNumber);


    lastSendOrderAmount = 0;

    for(int i=0;i<3;++i)
    {
        if(i+ordersIndex>=orders.length()){
            //放入一个空
            content.append((char)0x00);
            content.append((char)0x00);
            content.append((char)0x00);
            content.append((char)0x00);
            content.append((char)0x00);
            content.append((char)0x00);
        }else{
            AgvOrder order = orders.at(i+ordersIndex);
            content.append((order.rfid>>24)&0xFF);
            content.append((order.rfid>>16)&0xFF);
            content.append((order.rfid>>8)&0xFF);
            content.append((order.rfid)&0xFF);
            content.append(order.order&0xFF);
            content.append(order.param&0xFF);
            lastSendOrderAmount+=1;
        }
    }

    content.append((char)0x00);
    content.append((char)0x00);
    content.append((char)0x00);
    content.append((char)0x00);
    content.append((char)0x00);
    content.append((char)0x00);

    ordersIndex+=lastSendOrderAmount;
    QByteArray qba = getSendPacket(content);
    send(qba.data(),qba.length());
}

//将内容封包
//加入包头、(功能码)、包长、(内容)、校验和、包尾
QByteArray Agv::getSendPacket(QByteArray content)
{
    //组包//加入包头、功能码、内容、校验和、包尾
    QByteArray result;

    //包头
    result.append(AGV_PACK_HEAD);

    //包长
    int len = 1/*包长*/+content.length()/*内容长*/+1/*校验码*/+1/*包尾*/;
    result.append(len&0xFF);

    //内容
    result.append(content);

    //校验和
    unsigned char sum = checkSum((unsigned char *)content.data(),content.length());
    result.append(sum);

    //包尾
    result.append(AGV_PACK_END);

    return result;
}


