#ifndef USERMSGPROCESSOR_H
#define USERMSGPROCESSOR_H

#include <string>
#include <QMutex>
#include <QThread>
#include <zmq.hpp>


class UserMsgProcessor : public QThread
{
    Q_OBJECT
public:
    explicit UserMsgProcessor(QObject *parent = nullptr);
    ~UserMsgProcessor();

    typedef struct LoginUserInfo
    {
        LoginUserInfo() {}
        int id;//ID
        void *ctx;
        QString password;//密码
        QString username;//用户名
        int role;//角色
        QString access_tocken;//随机码(安全码)，登录以后，给用户的，之后的所有请求要求带上随机码。
    }Login_User_Info;

    //    void run() override;
    void myquit();

    //对接收到的一条消息进行xml解析
    std::string parseOneMsg(zmq::context_t * ctx, const std::string &oneMsg);
signals:

public slots:

private:
    volatile bool isQuit;

    QString makeAccessToken();

    //对解析后的请求信息，进行处理，并得到相应的回应内容
    std::string responseOneMsg(zmq::context_t *ctx, QMap<QString,QString> requestDatas, QList<QMap<QString,QString> > datalists);

    //检查请求的参数里是否包含应该包含的参数，并且不为空。如果不包含或者为空，直接将fail和错误信息写入responseParams中。
    bool checkParamExistAndNotNull(QMap<QString, QString> &requestDatas, QMap<QString, QString> &responseParams, const char *s,...);

    //检查access_token，如果不正确，那就直接写入response中。如果正确，返回这个登录用户的信息
    bool checkAccessToken(zmq::context_t *ctx,QMap<QString,QString> &requestDatas,QMap<QString,QString> &responseParams,LoginUserInfo &loginUserinfo);

    //对小心的type分类后，分别调用一下函数进行处理
    std::string clientMsgUserProcess(zmq::context_t *ctx, QMap<QString, QString> &requestDatas, QList<QMap<QString, QString> > &datalists);
    std::string clientMsgMapProcess(zmq::context_t *ctx,QMap<QString,QString> &requestDatas,QList<QMap<QString,QString> > &datalists);
    std::string clientMsgAgvProcess(zmq::context_t *ctx,QMap<QString,QString> &requestDatas,QList<QMap<QString,QString> > &datalists);
    std::string clientMsgAgvManageProcess(zmq::context_t *ctx,QMap<QString,QString> &requestDatas,QList<QMap<QString,QString> > &datalists);
    std::string clientMsgTaskProcess(zmq::context_t *ctx,QMap<QString,QString> &requestDatas,QList<QMap<QString,QString> > &datalists);
    std::string clientMsgLogProcess(zmq::context_t *ctx,QMap<QString,QString> &requestDatas,QList<QMap<QString,QString> > &datalists);


    //接下来是具体的业务
    /////////////////////////////关于用户部分
    //用户登录
    void User_Login(zmq::context_t *ctx, QMap<QString, QString> &requestDatas, QList<QMap<QString, QString> > &datalists,QMap<QString,QString> &responseParams,QList<QMap<QString,QString> > &responseDatalists,LoginUserInfo &loginUserInfo);
    //用户登出
    void User_Logout(zmq::context_t *ctx, QMap<QString, QString> &requestDatas, QList<QMap<QString, QString> > &datalists,QMap<QString,QString> &responseParams,QList<QMap<QString,QString> > &responseDatalists);
    //修改密码
    void User_ChangePassword(zmq::context_t *ctx, QMap<QString, QString> &requestDatas, QList<QMap<QString, QString> > &datalists,QMap<QString,QString> &responseParams,QList<QMap<QString,QString> > &responseDatalists);
    //用户列表
    void User_List(zmq::context_t *ctx, QMap<QString, QString> &requestDatas, QList<QMap<QString, QString> > &datalists,QMap<QString,QString> &responseParams,QList<QMap<QString,QString> > &responseDatalists,LoginUserInfo &loginUserInfo);
    //删除用户
    void User_Delete(zmq::context_t *ctx, QMap<QString, QString> &requestDatas, QList<QMap<QString, QString> > &datalists,QMap<QString,QString> &responseParams,QList<QMap<QString,QString> > &responseDatalists);
    //添加用户
    void User_Add(zmq::context_t *ctx, QMap<QString, QString> &requestDatas, QList<QMap<QString, QString> > &datalists,QMap<QString,QString> &responseParams,QList<QMap<QString,QString> > &responseDatalists);
    //修改用户
    void User_Modify(zmq::context_t *ctx, QMap<QString, QString> &requestDatas, QList<QMap<QString, QString> > &datalists,QMap<QString,QString> &responseParams,QList<QMap<QString,QString> > &responseDatalists);

    /////////////////////////////关于地图部分
    //创建地图
    void Map_Create(zmq::context_t *ctx, QMap<QString, QString> &requestDatas, QList<QMap<QString, QString> > &datalists,QMap<QString,QString> &responseParams,QList<QMap<QString,QString> > &responseDatalists);
    //地图 站点列表
    void Map_StationList(zmq::context_t *ctx, QMap<QString, QString> &requestDatas, QList<QMap<QString, QString> > &datalists,QMap<QString,QString> &responseParams,QList<QMap<QString,QString> > &responseDatalists);
    //地图 线路列表
    void Map_LineList(zmq::context_t *ctx, QMap<QString, QString> &requestDatas, QList<QMap<QString, QString> > &datalists,QMap<QString,QString> &responseParams,QList<QMap<QString,QString> > &responseDatalists);

    //查询左中右信息

    //查询adj信息


    /////////////////////////////关于手控部分
    //    //请求小车控制权
    //    void Agv_Hand(zmq::context_t *ctx, QMap<QString, QString> &requestDatas, QList<QMap<QString, QString> > &datalists,QMap<QString,QString> &responseParams,QList<QMap<QString,QString> > &responseDatalists);
    //    //释放小车控制权
    //    void Agv_Release(zmq::context_t *ctx, QMap<QString, QString> &requestDatas, QList<QMap<QString, QString> > &datalists,QMap<QString,QString> &responseParams,QList<QMap<QString,QString> > &responseDatalists);
    //    //前进
    //    void Agv_Forward(zmq::context_t *ctx, QMap<QString, QString> &requestDatas, QList<QMap<QString, QString> > &datalists,QMap<QString,QString> &responseParams,QList<QMap<QString,QString> > &responseDatalists);
    //    //后退
    //    void Agv_Backward(zmq::context_t *ctx, QMap<QString, QString> &requestDatas, QList<QMap<QString, QString> > &datalists,QMap<QString,QString> &responseParams,QList<QMap<QString,QString> > &responseDatalists);
    //    //左转
    //    void Agv_Turnleft(zmq::context_t *ctx, QMap<QString, QString> &requestDatas, QList<QMap<QString, QString> > &datalists,QMap<QString,QString> &responseParams,QList<QMap<QString,QString> > &responseDatalists);
    //    //右转
    //    void Agv_Turnright(zmq::context_t *ctx, QMap<QString, QString> &requestDatas, QList<QMap<QString, QString> > &datalists,QMap<QString,QString> &responseParams,QList<QMap<QString,QString> > &responseDatalists);
    //灯带
    void Agv_Light(zmq::context_t *ctx, QMap<QString, QString> &requestDatas, QList<QMap<QString, QString> > &datalists,QMap<QString,QString> &responseParams,QList<QMap<QString,QString> > &responseDatalists);

    /////////////////////////////////车辆管理部分
    //列表
    void AgvManage_List(zmq::context_t *ctx, QMap<QString, QString> &requestDatas, QList<QMap<QString, QString> > &datalists,QMap<QString,QString> &responseParams,QList<QMap<QString,QString> > &responseDatalists);
    //增加
    void AgvManage_Add(zmq::context_t *ctx, QMap<QString, QString> &requestDatas, QList<QMap<QString, QString> > &datalists,QMap<QString,QString> &responseParams,QList<QMap<QString,QString> > &responseDatalists);
    //删除
    void AgvManage_Delete(zmq::context_t *ctx, QMap<QString, QString> &requestDatas, QList<QMap<QString, QString> > &datalists,QMap<QString,QString> &responseParams,QList<QMap<QString,QString> > &responseDatalists);
    //修改
    void AgvManage_Modify(zmq::context_t *ctx, QMap<QString, QString> &requestDatas, QList<QMap<QString, QString> > &datalists,QMap<QString,QString> &responseParams,QList<QMap<QString,QString> > &responseDatalists);

    ////////////////////////////////任务部分

    //创建任务(创建到X点的任务)
    void Task_CreateToX(zmq::context_t *ctx, QMap<QString, QString> &requestDatas, QList<QMap<QString, QString> > &datalists,QMap<QString,QString> &responseParams,QList<QMap<QString,QString> > &responseDatalists);
    //创建任务(创建指定车辆到X点的任务)
    void Task_CreateAgvToX(zmq::context_t *ctx, QMap<QString, QString> &requestDatas, QList<QMap<QString, QString> > &datalists,QMap<QString,QString> &responseParams,QList<QMap<QString,QString> > &responseDatalists);
    //创建任务(创建经过Y点到X点的任务)
    void Task_CreateYToX(zmq::context_t *ctx, QMap<QString, QString> &requestDatas, QList<QMap<QString, QString> > &datalists,QMap<QString,QString> &responseParams,QList<QMap<QString,QString> > &responseDatalists);
    //创建任务(创建指定车辆经过Y点到X点的任务)
    void Task_CreateAgvYToX(zmq::context_t *ctx, QMap<QString, QString> &requestDatas, QList<QMap<QString, QString> > &datalists,QMap<QString,QString> &responseParams,QList<QMap<QString,QString> > &responseDatalists);
    //创建任务(创建指定车辆经过Y点到X点的任务)
    void Task_CreateAgvYToXCircle(zmq::context_t *ctx, QMap<QString, QString> &requestDatas, QList<QMap<QString, QString> > &datalists,QMap<QString,QString> &responseParams,QList<QMap<QString,QString> > &responseDatalists);
    //查询任务状态
    void Task_QueryStatus(zmq::context_t *ctx, QMap<QString, QString> &requestDatas, QList<QMap<QString, QString> > &datalists,QMap<QString,QString> &responseParams,QList<QMap<QString,QString> > &responseDatalists);
    //取消任务
    void Task_Cancel(zmq::context_t *ctx, QMap<QString, QString> &requestDatas, QList<QMap<QString, QString> > &datalists,QMap<QString,QString> &responseParams,QList<QMap<QString,QString> > &responseDatalists);
    //未分配任务列表
    void Task_ListUnassigned(zmq::context_t *ctx, QMap<QString, QString> &requestDatas, QList<QMap<QString, QString> > &datalists,QMap<QString,QString> &responseParams,QList<QMap<QString,QString> > &responseDatalists);
    //正在执行任务列表
    void Task_ListDoing(zmq::context_t *ctx, QMap<QString, QString> &requestDatas, QList<QMap<QString, QString> > &datalists,QMap<QString,QString> &responseParams,QList<QMap<QString,QString> > &responseDatalists);
    //已经完成任务列表(today)
    void Task_ListDoneToday(zmq::context_t *ctx, QMap<QString, QString> &requestDatas, QList<QMap<QString, QString> > &datalists,QMap<QString,QString> &responseParams,QList<QMap<QString,QString> > &responseDatalists);
    //已经完成任务列表(all)
    void Task_ListDoneAll(zmq::context_t *ctx, QMap<QString, QString> &requestDatas, QList<QMap<QString, QString> > &datalists,QMap<QString,QString> &responseParams,QList<QMap<QString,QString> > &responseDatalists);
    //已经完成任务列表(from to 时间)
    void Task_ListDoneDuring(zmq::context_t *ctx, QMap<QString, QString> &requestDatas, QList<QMap<QString, QString> > &datalists,QMap<QString,QString> &responseParams,QList<QMap<QString,QString> > &responseDatalists);
    //单个任务的详细情况
    void Task_Detail(zmq::context_t *ctx, QMap<QString, QString> &requestDatas, QList<QMap<QString, QString> > &datalists,QMap<QString,QString> &responseParams,QList<QMap<QString,QString> > &responseDatalists);

    ////////////////////////////////////日志管理
    //查询日志 from to时间
    void Log_ListDuring(zmq::context_t *ctx, QMap<QString, QString> &requestDatas, QList<QMap<QString, QString> > &datalists,QMap<QString,QString> &responseParams,QList<QMap<QString,QString> > &responseDatalists);
    //查询所有日志
    void Log_ListAll(zmq::context_t *ctx, QMap<QString, QString> &requestDatas, QList<QMap<QString, QString> > &datalists,QMap<QString,QString> &responseParams,QList<QMap<QString,QString> > &responseDatalists);
private:

    ///登录的客户端的id和它对应的sock
    QMutex loginUserIdSockMutex;
    QList<LoginUserInfo> loginUserIdSock;
};

#endif // USERMSGPROCESSOR_H
