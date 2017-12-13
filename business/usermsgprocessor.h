#ifndef USERMSGPROCESSOR_H
#define USERMSGPROCESSOR_H

#include <QThread>
#include "util/global.h"

class UserMsgProcessor : public QThread
{
    Q_OBJECT
public:
    explicit UserMsgProcessor(QObject *parent = nullptr);
    ~UserMsgProcessor();
    void run() override;
    void myquit();

    QString makeAccessToken();


signals:

public slots:

private:
    volatile bool isQuit;

    //对接收到的一条消息进行xml解析
    void parseOneMsg(const QyhMsgDateItem &item, const std::string &oneMsg);

    //对解析后的请求信息，进行处理，并得到相应的回应内容
    void responseOneMsg(const QyhMsgDateItem &item, QMap<QString,QString> requestDatas, QList<QMap<QString,QString> > datalists);

    //检查请求的参数里是否包含应该包含的参数，并且不为空。如果不包含或者为空，直接将fail和错误信息写入responseParams中。
    bool checkParamExistAndNotNull(QMap<QString, QString> &requestDatas, QMap<QString, QString> &responseParams, const char *s,...);

    //检查access_token，如果不正确，那就直接写入response中。如果正确，返回这个登录用户的信息
    bool checkAccessToken(const QyhMsgDateItem &item,QMap<QString,QString> &requestDatas,QMap<QString,QString> &responseParams,LoginUserInfo &loginUserInfo);

    //对小心的type分类后，分别调用一下函数进行处理
    void clientMsgUserProcess(const QyhMsgDateItem &item, QMap<QString, QString> &requestDatas, QList<QMap<QString, QString> > &datalists);
    void clientMsgMapProcess(const QyhMsgDateItem &item,QMap<QString,QString> &requestDatas,QList<QMap<QString,QString> > &datalists);
    void clientMsgAgvProcess(const QyhMsgDateItem &item,QMap<QString,QString> &requestDatas,QList<QMap<QString,QString> > &datalists);
    void clientMsgAgvManageProcess(const QyhMsgDateItem &item,QMap<QString,QString> &requestDatas,QList<QMap<QString,QString> > &datalists);
    void clientMsgTaskProcess(const QyhMsgDateItem &item,QMap<QString,QString> &requestDatas,QList<QMap<QString,QString> > &datalists);
    void clientMsgLogProcess(const QyhMsgDateItem &item,QMap<QString,QString> &requestDatas,QList<QMap<QString,QString> > &datalists);


    //接下来是具体的业务
    /////////////////////////////关于用户部分
    //用户登录
    void User_Login(const QyhMsgDateItem &item, QMap<QString, QString> &requestDatas, QList<QMap<QString, QString> > &datalists,QMap<QString,QString> &responseParams,QList<QMap<QString,QString> > &responseDatalists,LoginUserInfo &loginUserInfo);
    //用户登出
    void User_Logout(const QyhMsgDateItem &item, QMap<QString, QString> &requestDatas, QList<QMap<QString, QString> > &datalists,QMap<QString,QString> &responseParams,QList<QMap<QString,QString> > &responseDatalists,LoginUserInfo &loginUserInfo);
    //修改密码
    void User_ChangePassword(const QyhMsgDateItem &item, QMap<QString, QString> &requestDatas, QList<QMap<QString, QString> > &datalists,QMap<QString,QString> &responseParams,QList<QMap<QString,QString> > &responseDatalists,LoginUserInfo &loginUserInfo);
    //用户列表
    void User_List(const QyhMsgDateItem &item, QMap<QString, QString> &requestDatas, QList<QMap<QString, QString> > &datalists,QMap<QString,QString> &responseParams,QList<QMap<QString,QString> > &responseDatalists,LoginUserInfo &loginUserInfo);
    //删除用户
    void User_Delete(const QyhMsgDateItem &item, QMap<QString, QString> &requestDatas, QList<QMap<QString, QString> > &datalists,QMap<QString,QString> &responseParams,QList<QMap<QString,QString> > &responseDatalists,LoginUserInfo &loginUserInfo);
    //添加用户
    void User_Add(const QyhMsgDateItem &item, QMap<QString, QString> &requestDatas, QList<QMap<QString, QString> > &datalists,QMap<QString,QString> &responseParams,QList<QMap<QString,QString> > &responseDatalists,LoginUserInfo &loginUserInfo);
    //修改用户
    void User_Modify(const QyhMsgDateItem &item, QMap<QString, QString> &requestDatas, QList<QMap<QString, QString> > &datalists,QMap<QString,QString> &responseParams,QList<QMap<QString,QString> > &responseDatalists,LoginUserInfo &loginUserInfo);

    /////////////////////////////关于地图部分
    //创建地图
    void Map_Create(const QyhMsgDateItem &item, QMap<QString, QString> &requestDatas, QList<QMap<QString, QString> > &datalists,QMap<QString,QString> &responseParams,QList<QMap<QString,QString> > &responseDatalists,LoginUserInfo &loginUserInfo);
    //地图 站点列表
    void Map_StationList(const QyhMsgDateItem &item, QMap<QString, QString> &requestDatas, QList<QMap<QString, QString> > &datalists,QMap<QString,QString> &responseParams,QList<QMap<QString,QString> > &responseDatalists,LoginUserInfo &loginUserInfo);
    //地图 线路列表
    void Map_LineList(const QyhMsgDateItem &item, QMap<QString, QString> &requestDatas, QList<QMap<QString, QString> > &datalists,QMap<QString,QString> &responseParams,QList<QMap<QString,QString> > &responseDatalists,LoginUserInfo &loginUserInfo);
    //订阅车辆位置信息
    void Map_AgvPositionSubscribe(const QyhMsgDateItem &item, QMap<QString, QString> &requestDatas, QList<QMap<QString, QString> > &datalists,QMap<QString,QString> &responseParams,QList<QMap<QString,QString> > &responseDatalists,LoginUserInfo &loginUserInfo);
    //取消订阅车辆位置信息
    void Map_AgvPositionCancelSubscribe(const QyhMsgDateItem &item, QMap<QString, QString> &requestDatas, QList<QMap<QString, QString> > &datalists,QMap<QString,QString> &responseParams,QList<QMap<QString,QString> > &responseDatalists,LoginUserInfo &loginUserInfo);
    //获取地图背景
    void Map_GetImage(const QyhMsgDateItem &item, QMap<QString, QString> &requestDatas, QList<QMap<QString, QString> > &datalists,QMap<QString,QString> &responseParams,QList<QMap<QString,QString> > &responseDatalists,LoginUserInfo &loginUserInfo);
    //设置地图背景
    void Map_SetImage(const QyhMsgDateItem &item, QMap<QString, QString> &requestDatas, QList<QMap<QString, QString> > &datalists,QMap<QString,QString> &responseParams,QList<QMap<QString,QString> > &responseDatalists,LoginUserInfo &loginUserInfo);

    //查询左中右信息

    //查询adj信息


    /////////////////////////////关于手控部分
    //请求小车控制权
    void Agv_Hand(const QyhMsgDateItem &item, QMap<QString, QString> &requestDatas, QList<QMap<QString, QString> > &datalists,QMap<QString,QString> &responseParams,QList<QMap<QString,QString> > &responseDatalists,LoginUserInfo &loginUserInfo);
    //释放小车控制权
    void Agv_Release(const QyhMsgDateItem &item, QMap<QString, QString> &requestDatas, QList<QMap<QString, QString> > &datalists,QMap<QString,QString> &responseParams,QList<QMap<QString,QString> > &responseDatalists,LoginUserInfo &loginUserInfo);
    //前进
    void Agv_Forward(const QyhMsgDateItem &item, QMap<QString, QString> &requestDatas, QList<QMap<QString, QString> > &datalists,QMap<QString,QString> &responseParams,QList<QMap<QString,QString> > &responseDatalists,LoginUserInfo &loginUserInfo);
    //后退
    void Agv_Backward(const QyhMsgDateItem &item, QMap<QString, QString> &requestDatas, QList<QMap<QString, QString> > &datalists,QMap<QString,QString> &responseParams,QList<QMap<QString,QString> > &responseDatalists,LoginUserInfo &loginUserInfo);
    //左转
    void Agv_Turnleft(const QyhMsgDateItem &item, QMap<QString, QString> &requestDatas, QList<QMap<QString, QString> > &datalists,QMap<QString,QString> &responseParams,QList<QMap<QString,QString> > &responseDatalists,LoginUserInfo &loginUserInfo);
    //右转
    void Agv_Turnright(const QyhMsgDateItem &item, QMap<QString, QString> &requestDatas, QList<QMap<QString, QString> > &datalists,QMap<QString,QString> &responseParams,QList<QMap<QString,QString> > &responseDatalists,LoginUserInfo &loginUserInfo);
    //灯带
    void Agv_Light(const QyhMsgDateItem &item, QMap<QString, QString> &requestDatas, QList<QMap<QString, QString> > &datalists,QMap<QString,QString> &responseParams,QList<QMap<QString,QString> > &responseDatalists,LoginUserInfo &loginUserInfo);
    //小车状态订阅
    void Agv_StatusSubscribte(const QyhMsgDateItem &item, QMap<QString, QString> &requestDatas, QList<QMap<QString, QString> > &datalists,QMap<QString,QString> &responseParams,QList<QMap<QString,QString> > &responseDatalists,LoginUserInfo &loginUserInfo);
    //取消小车状态订阅
    void Agv_CancelStatusSubscribe(const QyhMsgDateItem &item, QMap<QString, QString> &requestDatas, QList<QMap<QString, QString> > &datalists,QMap<QString,QString> &responseParams,QList<QMap<QString,QString> > &responseDatalists,LoginUserInfo &loginUserInfo);

    /////////////////////////////////车辆管理部分
    //列表
    void AgvManage_List(const QyhMsgDateItem &item, QMap<QString, QString> &requestDatas, QList<QMap<QString, QString> > &datalists,QMap<QString,QString> &responseParams,QList<QMap<QString,QString> > &responseDatalists,LoginUserInfo &loginUserInfo);
    //增加
    void AgvManage_Add(const QyhMsgDateItem &item, QMap<QString, QString> &requestDatas, QList<QMap<QString, QString> > &datalists,QMap<QString,QString> &responseParams,QList<QMap<QString,QString> > &responseDatalists,LoginUserInfo &loginUserInfo);
    //删除
    void AgvManage_Delete(const QyhMsgDateItem &item, QMap<QString, QString> &requestDatas, QList<QMap<QString, QString> > &datalists,QMap<QString,QString> &responseParams,QList<QMap<QString,QString> > &responseDatalists,LoginUserInfo &loginUserInfo);
    //修改
    void AgvManage_Modify(const QyhMsgDateItem &item, QMap<QString, QString> &requestDatas, QList<QMap<QString, QString> > &datalists,QMap<QString,QString> &responseParams,QList<QMap<QString,QString> > &responseDatalists,LoginUserInfo &loginUserInfo);

    ////////////////////////////////任务部分
    //订阅任务状态信息（简单显示）
    void Task_Subscribe(const QyhMsgDateItem &item, QMap<QString, QString> &requestDatas, QList<QMap<QString, QString> > &datalists,QMap<QString,QString> &responseParams,QList<QMap<QString,QString> > &responseDatalists,LoginUserInfo &loginUserInfo);
    //取消任务状态信息订阅
    void Task_CancelSubscribe(const QyhMsgDateItem &item, QMap<QString, QString> &requestDatas, QList<QMap<QString, QString> > &datalists,QMap<QString,QString> &responseParams,QList<QMap<QString,QString> > &responseDatalists,LoginUserInfo &loginUserInfo);

    //创建任务(创建到X点的任务)
    void Task_CreateToX(const QyhMsgDateItem &item, QMap<QString, QString> &requestDatas, QList<QMap<QString, QString> > &datalists,QMap<QString,QString> &responseParams,QList<QMap<QString,QString> > &responseDatalists,LoginUserInfo &loginUserInfo);
    //创建任务(创建指定车辆到X点的任务)
    void Task_CreateAgvToX(const QyhMsgDateItem &item, QMap<QString, QString> &requestDatas, QList<QMap<QString, QString> > &datalists,QMap<QString,QString> &responseParams,QList<QMap<QString,QString> > &responseDatalists,LoginUserInfo &loginUserInfo);
    //创建任务(创建经过Y点到X点的任务)
    void Task_CreateYToX(const QyhMsgDateItem &item, QMap<QString, QString> &requestDatas, QList<QMap<QString, QString> > &datalists,QMap<QString,QString> &responseParams,QList<QMap<QString,QString> > &responseDatalists,LoginUserInfo &loginUserInfo);
    //创建任务(创建指定车辆经过Y点到X点的任务)
    void Task_CreateAgvYToX(const QyhMsgDateItem &item, QMap<QString, QString> &requestDatas, QList<QMap<QString, QString> > &datalists,QMap<QString,QString> &responseParams,QList<QMap<QString,QString> > &responseDatalists,LoginUserInfo &loginUserInfo);
    //查询任务状态
    void Task_QueryStatus(const QyhMsgDateItem &item, QMap<QString, QString> &requestDatas, QList<QMap<QString, QString> > &datalists,QMap<QString,QString> &responseParams,QList<QMap<QString,QString> > &responseDatalists,LoginUserInfo &loginUserInfo);
    //取消任务
    void Task_Cancel(const QyhMsgDateItem &item, QMap<QString, QString> &requestDatas, QList<QMap<QString, QString> > &datalists,QMap<QString,QString> &responseParams,QList<QMap<QString,QString> > &responseDatalists,LoginUserInfo &loginUserInfo);
    //未分配任务列表
    void Task_ListUnassigned(const QyhMsgDateItem &item, QMap<QString, QString> &requestDatas, QList<QMap<QString, QString> > &datalists,QMap<QString,QString> &responseParams,QList<QMap<QString,QString> > &responseDatalists,LoginUserInfo &loginUserInfo);
    //正在执行任务列表
    void Task_ListDoing(const QyhMsgDateItem &item, QMap<QString, QString> &requestDatas, QList<QMap<QString, QString> > &datalists,QMap<QString,QString> &responseParams,QList<QMap<QString,QString> > &responseDatalists,LoginUserInfo &loginUserInfo);
    //已经完成任务列表(today)
    void Task_ListDoneToday(const QyhMsgDateItem &item, QMap<QString, QString> &requestDatas, QList<QMap<QString, QString> > &datalists,QMap<QString,QString> &responseParams,QList<QMap<QString,QString> > &responseDatalists,LoginUserInfo &loginUserInfo);
    //已经完成任务列表(all)
    void Task_ListDoneAll(const QyhMsgDateItem &item, QMap<QString, QString> &requestDatas, QList<QMap<QString, QString> > &datalists,QMap<QString,QString> &responseParams,QList<QMap<QString,QString> > &responseDatalists,LoginUserInfo &loginUserInfo);
    //已经完成任务列表(from to 时间)
    void Task_ListDoneDuring(const QyhMsgDateItem &item, QMap<QString, QString> &requestDatas, QList<QMap<QString, QString> > &datalists,QMap<QString,QString> &responseParams,QList<QMap<QString,QString> > &responseDatalists,LoginUserInfo &loginUserInfo);
    //单个任务的详细情况
    void Task_Detail(const QyhMsgDateItem &item, QMap<QString, QString> &requestDatas, QList<QMap<QString, QString> > &datalists,QMap<QString,QString> &responseParams,QList<QMap<QString,QString> > &responseDatalists,LoginUserInfo &loginUserInfo);

    ////////////////////////////////////日志管理
    //查询日志 from to时间
    void Log_ListDuring(const QyhMsgDateItem &item, QMap<QString, QString> &requestDatas, QList<QMap<QString, QString> > &datalists,QMap<QString,QString> &responseParams,QList<QMap<QString,QString> > &responseDatalists,LoginUserInfo &loginUserInfo);
    //查询所有日志
    void Log_ListAll(const QyhMsgDateItem &item, QMap<QString, QString> &requestDatas, QList<QMap<QString, QString> > &datalists,QMap<QString,QString> &responseParams,QList<QMap<QString,QString> > &responseDatalists,LoginUserInfo &loginUserInfo);
    //订阅日志
    void Log_Subscribe(const QyhMsgDateItem &item, QMap<QString, QString> &requestDatas, QList<QMap<QString, QString> > &datalists,QMap<QString,QString> &responseParams,QList<QMap<QString,QString> > &responseDatalists,LoginUserInfo &loginUserInfo);
    //取消订阅日志
    void Log_CancelSubscribe(const QyhMsgDateItem &item, QMap<QString, QString> &requestDatas, QList<QMap<QString, QString> > &datalists,QMap<QString,QString> &responseParams,QList<QMap<QString,QString> > &responseDatalists,LoginUserInfo &loginUserInfo);
};

#endif // USERMSGPROCESSOR_H
