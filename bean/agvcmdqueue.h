#ifndef AGVCMDQUEUE_H
#define AGVCMDQUEUE_H

#include <list>
#include <mutex>
#include <QByteArray>
/*
 * 发送给Agv的命令的队列及其处理线程
 * 队列的单个内容是AgvOrder
 * 维护长队列
 */

//包头
#define AGV_PACK_HEAD       0x55
//包尾
#define AGV_PACK_END        0xAA

//功能码: 手动模式
#define AGV_PACK_SEND_CODE_CONFIG_MODE      0x70
//功能码: 手动模式
#define AGV_PACK_SEND_CODE_HAND_MODE        0x71
//功能码: 手动模式
#define AGV_PACK_SEND_CODE_AUTO_MODE        0x72
//功能码：自动模式
#define AGV_PACK_SEND_CODE_DISPATCH_MODE    0x73

const char CHAR_NULL = '\0';

class AgvOrder{
public:
    //卡ID(也就是RFID)
    enum{
        RFID_CODE_IMMEDIATELY = 0x00000000,//立即执行命令
        RFID_CODE_EMPTY = 0xFFFFFFFF,//空卡
    };
    int rfid = RFID_CODE_EMPTY;

    //命令
    enum{
        ORDER_STOP = 0x00,//停止 param延时时间 [0x0,0xf]
        ORDER_FORWARD = 0x01,//前进 param速度代码[0,10]
        ORDER_BACKWARD = 0x02,//后退 param速度代码[0,10]
        ORDER_TURN_LEFT = 0x03,//左转 param转向角度[0,180] 0为寻磁方向
        ORDER_TURN_RIGHT = 0x04,//右转 param转向角度[0,180] 0为寻磁方向
        ORDER_MP3_FILE = 0x05,//MP3文件ID
        ORDER_MP3_VOLUME = 0x06,//MP3音量（1-10）
        ORDER_FORWARD_STRIPE = 0x08,//前进到寻磁 param为最远距离(cm)
        ORDER_BACKWARD_PLATE = 0x09,//后退到栈板触发 param为最远距离(cm)
        ORDER_UP_DOWN = 0xA0,//升降插齿 param为距离 [0,255] cm
    };
    int order = ORDER_STOP;
    //参数
    int param = 0x00;
};


class AgvCmdQueue
{
public:

    typedef std::function<bool (const char *data,int len)> ToSendCallback;

    enum{
        PICK_PUT_HEIGHT = 30,//叉起或者放下需要的升降的高度
    };

    AgvCmdQueue();
    ~AgvCmdQueue();

    void init(ToSendCallback _toSend);

    void clear();

    void setQueue(std::list<AgvOrder> ord);

    void onOrderQueueChanged(int queueNumber,int orderQueueNumber);

    static void cmdThread(void *param);
private:
    void cmdProcess();
    QByteArray getSendPacket(QByteArray content);
    QByteArray getRfidByte(int rfid);
    void sendOrder();
    std::list<AgvOrder> orders;
    std::mutex mtx;
    volatile bool quit;
    volatile bool threadAlreadQuit;

    int8_t sendQueueNumber = 0;
    volatile int recvQueueNumber = 0;//当前命令的序列编号
    volatile int orderExcuteIndex = 0;//当前执行到命令条数(0,4)
    volatile int ordersSendIndex = 0;//当前发送指令的下标
    volatile int lastSendOrderAmount = 0;//上一次发送的指令的数量

    ToSendCallback toSend;
};

#endif // AGVCMDQUEUE_H
