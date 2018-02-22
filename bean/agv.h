#ifndef AGV_H
#define AGV_H

#include <QObject>
#include "agvcmdqueue.h"
#include <QTcpSocket>

class Agv : public QObject
{
    Q_OBJECT
public:
    explicit Agv(QObject *parent = nullptr);
    ~Agv();

    //任务结束回调
    typedef std::function<void (Agv *)> TaskFinishCallback;
    TaskFinishCallback taskFinish;

    //任务错误回调
    typedef std::function<void (int,Agv *)> TaskErrorCallback;
    TaskErrorCallback taskError;

    //任务被打断回调
    typedef std::function<void (Agv *)> TaskInteruptCallback;
    TaskInteruptCallback taskInteruput;

    //更新里程计
    typedef std::function<void (int,Agv *)> UpdateMCallback;
    UpdateMCallback updateM;

    //更新里程计和站点
    typedef std::function<void (int,int,Agv *)> UpdateMRCallback;
    UpdateMRCallback updateMR;

    bool init(QString _ip, int _port, TaskFinishCallback _taskFinish = nullptr, TaskErrorCallback _taskError = nullptr, TaskInteruptCallback _taskInteruput = nullptr, UpdateMCallback _updateM = nullptr, UpdateMRCallback _updateMR = nullptr);

    //开始任务
    void startTask(QList<AgvOrder> &ord);

    //停止、取消任务
    void stopTask();

    void onQueueFinish();

    void onSend(const char *data,int len);

    void setTaskFinishCallback(TaskFinishCallback _taskFinish){
        taskFinish = _taskFinish;
    }

    void setTaskErrorCallback(TaskErrorCallback _taskError){
        taskError = _taskError;
    }

    void setTaskInteruptCallback(TaskInteruptCallback _taskInteruput){
        taskInteruput = _taskInteruput;
    }

    /////////////-------------------------------------------------------------------------
    //ID
    int id = 0;
    //编号
    QString name;
    //IP地址和端口
    QString ip;
    int port = 0;

    //用于计算当前位置信息
    int lastStationOdometer = 0;//上一个点位的里程计
    int lastRfid = 0;

    //当前位置
    int x = 0;
    int y = 0;
    int rotation = 0;

    //计算路径用的
    int task = 0;
    int lastStation = 0;
    int nowStation = 0;
    int nextStation = 0;

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
    int status = AGV_STATUS_IDLE;

    //模式
    enum{
        AGV_MODE_AUTO = 0,//自动模式
        AGV_MODE_HAND = 1//手动模式
    };
    int mode = AGV_MODE_AUTO;

    int mileage = 0;//行驶距离 (mm)
    int currentRfid = 0;//当前rfid号，后8位
    int current = 0;//电流 0.1A
    int voltage = 0;//电压 0.01v
    int positionMagneticStripe = 0;//当前磁条位置
    int pcbTemperature = 0;//温度 主控板的
    int motorTemperature = 0;//温度 电机的
    int cpu = 0;//cpu使用率
    int speed = 0;//速度 [1,10]
    int angle = 0;//保存反馈的小车转向角度[-90,90]
    int height = 0;//叉脚高度cm [0,250]

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
    int error_no = 0x00;//错误代码

    int recvQueueNumber = 0;//当前命令的序列编号
    int orderCount = 0;//当前执行到命令条数(0,4)
    int nextRfid = 0;//下一个目标ID号
    int CRC = 0;//crc和

    QList<int> currentPath;
    //执行任务
    int currentTaskId = 0;

    enum{
        AGV_DOING_PICKING = 0,
        AGV_DOING_PUTTING = 1,
        AGV_DOING_STANDING = 2,
        AGV_DOING_NOTHING = 3,
    };
    int agvDoing = AGV_DOING_NOTHING;

    /////////////-------------------------------------------------------------------------

signals:

public slots:
    void onRecv();
    void onConnect();
    void onDisconnect();
private:
    AgvCmdQueue *cmdQueue;//维护长队列、短队列
    QTcpSocket *connection;//维护和AGV的连接

    void processOnePack(QByteArray qba);
};

#endif // AGV_H
