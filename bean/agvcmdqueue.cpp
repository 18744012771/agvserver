#include "agvcmdqueue.h"
#include <QByteArray>
#include "util/common.h"

AgvCmdQueue::AgvCmdQueue():
    quit(false)
{

}

AgvCmdQueue::~AgvCmdQueue()
{
    quit = true;
    while(!threadAlreadQuit)
    {
        std::chrono::milliseconds dura(10);
        std::this_thread::sleep_for(dura);
    }
}

void AgvCmdQueue::cmdThread(void *param)
{
    ((AgvCmdQueue *)param)->cmdProcess();
}


void AgvCmdQueue::cmdProcess()
{
    int sendFailTimes;
    threadAlreadQuit = false;
    while(!quit)
    {
        //如果没有要发送的内容，等待
        mtx.lock();
        if(orders.length()<=0)
        {
            mtx.unlock();
            std::chrono::milliseconds dura(50);
            std::this_thread::sleep_for(dura);
            continue;
        }
        mtx.unlock();

        //如果两个序列值不同，表示上次的发送没有成功，隔400ms后重新发送
        if(recvQueueNumber!=sendQueueNumber)
        {
            //上一次的发送失败
            if(sendFailTimes<4)
            {
                std::chrono::milliseconds dura(100);
                std::this_thread::sleep_for(dura);
                ++sendFailTimes;
            }else{
                //上一次指令没有发送成功,重新发送
                if(ordersSendIndex - lastSendOrderAmount>=0)
                {
                    //对上一次的内容重新发送:
                    ordersSendIndex -= lastSendOrderAmount;
                    --sendQueueNumber;
                    sendOrder();
                    sendFailTimes = 0;
                }
            }
            continue;
        }else{
            sendFailTimes = 0;
        }

        //如果当前执行的序列>=1表示已经执行了1个或几个发过去的指令了，那么要填充新的指令给它
        if(orderExcuteIndex >= 1)
        {
            if(lastSendOrderAmount>=orderExcuteIndex)
            {
                ordersSendIndex -= (lastSendOrderAmount-orderExcuteIndex);
            }
            sendOrder();
        }
    }
    threadAlreadQuit = true;
}

void AgvCmdQueue::sendOrder()
{
    mtx.lock();
    if(orders.length() == 0 && ordersSendIndex == 0){
        //
    }else{
        if(ordersSendIndex>=orders.length()){
            orders.clear();
            mtx.unlock();
            //TODO:完成了队列的发送:
            if(orders.size()!=0 && finish!=nullptr)
            {
                finish();
            }
            return ;
        }
    }


    //根据orders封装一个发送的命令
    QByteArray content;

    //工作方式
    content.append(AGV_PACK_SEND_CODE_DISPATCH_MODE);

    //命令编号
    content.append(++sendQueueNumber);

    lastSendOrderAmount = 0;

    for(int i=0;i<3;++i)
    {
        if(i+ordersSendIndex>=orders.size()){
            //放入一个空
            content.append(getRfidByte(AgvOrder::RFID_CODE_IMMEDIATELY));
            content.append((char)(AgvOrder::ORDER_STOP));
            content.append((char)0x00);
        }else{
            AgvOrder order = orders[i+ordersSendIndex];
            content.append(getRfidByte(order.rfid));
            content.append(order.order&0xFF);
            content.append(order.param&0xFF);
            lastSendOrderAmount+=1;
        }
    }
    mtx.unlock();
    content.append(getRfidByte(AgvOrder::RFID_CODE_IMMEDIATELY));
    content.append((char)(AgvOrder::ORDER_STOP));
    content.append((char)0x00);

    ordersSendIndex+=lastSendOrderAmount;
    QByteArray qba = getSendPacket(content);

    if(toSend != nullptr)
    {
        toSend(qba.data(),qba.length());
    }
}

void AgvCmdQueue::init(ToSendCallback _toSend, FinishCallback _finish)
{
    toSend = _toSend;
    finish = _finish;
    //启动一个线程，创建socket并连接，然后发送和读取
    std::thread(cmdThread,this).detach();
}

void AgvCmdQueue::onOrderQueueChanged(int queueNumber,int orderQueueNumber)
{
    recvQueueNumber = queueNumber;
    orderExcuteIndex = orderQueueNumber;
}


void AgvCmdQueue::clear()
{
    mtx.lock();
    ordersSendIndex = 0;
    orders.clear();
    sendOrder();
    mtx.unlock();
}

void AgvCmdQueue::setQueue(const QList<AgvOrder> &ord)
{
    mtx.lock();
    ordersSendIndex = 0;
    orders=ord;
    mtx.unlock();
}

QByteArray AgvCmdQueue::getRfidByte(int rfid)
{
    QByteArray qba;
    qba.append((rfid)&0xFF);
    qba.append((rfid>>8)&0xFF);
    qba.append((rfid>>16)&0xFF);
    qba.append((rfid>>24)&0xFF);
    return qba;
}

//将内容封包
//加入包头、(功能码)、包长、(内容)、校验和、包尾
QByteArray AgvCmdQueue::getSendPacket(QByteArray content)
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

