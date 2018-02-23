#include "agvcmdqueue.h"
#include <QByteArray>
#include "util/common.h"
#include "util/global.h"

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

        //第一次发送任务
        if(ordersSendIndex == 0){
            sendOrder();
        }else{
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
    }
    threadAlreadQuit = true;
}

void AgvCmdQueue::sendOrder()
{
    mtx.lock();
    if(ordersSendIndex>=orders.length()){
        orders.clear();
        ordersSendIndex = 0;
        lastSendOrderAmount = 0;
        mtx.unlock();
        //TODO:完成了队列的发送:
        if(finish!=nullptr)
        {
            finish();
        }
        return ;
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

    QByteArray qba = getSendPacket(content);

    if(toSend != nullptr)
    {
        int sendTime = 3;//失败重复发送三次
        int interval = 200;//200ms记一次发送失败
        int per = 50;//每50ms查看一次结果
        int times = interval/per;//多少次查看结果失败后 记一次发送失败
        int percount = 0;//记录查看结果 失败的次数
        bool sendResult = false;//发送结果 默认是失败的

        for(int i=0;i<sendTime;++i)
        {
            toSend(qba.data(),qba.length());

            while(true)
            {
                QyhSleep(per);

                if(recvQueueNumber==sendQueueNumber)//发送成功
                {
                    sendResult = true;
                    break;
                }

                ++percount;
                if(percount>=times)
                {
                    sendResult = false;
                    break;
                }
            }

            if(sendResult)
            {
                ordersSendIndex+=lastSendOrderAmount;//发送成功，index改变
                break;
            }
        }

        if(!sendResult)++sendQueueNumber;//如果一直发送失败，可能是queueNumber相同造成的，这里增加1，下次就不会还是相同的
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
    //       包长 内容长         校验码 包尾
    int len = 1+content.length()+1+1;
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

