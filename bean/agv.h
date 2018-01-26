#ifndef AGV_H
#define AGV_H

#include <QObject>
#include <QTcpSocket>
#include <QQueue>
#include <QTimer>

//一些宏定义
//包头
#define AGV_PACK_HEAD    0x55
//包尾
#define AGV_PACK_END   0xAA

//功能码: 手动模式
#define AGV_PACK_SEND_CODE_CONFIG_MODE    0x70
//功能码: 手动模式
#define AGV_PACK_SEND_CODE_HAND_MODE    0x71
//功能码: 手动模式
#define AGV_PACK_SEND_CODE_AUTO_MODE    0x72
//功能码：自动模式
#define AGV_PACK_SEND_CODE_DISPATCH_MODE   0x73

//rfid是立即执行
#define AGV_PACK_SEND_RFID_CODE_IMMEDIATELY       0x00000000
#define AGV_PACK_SEND_RFID_CODE_ETERNITY          0xFFFFFFFF

//指令代码
#define AGV_PACK_SEND_INSTRUC_CODE_STOP      0x00
#define AGV_PACK_SEND_INSTRUC_CODE_FORWARD      0x01
#define AGV_PACK_SEND_INSTRUC_CODE_BACKWARD      0x02
#define AGV_PACK_SEND_INSTRUC_CODE_LEFT      0x03
#define AGV_PACK_SEND_INSTRUC_CODE_RIGHT      0x04
#define AGV_PACK_SEND_INSTRUC_CODE_MP3LEFT      0x05
#define AGV_PACK_SEND_INSTRUC_CODE_MP3RIGHT      0x06
#define AGV_PACK_SEND_INSTRUC_CODE_MP3VOLUME      0x07A


enum AGV_HAND_TYPE{
    AGV_HAND_TYPE_STOP = 0,//停止移动
    AGV_HAND_TYPE_FORWARD = 0x1,//前进
    AGV_HAND_TYPE_BACKWARD = 0x2,//后退
    AGV_HAND_TYPE_TURNLEFT = 0x3,//左转
    AGV_HAND_TYPE_TURNRIGHT = 0x4,//右转
};

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

class Agv : public QObject
{
    Q_OBJECT
public:

    enum{
        PICK_PUT_HEIGHT = 30,//叉起或者放下需要的升降的高度
    };

    explicit Agv(QObject *parent = nullptr);
    void init(QString _ip,int _port);
    bool send(const char *data,int len);


    void doPick();
    void doPut();
    void doStandBy();

    void doStop();

signals:
    void sigconnect();
    void sigdisconnect();

    //任务执行过程中，因为手动模式，被取消了
    void taskCancel(int taskId);

    //任务执行过程中，报错了，任务错误
    void taskError(int taskId);

    void updateOdometer(int odometer);
    void updateRfidAndOdometer(int rfid,int odomter);

    void pickFinish();
    void putFinish();
    void standByFinish();

public slots:
    void onAgvRead();
    void connectCallBack();
    void disconnectCallBack();

    void onCheckOrder();

    void connectStateChanged(QAbstractSocket::SocketState s);
public:
    //ID
    int id;
    //编号
    QString name;
    //IP地址和端口
    QString ip;
    int port;

    //用于计算当前位置信息
    int lastStationOdometer;//上一个点位的里程计
    int lastRfid;

    //当前位置
    int x;
    int y;
    int rotation;

    //计算路径用的
    int task;
    int lastStation;
    int nowStation;
    int nextStation;

    //用于发送用的
    int8_t sendQueueNumber;

    //状态
    enum{
        AGV_STATUS_HANDING = -1,//手动模式中，不可用
        AGV_STATUS_IDLE=0,//空闲可用
        AGV_STATUS_UNCONNECT=1,//未连接
        AGV_STATUS_TASKING=2,//正在执行任务
        AGV_STATUS_POWER_LOW=3,//电量低
        AGV_STATUS_ERROR=4,//故障
        AGV_STATUS_GO_CHARGING = 5,//返回充电中
        AGV_STATUS_CHARGING=6,//正在充电
    };
    int status;

    //上报内容----------------------------------------------------
    //模式
    enum{
        AGV_MODE_AUTO = 0,//自动模式
        AGV_MODE_HAND = 1//手动模式
    };
    int mode;

    int mileage;//行驶距离 (mm)
    int currentRfid;//当前rfid号，后8位
    int current;//电流 0.1A
    int voltage;//电压 0.01v
    int positionMagneticStripe;//当前磁条位置
    int pcbTemperature;//温度 主控板的
    int motorTemperature;//温度 电机的
    int cpu;//cpu使用率
    int speed;//速度 [1,10]
    int angle;//保存反馈的小车转向角度[-90,90]
    int height;//叉脚高度cm [0,250]

    enum{
        ERROR_D7 = 0xD7,//手动手柄放下。
        ERROR_D6 = 0xD6,//踏板放下
        ERROR_D5 = 0xD5,//踏板站人
        ERROR_D4 = 0xD4,//左腰靠放下
        ERROR_D3 = 0xD3,//右腰靠放下
        ERROR_D2 = 0xD2,//叉车主控断线。
        ERROR_D1 = 0xD1,//磁传感器断线
        ERROR_D0 = 0xD0,//地标传感器断线。
    };
    int error_no;//错误代码

    int recvQueueNumber;//当前命令的序列编号
    int orderCount;//当前执行到命令条数(0,4)
    int nextRfid;//下一个目标ID号
    int CRC;//crc和
    //上报内容----------------------------------------------------

    //开机上报的--------------------------------------------------
    //TODO
    char NET_MAC[8];//网关MAC
    char NET_IP[2];//ip两位
    //开机上报的--------------------------------------------------

    //和agv的网络连接
    QTcpSocket *tcpClient;

    //执行序列
    QList<AgvOrder> orders;
    int ordersIndex;
    int lastSendOrderAmount;


    QList<int> currentPath;

    //执行任务
    int currentTaskId;

    QTimer orderTimer;

private:
    enum{
        AGV_DOING_PICKING = 0,
        AGV_DOING_PUTTING = 1,
        AGV_DOING_STANDING = 2,
        AGV_DOING_NOTHING = 3,
    };
    int agvDoing = AGV_DOING_NOTHING;

    void processOnePack(QByteArray qba);
    void sendOrder();
    QByteArray getSendPacket(QByteArray content);
};

#endif // AGV_H
