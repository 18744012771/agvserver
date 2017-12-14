#include "usermsgprocessor.h"
#include "util/global.h"
#include "pugixml.hpp"
#include <sstream>
#include <iostream>
#include <QUuid>
#include <stdarg.h>
#include <QDebug>

UserMsgProcessor::UserMsgProcessor(QObject *parent) : QThread(parent),isQuit(false)
{
}
UserMsgProcessor::~UserMsgProcessor(){
    isQuit = true;
}

void UserMsgProcessor::run()
{
    while(!isQuit){
        QyhMsgDateItem item;
        if(g_user_msg_queue.try_dequeue(item)){

            if(item.data.length()==0)continue;

            g_log->log(AGV_LOG_LEVEL_INFO,"get client msg="+QString::fromStdString(item.data));

            parseOneMsg(item,item.data);
        }
        QyhSleep(100);
    }
}

QString UserMsgProcessor::makeAccessToken()
{
    //生成一个随机的16位字符串
    return QUuid::createUuid().toString().replace("{","").replace("}","");
}


void UserMsgProcessor::myquit()
{
    isQuit=true;
}

//对接收到的xml消息进行转换
//这里，采用速度最最快的pluginXml.以保证效率，解析单个xml的时间应该在3ms之内
void UserMsgProcessor::parseOneMsg(const QyhMsgDateItem &item, const std::string &oneMsg)
{
    QMap<QString,QString> params;
    QList<QMap<QString,QString> > datalist;
    //解析
    if(!getRequestParam(oneMsg,params,datalist))return ;

    //初步判断，如果不合格，那就直接淘汰
    QMap<QString,QString>::iterator itr;
    if((itr=params.find("type"))!=params.end()
            &&(itr=params.find("todo"))!=params.end()
            &&(itr=params.find("queuenumber"))!=params.end()){
        //接下来对这条消息进行响应
        LARGE_INTEGER start;
        LARGE_INTEGER end ;
        LARGE_INTEGER frequency;
        QueryPerformanceFrequency(&frequency);
        QueryPerformanceCounter(&start); //开始计时
        responseOneMsg(item,params,datalist);
        QueryPerformanceCounter(&end); //结束计时
        qDebug()<<"reponse one msg total time= "<<1000*(double)(end.QuadPart - start.QuadPart) / (double)frequency.QuadPart<<" ms";
    }
}

bool UserMsgProcessor::checkParamExistAndNotNull(QMap<QString,QString> &requestDatas,QMap<QString,QString> &responseParams,const char* s,...)
{
    bool result = true;
    //argno代表第几个参数 para就是这个参数
    va_list argp;

    const char *para = s;

    va_start(argp, s);
    do{
        //检查参数
        QString strParam = QString(para);
        if(requestDatas.find(strParam)==requestDatas.end()){
            responseParams.insert(QString("info"),QString("param lack:")+strParam);
            responseParams.insert(QString("result"),QString("fail"));
            result =  false;
            break;
        }else if(requestDatas[strParam].length()<=0){
            responseParams.insert(QString("info"),QString("null of:")+strParam);
            responseParams.insert(QString("result"),QString("fail"));
            result =  false;
            break;
        }
        para = va_arg(argp, char *);
    }while(para!=NULL);

    va_end(argp);

    return result;
}

bool UserMsgProcessor::checkAccessToken(const QyhMsgDateItem &item,QMap<QString,QString> &requestDatas,QMap<QString,QString> &responseParams,LoginUserInfo &loginUserInfo)
{
    if(!checkParamExistAndNotNull(requestDatas,responseParams,"access_token",NULL))
        return false;

    ////对access_token判断是否正确
    bool access_token_correct = false;

    if(loginUserIdSock.contains(item.sock)){
        if(loginUserIdSock[item.sock].access_tocken == requestDatas["access_token"]){
            access_token_correct = true;
            loginUserInfo = loginUserIdSock[item.sock];
        }
    }

    if(!access_token_correct){
        //access_token错误
        responseParams.insert(QString("info"),QString("not correct:access_token,please relogin!"));
        responseParams.insert(QString("result"),QString("fail"));

        return false;
    }
    return true;
}

//对接收到的消息，进行处理！
void UserMsgProcessor::responseOneMsg(const QyhMsgDateItem &item, QMap<QString, QString> requestDatas, QList<QMap<QString,QString> > datalists)
{
    if(requestDatas["type"] == "user"){
        clientMsgUserProcess(item,requestDatas,datalists);
    }else if(requestDatas["type"] == "map"){
        clientMsgMapProcess(item,requestDatas,datalists);
    }else if(requestDatas["type"] == "agv"){
        clientMsgAgvProcess(item,requestDatas,datalists);
    }else if(requestDatas["type"] == "agvManage"){
        clientMsgAgvManageProcess(item,requestDatas,datalists);
    }else if(requestDatas["type"] == "task"){
        clientMsgTaskProcess(item,requestDatas,datalists);
    }else if(requestDatas["type"] == "log"){
        clientMsgLogProcess(item,requestDatas,datalists);
    }
}

void UserMsgProcessor::clientMsgUserProcess(const QyhMsgDateItem &item,QMap<QString,QString> &requestDatas,QList<QMap<QString,QString> > &datalists)
{
    QMap<QString,QString> responseParams;
    QList<QMap<QString,QString> > responseDatalists;
    responseParams.insert(QString("type"),requestDatas["type"]);
    responseParams.insert(QString("todo"),requestDatas["todo"]);
    responseParams.insert(QString("queuenumber"),requestDatas["queuenumber"]);
    LoginUserInfo loginUserinfo;

    /////////////////////////////////////这段是和掐所有地方不应的一个点
    if(requestDatas["todo"]=="login")
    {
        User_Login(item,requestDatas,datalists,responseParams,responseDatalists,loginUserinfo);

        //封装        //发送
        QString xml = getResponseXml(responseParams,responseDatalists);
        g_netWork->sendToOne(item.sock,xml.toStdString().c_str(),xml.toStdString().length());

        return ;
    }
    /////////////////////////////////////这段是和掐所有地方不应的一个点

    /////所有的非登录消息，需要进行，安全验证 随机码
    if(!checkAccessToken(item,requestDatas,responseParams,loginUserinfo)){
        QString xml = getResponseXml(responseParams,responseDatalists);
        g_netWork->sendToOne(item.sock,xml.toStdString().c_str(),xml.toStdString().length());
        return ;
    }

    if(requestDatas["todo"]=="logout")
    {
        User_Logout(item,requestDatas,datalists,responseParams,responseDatalists,loginUserinfo);
    }
    else if(requestDatas["todo"]=="changepassword"){
        User_ChangePassword(item,requestDatas,datalists,responseParams,responseDatalists,loginUserinfo);
    }
    else if(requestDatas["todo"]=="list"){
        User_List(item,requestDatas,datalists,responseParams,responseDatalists,loginUserinfo);
    }
    else if(requestDatas["todo"]=="delete"){
        User_Delete(item,requestDatas,datalists,responseParams,responseDatalists,loginUserinfo);
    }
    else if(requestDatas["todo"]=="add"){
        User_Add(item,requestDatas,datalists,responseParams,responseDatalists,loginUserinfo);
    }
    else if(requestDatas["todo"]=="modify"){
        User_Modify(item,requestDatas,datalists,responseParams,responseDatalists,loginUserinfo);
    }

    //封装
    QString xml = getResponseXml(responseParams,responseDatalists);
    //发送
    g_netWork->sendToOne(item.sock,xml.toStdString().c_str(),xml.toStdString().length());
}

void UserMsgProcessor::clientMsgMapProcess(const QyhMsgDateItem &item,QMap<QString,QString> &requestDatas,QList<QMap<QString,QString> > &datalists)
{
    QMap<QString,QString> responseParams;
    QList<QMap<QString,QString> > responseDatalists;
    responseParams.insert(QString("type"),requestDatas["type"]);
    responseParams.insert(QString("todo"),requestDatas["todo"]);
    responseParams.insert(QString("queuenumber"),requestDatas["queuenumber"]);
    LoginUserInfo loginUserinfo;

    /////所有的非登录消息，需要进行，安全验证 随机码
    if(!checkAccessToken(item,requestDatas,responseParams,loginUserinfo)){
        QString xml = getResponseXml(responseParams,responseDatalists);
        g_netWork->sendToOne(item.sock,xml.toStdString().c_str(),xml.toStdString().length());
        return ;
    }

    /// 创建地图
    if(requestDatas["todo"]=="create"){
        Map_Create(item,requestDatas,datalists,responseParams,responseDatalists,loginUserinfo);
    }
    /// 站点列表
    if(requestDatas["todo"]=="stationlist"){
        Map_StationList(item,requestDatas,datalists,responseParams,responseDatalists,loginUserinfo);
    }

    /// 线路列表
    else if(requestDatas["todo"]=="linelist"){
        Map_LineList(item,requestDatas,datalists,responseParams,responseDatalists,loginUserinfo);
    }

    /// 订阅车辆位置信息
    else if(requestDatas["todo"]=="subscribe"){
        Map_AgvPositionSubscribe(item,requestDatas,datalists,responseParams,responseDatalists,loginUserinfo);
    }

    ///取消订阅车辆位置信息
    else if(requestDatas["todo"]=="cancelSubscribe"){
        Map_AgvPositionCancelSubscribe(item,requestDatas,datalists,responseParams,responseDatalists,loginUserinfo);
    }

    /// 获取背景图片
    if(requestDatas["todo"]=="getImage"){
        Map_GetImage(item,requestDatas,datalists,responseParams,responseDatalists,loginUserinfo);
    }

    /// 设置背景图片
    else if(requestDatas["todo"]=="setImage"){
        Map_SetImage(item,requestDatas,datalists,responseParams,responseDatalists,loginUserinfo);
    }

    //封装
    QString xml = getResponseXml(responseParams,responseDatalists);
    //发送
    g_netWork->sendToOne(item.sock,xml.toStdString().c_str(),xml.toStdString().length());

}

void UserMsgProcessor::clientMsgAgvProcess(const QyhMsgDateItem &item,QMap<QString,QString> &requestDatas,QList<QMap<QString,QString> > &datalists)
{
    QMap<QString,QString> responseParams;
    QList<QMap<QString,QString> > responseDatalists;
    responseParams.insert(QString("type"),requestDatas["type"]);
    responseParams.insert(QString("todo"),requestDatas["todo"]);
    responseParams.insert(QString("queuenumber"),requestDatas["queuenumber"]);
    LoginUserInfo loginUserinfo;

    /////所有的非登录消息，需要进行，安全验证 随机码
    if(!checkAccessToken(item,requestDatas,responseParams,loginUserinfo)){
        QString xml = getResponseXml(responseParams,responseDatalists);
        g_netWork->sendToOne(item.sock,xml.toStdString().c_str(),xml.toStdString().length());
        return ;
    }

    /// 请求控制权
    if(requestDatas["todo"]=="hand"){
        Agv_Hand(item,requestDatas,datalists,responseParams,responseDatalists,loginUserinfo);
    }
    /// 释放控制权
    else  if(requestDatas["todo"]=="release"){
        Agv_Release(item,requestDatas,datalists,responseParams,responseDatalists,loginUserinfo);
    }
    /// 小车前进
    else  if(requestDatas["todo"]=="forward"){
        Agv_Forward(item,requestDatas,datalists,responseParams,responseDatalists,loginUserinfo);
    }
    /// 小车后退
    else  if(requestDatas["todo"]=="backward"){
        Agv_Backward(item,requestDatas,datalists,responseParams,responseDatalists,loginUserinfo);
    }
    /// 小车左转
    else  if(requestDatas["todo"]=="turnleft"){
        Agv_Turnleft(item,requestDatas,datalists,responseParams,responseDatalists,loginUserinfo);
    }
    /// 小车右转
    else  if(requestDatas["todo"]=="turnright"){
        Agv_Turnright(item,requestDatas,datalists,responseParams,responseDatalists,loginUserinfo);
    }
    /// 小车灯带
    else  if(requestDatas["todo"]=="turnright"){
        Agv_Light(item,requestDatas,datalists,responseParams,responseDatalists,loginUserinfo);
    }
    /// TODO:因为不确定啥意思

    /// 订阅小车状态(只能订阅一辆车的状态信息)
    else  if(requestDatas["todo"]=="subscribe"){
        Agv_StatusSubscribte(item,requestDatas,datalists,responseParams,responseDatalists,loginUserinfo);
    }

    /// 取消订阅小车状态
    else  if(requestDatas["todo"]=="cancelSubscribe"){
        Agv_CancelStatusSubscribe(item,requestDatas,datalists,responseParams,responseDatalists,loginUserinfo);
    }

    //封装
    QString xml = getResponseXml(responseParams,responseDatalists);
    //发送
    g_netWork->sendToOne(item.sock,xml.toStdString().c_str(),xml.toStdString().length());

}

void UserMsgProcessor::clientMsgAgvManageProcess(const QyhMsgDateItem &item,QMap<QString,QString> &requestDatas,QList<QMap<QString,QString> > &datalists)
{
    QMap<QString,QString> responseParams;
    QList<QMap<QString,QString> > responseDatalists;
    responseParams.insert(QString("type"),requestDatas["type"]);
    responseParams.insert(QString("todo"),requestDatas["todo"]);
    responseParams.insert(QString("queuenumber"),requestDatas["queuenumber"]);
    LoginUserInfo loginUserinfo;

    /////所有的非登录消息，需要进行，安全验证 随机码
    if(!checkAccessToken(item,requestDatas,responseParams,loginUserinfo)){
        QString xml = getResponseXml(responseParams,responseDatalists);
        g_netWork->sendToOne(item.sock,xml.toStdString().c_str(),xml.toStdString().length());
        return ;
    }

    /// agv列表
    if(requestDatas["todo"]=="list"){
        AgvManage_List(item,requestDatas,datalists,responseParams,responseDatalists,loginUserinfo);
    }
    /// 增加agv
    else  if(requestDatas["todo"]=="add"){
        AgvManage_Add(item,requestDatas,datalists,responseParams,responseDatalists,loginUserinfo);
    }
    /// 删除agv
    else  if(requestDatas["todo"]=="delete"){
        AgvManage_Delete(item,requestDatas,datalists,responseParams,responseDatalists,loginUserinfo);
    }
    /// 修改agv
    else  if(requestDatas["todo"]=="modify"){
        AgvManage_Modify(item,requestDatas,datalists,responseParams,responseDatalists,loginUserinfo);
    }
    //封装
    QString xml = getResponseXml(responseParams,responseDatalists);
    //发送
    g_netWork->sendToOne(item.sock,xml.toStdString().c_str(),xml.toStdString().length());
}

void UserMsgProcessor::clientMsgTaskProcess(const QyhMsgDateItem &item,QMap<QString,QString> &requestDatas,QList<QMap<QString,QString> > &datalists)
{
    QMap<QString,QString> responseParams;
    QList<QMap<QString,QString> > responseDatalists;
    responseParams.insert(QString("type"),requestDatas["type"]);
    responseParams.insert(QString("todo"),requestDatas["todo"]);
    responseParams.insert(QString("queuenumber"),requestDatas["queuenumber"]);
    LoginUserInfo loginUserinfo;

    /////所有的非登录消息，需要进行，安全验证 随机码
    if(!checkAccessToken(item,requestDatas,responseParams,loginUserinfo)){
        QString xml = getResponseXml(responseParams,responseDatalists);
        g_netWork->sendToOne(item.sock,xml.toStdString().c_str(),xml.toStdString().length());
        return ;
    }

    if(requestDatas["todo"]=="subscribe"){
        Task_Subscribe(item,requestDatas,datalists,responseParams,responseDatalists,loginUserinfo);
    }

    else if(requestDatas["todo"]=="cancelSubscribe"){
        Task_CancelSubscribe(item,requestDatas,datalists,responseParams,responseDatalists,loginUserinfo);
    }

    /// 创建任务(创建到X点的任务)
    else if(requestDatas["todo"]=="toX"){
        Task_CreateToX(item,requestDatas,datalists,responseParams,responseDatalists,loginUserinfo);
    }
    /// 创建任务(创建指定车辆到X点的任务)
    else  if(requestDatas["todo"]=="agvToX"){
        Task_CreateAgvToX(item,requestDatas,datalists,responseParams,responseDatalists,loginUserinfo);
    }
    /// 创建任务(创建经过Y点到X点的任务)
    else  if(requestDatas["todo"]=="passYtoX"){
        Task_CreateYToX(item,requestDatas,datalists,responseParams,responseDatalists,loginUserinfo);
    }
    /// 创建任务(创建指定车辆经过Y点到X点的任务)
    else  if(requestDatas["todo"]=="agvPassYtoX"){
        Task_CreateAgvYToX(item,requestDatas,datalists,responseParams,responseDatalists,loginUserinfo);
    }
    /// 查询任务状态
    if(requestDatas["todo"]=="queryStatus"){
        Task_QueryStatus(item,requestDatas,datalists,responseParams,responseDatalists,loginUserinfo);
    }
    /// 取消任务
    else  if(requestDatas["todo"]=="cancel"){
        Task_Cancel(item,requestDatas,datalists,responseParams,responseDatalists,loginUserinfo);
    }
    /// 未分配任务列表
    else  if(requestDatas["todo"]=="listUndo"){
        Task_ListUnassigned(item,requestDatas,datalists,responseParams,responseDatalists,loginUserinfo);
    }
    /// 正在执行任务列表
    else  if(requestDatas["todo"]=="listDoing"){
        Task_ListDoing(item,requestDatas,datalists,responseParams,responseDatalists,loginUserinfo);
    }
    /// 已经完成任务列表(today)
    if(requestDatas["todo"]=="listDoneToday"){
        Task_ListDoneToday(item,requestDatas,datalists,responseParams,responseDatalists,loginUserinfo);
    }
    /// 已经完成任务列表(all)
    else  if(requestDatas["todo"]=="listDone"){
        Task_ListDoneAll(item,requestDatas,datalists,responseParams,responseDatalists,loginUserinfo);
    }
    /// 已经完成任务列表(from to 时间)
    else  if(requestDatas["todo"]=="listDuring"){
        Task_ListDoneDuring(item,requestDatas,datalists,responseParams,responseDatalists,loginUserinfo);
    }
    //封装
    QString xml = getResponseXml(responseParams,responseDatalists);
    //发送
    g_netWork->sendToOne(item.sock,xml.toStdString().c_str(),xml.toStdString().length());
}

void UserMsgProcessor::clientMsgLogProcess(const QyhMsgDateItem &item,QMap<QString,QString> &requestDatas,QList<QMap<QString,QString> > &datalists)
{
    QMap<QString,QString> responseParams;
    QList<QMap<QString,QString> > responseDatalists;
    responseParams.insert(QString("type"),requestDatas["type"]);
    responseParams.insert(QString("todo"),requestDatas["todo"]);
    responseParams.insert(QString("queuenumber"),requestDatas["queuenumber"]);
    LoginUserInfo loginUserinfo;

    /////所有的非登录消息，需要进行，安全验证 随机码
    if(!checkAccessToken(item,requestDatas,responseParams,loginUserinfo)){
        QString xml = getResponseXml(responseParams,responseDatalists);
        g_netWork->sendToOne(item.sock,xml.toStdString().c_str(),xml.toStdString().length());
        return ;
    }

    /// 创建任务(创建到X点的任务)
    if(requestDatas["todo"]=="listDuring"){
        Log_ListDuring(item,requestDatas,datalists,responseParams,responseDatalists,loginUserinfo);
    }
    /// 创建任务(创建指定车辆到X点的任务)
    else  if(requestDatas["todo"]=="listAll"){
        Log_ListAll(item,requestDatas,datalists,responseParams,responseDatalists,loginUserinfo);
    }
    /// 创建任务(创建经过Y点到X点的任务)
    else  if(requestDatas["todo"]=="subscribe"){
        Log_Subscribe(item,requestDatas,datalists,responseParams,responseDatalists,loginUserinfo);
    }
    /// 创建任务(创建指定车辆经过Y点到X点的任务)
    else  if(requestDatas["todo"]=="cancelSubscribe"){
        Log_CancelSubscribe(item,requestDatas,datalists,responseParams,responseDatalists,loginUserinfo);
    }
    //封装
    QString xml = getResponseXml(responseParams,responseDatalists);
    //发送
    g_netWork->sendToOne(item.sock,xml.toStdString().c_str(),xml.toStdString().length());

}

//接下来是具体的业务
/////////////////////////////关于用户部分
//用户登录
void UserMsgProcessor::User_Login(const QyhMsgDateItem &item, QMap<QString, QString> &requestDatas, QList<QMap<QString, QString> > &datalists,QMap<QString,QString> &responseParams,QList<QMap<QString,QString> > &responseDatalists,LoginUserInfo &loginUserInfo)
{
    //////////////////请求登录(做特殊处理，因为这里不需要验证access_token!)
    if(checkParamExistAndNotNull(requestDatas,responseParams,"username","password",NULL)){//要求包含用户名和密码
        //是否可以重复登录呢？？ //我觉得应该 不可以，那就添加
        QString querySqlA = "select id,user_password,user_role,user_signState from agv_user where user_username=?";
        QList<QVariant> params;
        params<<requestDatas["username"];
        QList<QList<QVariant> > queryresult = g_sql->query(querySqlA,params);
        if(queryresult.length()==0){
            responseParams.insert(QString("info"),QString("not exist:username"));
            responseParams.insert(QString("result"),QString("fail"));
        }else{
            if(queryresult.at(0).at(1) == requestDatas["password"]){
                //                if(queryresult.at(0).at(3).toInt() ==1){//已经登录了！
                //                    //用户已经登录了
                //                    responseParams.insert(QString("info"),QString("already login by other!"));
                //                    responseParams.insert(QString("result"),QString("fail"));
                //                }else{
                //设置登录状态和登录时间
                QString updateSql = "update agv_user set user_signState=1,user_lastSignTime= ? where id=? ";
                params.clear();
                params<<QDateTime::currentDateTime()<<queryresult.at(0).at(0).toInt();
                if(!g_sql->exeSql(updateSql,params)){
                    //登录失败
                    responseParams.insert(QString("info"),QString("update database fail!"));
                    responseParams.insert(QString("result"),QString("fail"));
                }else{
                    //加入已登录的队伍中
                    LoginUserInfo loginUserInfo;
                    loginUserInfo.id = queryresult.at(0).at(0).toInt();
                    loginUserInfo.sock = item.sock;
                    loginUserInfo.access_tocken = makeAccessToken();
                    loginUserInfo.password = requestDatas["username"];
                    loginUserInfo.username = requestDatas["password"];
                    loginUserInfo.role = queryresult.at(0).at(2).toInt();
                    loginUserIdSock.insert(item.sock,loginUserInfo);

                    //登录成功
                    responseParams.insert(QString("info"),QString(""));
                    responseParams.insert(QString("result"),QString("success"));
                    responseParams.insert(QString("role"),queryresult.at(0).at(2).toString());
                    responseParams.insert(QString("id"),queryresult.at(0).at(0).toString());
                    responseParams.insert(QString("access_token"),loginUserInfo.access_tocken);
                }
                //}
            }else{
                //登录失败
                responseParams.insert(QString("info"),QString("not correct:password"));
                responseParams.insert(QString("result"),QString("fail"));
            }
        }
    }
}

//用户登出
void UserMsgProcessor:: User_Logout(const QyhMsgDateItem &item, QMap<QString, QString> &requestDatas, QList<QMap<QString, QString> > &datalists,QMap<QString,QString> &responseParams,QList<QMap<QString,QString> > &responseDatalists,LoginUserInfo &loginUserInfo){
    //////////////////退出登录
    if(checkParamExistAndNotNull(requestDatas,responseParams,"id",NULL)){
        //取消的订阅
        g_msgCenter.removeAgvPositionSubscribe(item.sock);
        g_msgCenter.removeAgvStatusSubscribe(item.sock);
        //设置它的数据库中的状态
        QString updateSql = "update agv_user set user_signState = 0 where id=?";
        QList<QVariant> param;
        param<<requestDatas["id"];
        g_sql->exeSql(updateSql,param);
        responseParams.insert(QString("info"),QString(""));
        responseParams.insert(QString("result"),QString("success"));
    }
}

//修改密码
void UserMsgProcessor:: User_ChangePassword(const QyhMsgDateItem &item, QMap<QString, QString> &requestDatas, QList<QMap<QString, QString> > &datalists,QMap<QString,QString> &responseParams,QList<QMap<QString,QString> > &responseDatalists,LoginUserInfo &loginUserInfo){
    ///////////////////修改密码
    if(checkParamExistAndNotNull(requestDatas,responseParams,"username","oldpassword","newpassword",NULL)){
        QString querySqlA = "select user_password from agv_user where user_username=?";
        QList<QVariant> params;
        params<<requestDatas["username"];
        QList<QList<QVariant> > queryresult = g_sql->query(querySqlA,params);
        if(queryresult.length()==0){
            responseParams.insert(QString("info"),QString("not exist:username."));
            responseParams.insert(QString("result"),QString("fail"));
        }else{
            if(queryresult.at(0).at(0).toString()==requestDatas["oldpassword"]){
                /////TODO:设置新的密码
                QString updateSql = "update agv_user set user_password=? where user_username=?";
                params.clear();
                params<<requestDatas["newpassword"]<<requestDatas["username"];
                if(!g_sql->exeSql(updateSql,params)){
                    responseParams.insert(QString("info"),QString(""));
                    responseParams.insert(QString("result"),QString("success"));
                }else{
                    responseParams.insert(QString("info"),QString("sql exec fail"));
                    responseParams.insert(QString("result"),QString("fail"));
                }
            }else{
                responseParams.insert(QString("info"),QString("not correct:oldpassword."));
                responseParams.insert(QString("result"),QString("fail"));
            }
        }
    }

}
//用户列表
void UserMsgProcessor:: User_List(const QyhMsgDateItem &item, QMap<QString, QString> &requestDatas, QList<QMap<QString, QString> > &datalists,QMap<QString,QString> &responseParams,QList<QMap<QString,QString> > &responseDatalists,LoginUserInfo &loginUserInfo){
    ////////////////////////获取用户列表
    //开始查询用户 只能查看到同等级或者低等级的用户
    QString querySqlB = "select id,user_username,user_password,user_signState,user_lastSignTime,user_createTime,user_role,user_realname,user_age from agv_user where user_role<=?";
    QList<QVariant> paramsB;
    paramsB<<loginUserInfo.role;
    QList<QList<QVariant> > queryresultB = g_sql->query(querySqlB,paramsB);

    responseParams.insert(QString("info"),QString(""));
    responseParams.insert(QString("result"),QString("success"));

    for(int i=0;i<queryresultB.length();++i)
    {
        if(queryresultB.at(i).length() == 9)
        {
            QMap<QString,QString> userinfo;
            userinfo.insert(QString("id"),queryresultB.at(i).at(0).toString());
            userinfo.insert(QString("username"),queryresultB.at(i).at(1).toString());
            userinfo.insert(QString("password"),queryresultB.at(i).at(2).toString());
            userinfo.insert(QString("signState"),queryresultB.at(i).at(3).toString());
            QString sss = queryresultB.at(i).at(4).toString();
            sss = sss.replace("T"," ");
            userinfo.insert(QString("lastSignTime"),sss);
            sss = queryresultB.at(i).at(5).toString();
            sss = sss.replace("T"," ");
            userinfo.insert(QString("createTime"),sss);
            userinfo.insert(QString("role"),queryresultB.at(i).at(6).toString());
            userinfo.insert(QString("realname"),queryresultB.at(i).at(7).toString());
            userinfo.insert(QString("age"),queryresultB.at(i).at(8).toString());
            responseDatalists.push_back(userinfo);
        }
    }
}
//删除用户
void UserMsgProcessor:: User_Delete(const QyhMsgDateItem &item, QMap<QString, QString> &requestDatas, QList<QMap<QString, QString> > &datalists,QMap<QString,QString> &responseParams,QList<QMap<QString,QString> > &responseDatalists,LoginUserInfo &loginUserInfo){
    ////////////////////////////////删除用户
    if(checkParamExistAndNotNull(requestDatas,responseParams,"id",NULL)){
        QString deleteSql = "delete from agv_user where id=?";
        QList<QVariant> params;
        params<<requestDatas["id"];
        if(!g_sql->exeSql(deleteSql,params)){
            responseParams.insert(QString("info"),QString("delete fail for sql fail"));
            responseParams.insert(QString("result"),QString("fail"));
        }else{
            responseParams.insert(QString("info"),QString(""));
            responseParams.insert(QString("result"),QString("success"));
        }
    }
}

void UserMsgProcessor::User_Modify(const QyhMsgDateItem &item, QMap<QString, QString> &requestDatas, QList<QMap<QString, QString> > &datalists,QMap<QString,QString> &responseParams,QList<QMap<QString,QString> > &responseDatalists,LoginUserInfo &loginUserInfo)
{
    ///////////////////////////////////修改
    if(checkParamExistAndNotNull(requestDatas,responseParams,"username","password","id",NULL))
    {
        QString username = requestDatas["username"];
        QString password = requestDatas["password"];

        QString updateSql = "update agv_user set user_username=?,user_password=?,";
        QString updateSqlTail = " where id=?";
        QList<QVariant> params;
        params.append(username);
        params.append(password);
        if(requestDatas.contains("realname")){
            updateSql +="user_realname=?,";
            params.append(requestDatas["realname"]);
        }
        if(requestDatas.contains("sex")){
            updateSql +="user_sex=?,";
            params.append(requestDatas["sex"]);
        }
        if(requestDatas.contains("age")){
            updateSql +="user_age=?,";
            params.append(requestDatas["age"]);
        }
        if(requestDatas.contains("role")){
            updateSql +="user_role=?,";
            params.append(requestDatas["role"]);
        }

        //去掉逗号
        updateSql = updateSql.left(updateSql.lastIndexOf(","));

        updateSql.append(updateSqlTail);
        params.append(requestDatas["id"]);

        if(g_sql->exeSql(updateSql,params)){
            //成功
            responseParams.insert(QString("info"),QString(""));
            responseParams.insert(QString("result"),QString("success"));
        }else{
            //失败
            responseParams.insert(QString("info"),QString("sql update fail"));
            responseParams.insert(QString("result"),QString("fail"));
        }
    }

}
//添加用户
void UserMsgProcessor:: User_Add(const QyhMsgDateItem &item, QMap<QString, QString> &requestDatas, QList<QMap<QString, QString> > &datalists,QMap<QString,QString> &responseParams,QList<QMap<QString,QString> > &responseDatalists,LoginUserInfo &loginUserInfo){
    ///////////////////////////////////添加用户
    if(checkParamExistAndNotNull(requestDatas,responseParams,"username","password","role",NULL))
    {
        QString username = requestDatas["username"];
        QString password = requestDatas["password"];
        QString role = requestDatas["role"];

        //查看剩余项目是否存在
        QString realname="";
        bool sex = true;
        int age=20;
        if(requestDatas.contains("realname")){
            realname = requestDatas["realname"];
        }
        if(requestDatas.contains("sex")){
            sex = requestDatas["sex"]=="1";
        }
        if(requestDatas.contains("age")){
            age = requestDatas["age"].toInt();
        }

        QString addSql = "insert into agv_user(user_username,user_password,user_role,user_realname,user_sex,user_age,user_createTime,user_signState)values(?,?,?,?,?,?,?,?);";
        QList<QVariant> params;
        params<<username<<password<<role<<realname<<sex<<age<<QDateTime::currentDateTime()<<0;
        if(g_sql->exeSql(addSql,params)){
            //成功
            responseParams.insert(QString("info"),QString(""));
            responseParams.insert(QString("result"),QString("success"));
        }else{
            //失败
            responseParams.insert(QString("info"),QString("sql insert into fail"));
            responseParams.insert(QString("result"),QString("fail"));
        }
    }

}

/////////////////////////////关于地图部分
//创建地图
void UserMsgProcessor::Map_Create(const QyhMsgDateItem &item, QMap<QString, QString> &requestDatas, QList<QMap<QString, QString> > &datalists,QMap<QString,QString> &responseParams,QList<QMap<QString,QString> > &responseDatalists,LoginUserInfo &loginUserInfo)
{
    if(checkParamExistAndNotNull(requestDatas,responseParams,"line","arc","station",NULL))
    {
        responseParams.insert(QString("info"),QString(""));
        responseParams.insert(QString("result"),QString("success"));
        g_agvMapCenter.resetMap(requestDatas["station"],requestDatas["line"],requestDatas["arc"],requestDatas["image"]);
    }
}

//地图 站点列表
void UserMsgProcessor::Map_StationList(const QyhMsgDateItem &item, QMap<QString, QString> &requestDatas, QList<QMap<QString, QString> > &datalists,QMap<QString,QString> &responseParams,QList<QMap<QString,QString> > &responseDatalists,LoginUserInfo &loginUserInfo){
    responseParams.insert(QString("info"),QString(""));
    responseParams.insert(QString("result"),QString("success"));
    for(QMap<int,AgvStation *>::iterator itr=g_m_stations.begin();itr!=g_m_stations.end();++itr){
        QMap<QString,QString> list;

        list.insert(QString("x"),QString("%1").arg(itr.value()->x));
        list.insert(QString("y"),QString("%1").arg(itr.value()->y));
        list.insert(QString("name"),QString("%1").arg(itr.value()->name));
        list.insert(QString("id"),QString("%1").arg(itr.value()->id));
        list.insert(QString("rfid"),QString("%1").arg(itr.value()->rfid));
        list.insert(QString("color_r"),QString("%1").arg(itr.value()->color_r));
        list.insert(QString("color_g"),QString("%1").arg(itr.value()->color_g));
        list.insert(QString("color_b"),QString("%1").arg(itr.value()->color_b));

        responseDatalists.push_back(list);
    }
}
//地图 线路列表
void UserMsgProcessor:: Map_LineList(const QyhMsgDateItem &item, QMap<QString, QString> &requestDatas, QList<QMap<QString, QString> > &datalists,QMap<QString,QString> &responseParams,QList<QMap<QString,QString> > &responseDatalists,LoginUserInfo &loginUserInfo){
    responseParams.insert(QString("info"),QString(""));
    responseParams.insert(QString("result"),QString("success"));
    for(QMap<int,AgvLine *>::iterator itr=g_m_lines.begin();itr!=g_m_lines.end();++itr){
        QMap<QString,QString> list;

        list.insert(QString("startStation"),QString("%1").arg(itr.value()->startStation));
        list.insert(QString("endStation"),QString("%1").arg(itr.value()->endStation));
        list.insert(QString("color_r"),QString("%1").arg(itr.value()->color_r));
        list.insert(QString("color_g"),QString("%1").arg(itr.value()->color_g));
        list.insert(QString("color_b"),QString("%1").arg(itr.value()->color_b));
        list.insert(QString("line"),QString("%1").arg(itr.value()->line));
        list.insert(QString("id"),QString("%1").arg(itr.value()->id));
        list.insert(QString("draw"),QString("%1").arg(itr.value()->draw));
        list.insert(QString("length"),QString("%1").arg(itr.value()->length));
        list.insert(QString("rate"),QString("%1").arg(itr.value()->rate));
        list.insert(QString("occuAgv"),QString("%1").arg(itr.value()->occuAgv));

        list.insert(QString("p1x"),QString("%1").arg(itr.value()->p1x));
        list.insert(QString("p1y"),QString("%1").arg(itr.value()->p1y));
        list.insert(QString("p2x"),QString("%1").arg(itr.value()->p2x));
        list.insert(QString("p2y"),QString("%1").arg(itr.value()->p2y));
        responseDatalists.push_back(list);
    }

}
//订阅车辆位置信息
void UserMsgProcessor:: Map_AgvPositionSubscribe(const QyhMsgDateItem &item, QMap<QString, QString> &requestDatas, QList<QMap<QString, QString> > &datalists,QMap<QString,QString> &responseParams,QList<QMap<QString,QString> > &responseDatalists,LoginUserInfo &loginUserInfo){
    //将sock加入到车辆位置订阅者丢列中
    if(g_msgCenter.addAgvPostionSubscribe(item.sock)){
        responseParams.insert(QString("info"),QString(""));
        responseParams.insert(QString("result"),QString("success"));
    }else{
        responseParams.insert(QString("info"),QString("unknow error"));
        responseParams.insert(QString("result"),QString("fail"));
    }

}
//取消订阅车辆位置信息
void UserMsgProcessor:: Map_AgvPositionCancelSubscribe(const QyhMsgDateItem &item, QMap<QString, QString> &requestDatas, QList<QMap<QString, QString> > &datalists,QMap<QString,QString> &responseParams,QList<QMap<QString,QString> > &responseDatalists,LoginUserInfo &loginUserInfo){
    //将sock加入到车辆位置订阅者丢列中
    if(g_msgCenter.removeAgvPositionSubscribe(item.sock)){
        responseParams.insert(QString("info"),QString(""));
        responseParams.insert(QString("result"),QString("success"));
    }else{
        responseParams.insert(QString("info"),QString("unknow error"));
        responseParams.insert(QString("result"),QString("fail"));
    }
}

//获取地图背景
void UserMsgProcessor::Map_GetImage(const QyhMsgDateItem &item, QMap<QString, QString> &requestDatas, QList<QMap<QString, QString> > &datalists,QMap<QString,QString> &responseParams,QList<QMap<QString,QString> > &responseDatalists,LoginUserInfo &loginUserInfo)
{
    if(checkParamExistAndNotNull(requestDatas,responseParams,"ip","port",NULL))
    {
        QString file_name = g_msgCenter.getDownloadFileName();
        long file_length = g_msgCenter.getDownloadFileLength();
        if(file_name.length()>0 && file_length>0)
        {
            responseParams.insert(QString("result"),QString("success"));
            responseParams.insert(QString("info"),QString(""));
            responseParams.insert(QString("file_name"),file_name);
            responseParams.insert(QString("file_length"),QString("%1").arg(file_length));
            g_msgCenter.readyDownloadFile(requestDatas["ip"].toStdString(),requestDatas["port"].toInt());
        }
        else{
            responseParams.insert(QString("result"),QString("fail"));
            responseParams.insert(QString("info"),QString("no background image"));
        }
    }
}

//设置地图背景
void UserMsgProcessor::Map_SetImage(const QyhMsgDateItem &item, QMap<QString, QString> &requestDatas, QList<QMap<QString, QString> > &datalists,QMap<QString,QString> &responseParams,QList<QMap<QString,QString> > &responseDatalists,LoginUserInfo &loginUserInfo)
{
    if(checkParamExistAndNotNull(requestDatas,responseParams,"ip","port","file_name","file_length",NULL))
    {
        g_msgCenter.readyToUpload(requestDatas["ip"].toStdString(),requestDatas["port"].toInt(),requestDatas["file_name"],requestDatas["file_length"].toInt());
        responseParams.insert(QString("result"),QString("success"));
        responseParams.insert(QString("info"),QString(""));
    }
}

/////////////////////////////关于手控部分
//请求小车控制权
void UserMsgProcessor:: Agv_Hand(const QyhMsgDateItem &item, QMap<QString, QString> &requestDatas, QList<QMap<QString, QString> > &datalists,QMap<QString,QString> &responseParams,QList<QMap<QString,QString> > &responseDatalists,LoginUserInfo &loginUserInfo){
    ////该用户要求控制id为agvid的小车，
    if(checkParamExistAndNotNull(requestDatas,responseParams,"agvid",NULL)){
        int iAgvId = requestDatas["agvid"].toInt();
        //查找这辆车
        if(!g_m_agvs.contains(iAgvId)){
            //不存在这辆车
            responseParams.insert(QString("info"),QString("not found agv with id:")+requestDatas["agvid"]);
            responseParams.insert(QString("result"),QString("fail"));
        }else{
            Agv *agv = g_m_agvs[iAgvId];
            if(agv->currentHandUser == loginUserInfo.id||agv->currentHandUser <= 0){
                //OK
                agv->currentHandUser = loginUserInfo.id;
                agv->currentHandUserRole = loginUserInfo.role;
                responseParams.insert(QString("info"),QString(""));
                responseParams.insert(QString("result"),QString("success"));
            }else{
                //判断两个用户的权限，如果申请的人更高，OK。如果没有更高的权限，失败
                if(agv->currentHandUserRole<loginUserInfo.role){
                    //新用户权限更高
                    agv->currentHandUser = loginUserInfo.id;
                    agv->currentHandUserRole = loginUserInfo.role;
                    responseParams.insert(QString("info"),QString(""));
                    responseParams.insert(QString("result"),QString("success"));
                }else{
                    //新用户权限并不高//旧用户继续占用这辆车的手动控制权
                    responseParams.insert(QString("info"),QString("agv already handed by other user"));
                    responseParams.insert(QString("result"),QString("fail"));
                }
            }
        }
    }
}
//释放小车控制权
void UserMsgProcessor:: Agv_Release(const QyhMsgDateItem &item, QMap<QString, QString> &requestDatas, QList<QMap<QString, QString> > &datalists,QMap<QString,QString> &responseParams,QList<QMap<QString,QString> > &responseDatalists,LoginUserInfo &loginUserInfo){
    ////该用户要求控制id为agvid的小车，
    if(checkParamExistAndNotNull(requestDatas,responseParams,"agvid",NULL)){

        int iAgvId = requestDatas["agvid"].toInt();
        //查找这辆车
        if(!g_m_agvs.contains(iAgvId)){
            //不存在这辆车
            responseParams.insert(QString("info"),QString("not found agv with id:")+requestDatas["agvid"]);
            responseParams.insert(QString("result"),QString("fail"));
        }else{
            Agv *agv = g_m_agvs[iAgvId];
            if(agv->currentHandUser == loginUserInfo.id){
                //OK
                agv->currentHandUser=0;
                agv->currentHandUserRole=0;
                responseParams.insert(QString("info"),QString(""));
                responseParams.insert(QString("result"),QString("success"));
            }else{
                //这辆车并不受你控制
                responseParams.insert(QString("info"),QString("agv is not under your control"));
                responseParams.insert(QString("result"),QString("fail"));
            }
        }
    }
}
//前进
void UserMsgProcessor:: Agv_Forward(const QyhMsgDateItem &item, QMap<QString, QString> &requestDatas, QList<QMap<QString, QString> > &datalists,QMap<QString,QString> &responseParams,QList<QMap<QString,QString> > &responseDatalists,LoginUserInfo &loginUserInfo){
    ////该用户要求控制id为agvid的小车，
    if(checkParamExistAndNotNull(requestDatas,responseParams,"agvid","speed",NULL)){
        int iAgvId = requestDatas["agvid"].toInt();
        int iSpeed = requestDatas["speed"].toInt();
        //查找这辆车

        if(!g_m_agvs.contains(iAgvId)){
            //不存在这辆车
            responseParams.insert(QString("info"),QString("not found agv with id:")+requestDatas["agvid"]);
            responseParams.insert(QString("result"),QString("fail"));
        }else
        {
            Agv *agv = g_m_agvs[iAgvId];
            if(agv->currentHandUser == loginUserInfo.id){
                //OK
                //下发指令
                g_hrgAgvCenter.handControlCmd(iAgvId,AGV_HAND_TYPE_FORWARD,iSpeed);
            }else{
                //这辆车并不受你控制
                responseParams.insert(QString("info"),QString("agv is not under your control"));
                responseParams.insert(QString("result"),QString("fail"));
            }
        }
    }
}

//后退
void UserMsgProcessor:: Agv_Backward(const QyhMsgDateItem &item, QMap<QString, QString> &requestDatas, QList<QMap<QString, QString> > &datalists,QMap<QString,QString> &responseParams,QList<QMap<QString,QString> > &responseDatalists,LoginUserInfo &loginUserInfo){
    ////该用户要求控制id为agvid的小车，
    if(checkParamExistAndNotNull(requestDatas,responseParams,"agvid","speed",NULL)){
        int iAgvId = requestDatas["agvid"].toInt();
        int iSpeed = requestDatas["speed"].toInt();
        //查找这辆车
        if(!g_m_agvs.contains(iAgvId)){
            //不存在这辆车
            responseParams.insert(QString("info"),QString("not found agv with id:")+requestDatas["agvid"]);
            responseParams.insert(QString("result"),QString("fail"));
        }else
        {
            Agv *agv = g_m_agvs[iAgvId];
            if(agv->currentHandUser == loginUserInfo.id){
                //OK
                //下发指令
                g_hrgAgvCenter.handControlCmd(iAgvId,AGV_HAND_TYPE_BACKWARD,iSpeed);
            }else{
                //这辆车并不受你控制
                responseParams.insert(QString("info"),QString("agv is not in your hand"));
                responseParams.insert(QString("result"),QString("fail"));
            }
        }
    }
}
//左转
void UserMsgProcessor:: Agv_Turnleft(const QyhMsgDateItem &item, QMap<QString, QString> &requestDatas, QList<QMap<QString, QString> > &datalists,QMap<QString,QString> &responseParams,QList<QMap<QString,QString> > &responseDatalists,LoginUserInfo &loginUserInfo){
    ////该用户要求控制id为agvid的小车，
    if(checkParamExistAndNotNull(requestDatas,responseParams,"agvid","speed",NULL)){
        int iAgvId = requestDatas["agvid"].toInt();
        int iSpeed = requestDatas["speed"].toInt();

        if(!g_m_agvs.contains(iAgvId)){
            //不存在这辆车
            responseParams.insert(QString("info"),QString("not found agv with id:")+requestDatas["agvid"]);
            responseParams.insert(QString("result"),QString("fail"));
        }else
        {
            Agv *agv = g_m_agvs[iAgvId];
            if(agv->currentHandUser == loginUserInfo.id){
                //OK
                //下发指令
                g_hrgAgvCenter.handControlCmd(iAgvId,AGV_HAND_TYPE_TURNLEFT,iSpeed);
            }else{
                //这辆车并不受你控制
                responseParams.insert(QString("info"),QString("agv is not in your hand"));
                responseParams.insert(QString("result"),QString("fail"));
            }
        }
    }
}
//右转
void UserMsgProcessor:: Agv_Turnright(const QyhMsgDateItem &item, QMap<QString, QString> &requestDatas, QList<QMap<QString, QString> > &datalists,QMap<QString,QString> &responseParams,QList<QMap<QString,QString> > &responseDatalists,LoginUserInfo &loginUserInfo){
    ////该用户要求控制id为agvid的小车，
    if(checkParamExistAndNotNull(requestDatas,responseParams,"agvid","speed",NULL)){
        int iAgvId = requestDatas["agvid"].toInt();
        int iSpeed = requestDatas["speed"].toInt();

        if(!g_m_agvs.contains(iAgvId)){
            //不存在这辆车
            responseParams.insert(QString("info"),QString("not found agv with id:")+requestDatas["agvid"]);
            responseParams.insert(QString("result"),QString("fail"));
        }else
        {
            Agv *agv = g_m_agvs[iAgvId];
            if(agv->currentHandUser == loginUserInfo.id){
                //OK
                //下发指令
                g_hrgAgvCenter.handControlCmd(iAgvId,AGV_HAND_TYPE_TURNRIGHT,iSpeed);
            }else{
                //这辆车并不受你控制
                responseParams.insert(QString("info"),QString("agv is not in your hand"));
                responseParams.insert(QString("result"),QString("fail"));
            }
        }
    }
}
//灯带
void UserMsgProcessor:: Agv_Light(const QyhMsgDateItem &item, QMap<QString, QString> &requestDatas, QList<QMap<QString, QString> > &datalists,QMap<QString,QString> &responseParams,QList<QMap<QString,QString> > &responseDatalists,LoginUserInfo &loginUserInfo){}
//小车状态订阅
void UserMsgProcessor:: Agv_StatusSubscribte(const QyhMsgDateItem &item, QMap<QString, QString> &requestDatas, QList<QMap<QString, QString> > &datalists,QMap<QString,QString> &responseParams,QList<QMap<QString,QString> > &responseDatalists,LoginUserInfo &loginUserInfo){
    //OK
    //下发指令
    g_msgCenter.addAgvStatusSubscribe(item.sock);
    responseParams.insert(QString("info"),QString(""));
    responseParams.insert(QString("result"),QString("success"));

}

//取消小车状态订阅
void UserMsgProcessor:: Agv_CancelStatusSubscribe(const QyhMsgDateItem &item, QMap<QString, QString> &requestDatas, QList<QMap<QString, QString> > &datalists,QMap<QString,QString> &responseParams,QList<QMap<QString,QString> > &responseDatalists,LoginUserInfo &loginUserInfo)
{
    //OK
    //下发指令
    g_msgCenter.removeAgvStatusSubscribe(item.sock);
    responseParams.insert(QString("info"),QString(""));
    responseParams.insert(QString("result"),QString("success"));
}

/////////////////////////////////车辆管理部分
//列表
void UserMsgProcessor:: AgvManage_List(const QyhMsgDateItem &item, QMap<QString, QString> &requestDatas, QList<QMap<QString, QString> > &datalists,QMap<QString,QString> &responseParams,QList<QMap<QString,QString> > &responseDatalists,LoginUserInfo &loginUserInfo){
    responseParams.insert(QString("info"),QString(""));
    responseParams.insert(QString("result"),QString("success"));
    for(QMap<int,Agv *>::iterator itr = g_m_agvs.begin();itr!=g_m_agvs.end();++itr){
        QMap<QString,QString> list;
        Agv *agv = itr.value();

        list.insert(QString("id"),QString("%1").arg(agv->id));
        list.insert(QString("name"),QString("%1").arg(agv->name));

        responseDatalists.push_back(list);
    }
}
//增加
void UserMsgProcessor:: AgvManage_Add(const QyhMsgDateItem &item, QMap<QString, QString> &requestDatas, QList<QMap<QString, QString> > &datalists,QMap<QString,QString> &responseParams,QList<QMap<QString,QString> > &responseDatalists,LoginUserInfo &loginUserInfo){
    //要求name和ip
    if(checkParamExistAndNotNull(requestDatas,responseParams,"name","ip",NULL)){
        //TODO //TODO
        QString insertSql = "insert into agv_agv (agv_name)values(?)";
        QList<QVariant> tempParams;
        tempParams<<requestDatas["name"]<<requestDatas["ip"];
        if(g_sql->exeSql(insertSql,tempParams)){
            int newId;
            QString querySql = "select id from agv_agv where agv_name = ? ";
            QList<QList<QVariant>> queryresult = g_sql->query(querySql,tempParams);
            if(queryresult.length()>0 &&queryresult.at(0).length()>0)
            {
                newId = queryresult.at(0).at(0).toInt();
                Agv *agv = new Agv;
                agv->id = (newId);
                agv->name = (requestDatas["name"]);
                //agv->setIp(requestDatas["ip"]);
                g_m_agvs.insert(newId,agv);
                responseParams.insert(QString("info"),QString(""));
                responseParams.insert(QString("result"),QString("success"));
                responseParams.insert(QString("id"),queryresult.at(0).at(0).toString());
            }else{
                responseParams.insert(QString("info"),QString("sql insert fail"));
                responseParams.insert(QString("result"),QString("fail"));
            }
        }else{
            responseParams.insert(QString("info"),QString("sql insert fail"));
            responseParams.insert(QString("result"),QString("fail"));
        }
    }

}
//删除
void UserMsgProcessor:: AgvManage_Delete(const QyhMsgDateItem &item, QMap<QString, QString> &requestDatas, QList<QMap<QString, QString> > &datalists,QMap<QString,QString> &responseParams,QList<QMap<QString,QString> > &responseDatalists,LoginUserInfo &loginUserInfo){
    //要求agvid
    if(checkParamExistAndNotNull(requestDatas,responseParams,"agvid",NULL)){
        int iAgvId = requestDatas["agvid"].toInt();

        //查找是否存在
        if(g_m_agvs.contains(iAgvId)){
            //从数据库中清除
            QString deleteSql = "delete from agv_agv where id=?";
            QList<QVariant> tempParams;
            tempParams<<iAgvId;
            if(g_sql->exeSql(deleteSql,tempParams))
            {
                //从列表中清楚
                if(g_m_agvs.contains(iAgvId)){
                    g_m_agvs.remove(iAgvId);
                }
                responseParams.insert(QString("info"),QString(""));
                responseParams.insert(QString("result"),QString("success"));
            }else{
                responseParams.insert(QString("info"),QString("delete sql exec fail"));
                responseParams.insert(QString("result"),QString("fail"));
            }
        }else{
            responseParams.insert(QString("info"),QString("not exist of this agvid."));
            responseParams.insert(QString("result"),QString("fail"));
        }
    }
}
//修改
void UserMsgProcessor:: AgvManage_Modify(const QyhMsgDateItem &item, QMap<QString, QString> &requestDatas, QList<QMap<QString, QString> > &datalists,QMap<QString,QString> &responseParams,QList<QMap<QString,QString> > &responseDatalists,LoginUserInfo &loginUserInfo){
    if(checkParamExistAndNotNull(requestDatas,responseParams,"agvid","name","ip",NULL)){
        int iAgvId = requestDatas["agvid"].toInt();

        if(!g_m_agvs.contains(iAgvId)){
            //不存在这辆车
            responseParams.insert(QString("info"),QString("not exist of this agvid."));
            responseParams.insert(QString("result"),QString("fail"));
        }else{
            Agv *agv = g_m_agvs[iAgvId];
            QString updateSql = "update agv_agv set agv_name=?,agv_ip=? where id=?";
            QList<QVariant> params;
            params<<(requestDatas["name"])<<(requestDatas["ip"])<<(requestDatas["agvid"]);
            if(g_sql->exeSql(updateSql,params)){
                agv->name = (requestDatas["name"]);
                //agv->setIp(requestDatas["ip"]);
                responseParams.insert(QString("info"),QString(""));
                responseParams.insert(QString("result"),QString("success"));
            }else{
                responseParams.insert(QString("info"),QString("sql update exec fail"));
                responseParams.insert(QString("result"),QString("fail"));
            }
        }
    }
}


////////////////////////////////任务部分

//订阅任务状态信息（简单显示）
void UserMsgProcessor::Task_Subscribe(const QyhMsgDateItem &item, QMap<QString, QString> &requestDatas, QList<QMap<QString, QString> > &datalists,QMap<QString,QString> &responseParams,QList<QMap<QString,QString> > &responseDatalists,LoginUserInfo &loginUserInfo)
{
    //将sock加入到车辆位置订阅者丢列中
    if(g_msgCenter.addAgvTaskSubscribe(item.sock)){
        responseParams.insert(QString("info"),QString(""));
        responseParams.insert(QString("result"),QString("success"));
    }else{
        responseParams.insert(QString("info"),QString("unknow error"));
        responseParams.insert(QString("result"),QString("fail"));
    }
}


//取消任务状态信息订阅
void UserMsgProcessor::Task_CancelSubscribe(const QyhMsgDateItem &item, QMap<QString, QString> &requestDatas, QList<QMap<QString, QString> > &datalists,QMap<QString,QString> &responseParams,QList<QMap<QString,QString> > &responseDatalists,LoginUserInfo &loginUserInfo)
{
    //将sock加入到车辆位置订阅者丢列中
    if(g_msgCenter.removeAgvTaskSubscribe(item.sock)){
        responseParams.insert(QString("info"),QString(""));
        responseParams.insert(QString("result"),QString("success"));
    }else{
        responseParams.insert(QString("info"),QString("unknow error"));
        responseParams.insert(QString("result"),QString("fail"));
    }
}

//创建任务(创建到X点的任务)
void UserMsgProcessor::Task_CreateToX(const QyhMsgDateItem &item, QMap<QString, QString> &requestDatas, QList<QMap<QString, QString> > &datalists,QMap<QString,QString> &responseParams,QList<QMap<QString,QString> > &responseDatalists,LoginUserInfo &loginUserInfo){
    if(checkParamExistAndNotNull(requestDatas,responseParams,"x",NULL)){
        //可选项<xWaitType><xWaitTime><yWaitType><yWaitTime>
        int iX = requestDatas["x"].toInt();

        //确保站点存在
        int waitTypeX = AGV_TASK_WAIT_TYPE_TIME;
        int watiTimeX = 30;

        //判断是否设置了等待时间和等待时长
        if(requestDatas.contains("xWaitType"))waitTypeX=requestDatas["xWaitType"].toInt();
        if(requestDatas.contains("xWaitTime"))watiTimeX=requestDatas["xWaitTime"].toInt();

        if(!g_m_stations.contains(iX)){
            responseParams.insert(QString("result"),QString("fail"));
            responseParams.insert(QString("info"),QString("not found station"));
        }else{
            int id = g_taskCenter.makeAimTask(iX,waitTypeX,watiTimeX);
            responseParams.insert(QString("info"),QString(""));
            responseParams.insert(QString("result"),QString("success"));
            responseParams.insert(QString("id"),QString("%1").arg(id));
        }
    }
}

//创建任务(创建指定车辆到X点的任务)
void UserMsgProcessor::Task_CreateAgvToX(const QyhMsgDateItem &item, QMap<QString, QString> &requestDatas, QList<QMap<QString, QString> > &datalists,QMap<QString,QString> &responseParams,QList<QMap<QString,QString> > &responseDatalists,LoginUserInfo &loginUserInfo){
    if(checkParamExistAndNotNull(requestDatas,responseParams,"x","agvid",NULL)){
        int iX = requestDatas["x"].toInt();
        int iAgvid = requestDatas["agvid"].toInt();
        //确保站点存在
        int waitTypeX = AGV_TASK_WAIT_TYPE_TIME;
        int watiTimeX = 30;

        //判断是否设置了等待时间和等待时长
        if(requestDatas.contains("xWaitType"))waitTypeX=requestDatas["xWaitType"].toInt();
        if(requestDatas.contains("xWaitTime"))watiTimeX=requestDatas["xWaitTime"].toInt();

        if(!g_m_stations.contains(iX)){
            responseParams.insert(QString("result"),QString("fail"));
            responseParams.insert(QString("info"),QString("not found station"));
        }else if(!g_m_agvs.contains(iAgvid)){
            responseParams.insert(QString("result"),QString("fail"));
            responseParams.insert(QString("info"),QString("not found agv"));
        }else{
            int id = g_taskCenter.makeAgvAimTask(iAgvid,iX,waitTypeX,watiTimeX);
            responseParams.insert(QString("info"),QString(""));
            responseParams.insert(QString("result"),QString("success"));
            responseParams.insert(QString("id"),QString("%1").arg(id));
        }
    }
}
//创建任务(创建经过Y点到X点的任务)
void UserMsgProcessor::Task_CreateYToX(const QyhMsgDateItem &item, QMap<QString, QString> &requestDatas, QList<QMap<QString, QString> > &datalists,QMap<QString,QString> &responseParams,QList<QMap<QString,QString> > &responseDatalists,LoginUserInfo &loginUserInfo){
    if(checkParamExistAndNotNull(requestDatas,responseParams,"x","y",NULL)){
        int iX = requestDatas["x"].toInt();
        int iY = requestDatas["y"].toInt();

        int waitTypeX = AGV_TASK_WAIT_TYPE_TIME;
        int watiTimeX = 30;
        int waitTypeY = AGV_TASK_WAIT_TYPE_TIME;
        int watiTimeY = 30;

        //判断是否设置了等待时间和等待时长
        if(requestDatas.contains("xWaitType"))waitTypeX=requestDatas["xWaitType"].toInt();
        if(requestDatas.contains("xWaitTime"))watiTimeX=requestDatas["xWaitTime"].toInt();
        if(requestDatas.contains("yWaitType"))waitTypeY=requestDatas["yWaitType"].toInt();
        if(requestDatas.contains("yWaitTime"))watiTimeY=requestDatas["yWaitTime"].toInt();

        //确保站点存在
        if(!g_m_stations.contains(iX)){
            responseParams.insert(QString("result"),QString("fail"));
            responseParams.insert(QString("info"),QString("not found station x"));
        }else if(!g_m_stations.contains(iY)){
            responseParams.insert(QString("result"),QString("fail"));
            responseParams.insert(QString("info"),QString("not found station y"));
        }else{
            int id = g_taskCenter.makePickupTask(iX,iY,waitTypeX,watiTimeX,waitTypeY,watiTimeY);
            responseParams.insert(QString("info"),QString(""));
            responseParams.insert(QString("result"),QString("success"));
            responseParams.insert(QString("id"),QString("%1").arg(id));
        }
    }
}
//创建任务(创建指定车辆经过Y点到X点的任务)
void UserMsgProcessor::Task_CreateAgvYToX(const QyhMsgDateItem &item, QMap<QString, QString> &requestDatas, QList<QMap<QString, QString> > &datalists,QMap<QString,QString> &responseParams,QList<QMap<QString,QString> > &responseDatalists,LoginUserInfo &loginUserInfo){

    if(checkParamExistAndNotNull(requestDatas,responseParams,"x","y","agvid",NULL)){
        int iX = requestDatas["x"].toInt();
        int iY = requestDatas["y"].toInt();
        int agvId = requestDatas["agvid"].toInt();

        int waitTypeX = AGV_TASK_WAIT_TYPE_NOWAIT;
        int watiTimeX = 30;
        int waitTypeY = AGV_TASK_WAIT_TYPE_NOWAIT;
        int watiTimeY = 30;

        //判断是否设置了等待时间和等待时长
        if(requestDatas.contains("xWaitType"))waitTypeX=requestDatas["xWaitType"].toInt();
        if(requestDatas.contains("xWaitTime"))watiTimeX=requestDatas["xWaitTime"].toInt();
        if(requestDatas.contains("yWaitType"))waitTypeY=requestDatas["yWaitType"].toInt();
        if(requestDatas.contains("yWaitTime"))watiTimeY=requestDatas["yWaitTime"].toInt();

        //确保站点存在
        if(!g_m_stations.contains(iX)){
            responseParams.insert(QString("result"),QString("fail"));
            responseParams.insert(QString("info"),QString("not found station x"));
        }else if(!g_m_stations.contains(iY)){
            responseParams.insert(QString("result"),QString("fail"));
            responseParams.insert(QString("info"),QString("not found station y"));
        }else if(!g_m_agvs.contains(agvId)){
            responseParams.insert(QString("result"),QString("fail"));
            responseParams.insert(QString("info"),QString("not found agv"));
        }else{
            int id = g_taskCenter.makeAgvPickupTask(agvId,iX,iY,waitTypeX,watiTimeX,waitTypeY,watiTimeY);
            responseParams.insert(QString("info"),QString(""));
            responseParams.insert(QString("result"),QString("success"));
            responseParams.insert(QString("id"),QString("%1").arg(id));
        }
    }
}
//查询任务状态
void UserMsgProcessor::Task_QueryStatus(const QyhMsgDateItem &item, QMap<QString, QString> &requestDatas, QList<QMap<QString, QString> > &datalists,QMap<QString,QString> &responseParams,QList<QMap<QString,QString> > &responseDatalists,LoginUserInfo &loginUserInfo){
    if(checkParamExistAndNotNull(requestDatas,responseParams,"taskid",NULL)){
        int taskid = requestDatas["taskid"].toInt();
        int status = g_taskCenter.queryTaskStatus(taskid);

        responseParams.insert(QString("status"),QString("%1").arg(status));
        responseParams.insert(QString("info"),QString(""));
        responseParams.insert(QString("result"),QString("success"));
    }
}
//取消任务
void UserMsgProcessor::Task_Cancel(const QyhMsgDateItem &item, QMap<QString, QString> &requestDatas, QList<QMap<QString, QString> > &datalists,QMap<QString,QString> &responseParams,QList<QMap<QString,QString> > &responseDatalists,LoginUserInfo &loginUserInfo){
    if(checkParamExistAndNotNull(requestDatas,responseParams,"taskid",NULL)){
        int taskid = requestDatas["taskid"].toInt();

        if(g_taskCenter.cancelTask(taskid)){
            responseParams.insert(QString("info"),QString(""));
            responseParams.insert(QString("result"),QString("success"));
        }else{
            //
            responseParams.insert(QString("info"),QString("not find taskid in unassigned or doging tasks list"));
            responseParams.insert(QString("result"),QString("fail"));
        }
    }
}
//未分配任务列表
void UserMsgProcessor::Task_ListUnassigned(const QyhMsgDateItem &item, QMap<QString, QString> &requestDatas, QList<QMap<QString, QString> > &datalists,QMap<QString,QString> &responseParams,QList<QMap<QString,QString> > &responseDatalists,LoginUserInfo &loginUserInfo){
    responseParams.insert(QString("info"),QString(""));
    responseParams.insert(QString("result"),QString("success"));
    //添加列表
    QList<AgvTask *> tasks = g_taskCenter.getUnassignedTasks();
    for(QList<AgvTask *>::iterator itr = tasks.begin();itr!=tasks.end();++itr){
        AgvTask * task = *itr;
        QMap<QString,QString> onetask;

        onetask.insert(QString("id"),QString("%1").arg(task->id()));
        onetask.insert(QString("produceTime"),task->produceTime().toString(DATE_TIME_FORMAT));
        onetask.insert(QString("excuteCar"),QString("%1").arg(task->excuteCar()));
        onetask.insert(QString("status"),QString("%1").arg(task->status()));

        responseDatalists.push_back(onetask);
    }

}
//正在执行任务列表
void UserMsgProcessor::Task_ListDoing(const QyhMsgDateItem &item, QMap<QString, QString> &requestDatas, QList<QMap<QString, QString> > &datalists,QMap<QString,QString> &responseParams,QList<QMap<QString,QString> > &responseDatalists,LoginUserInfo &loginUserInfo){
    responseParams.insert(QString("info"),QString(""));
    responseParams.insert(QString("result"),QString("success"));
    //添加列表
    QList<AgvTask *> tasks = g_taskCenter.getDoingTasks();
    for(QList<AgvTask *>::iterator itr = tasks.begin();itr!=tasks.end();++itr){
        AgvTask * task = *itr;
        QMap<QString,QString> onetask;

        onetask.insert(QString("id"),QString("%1").arg(task->id()));
        onetask.insert(QString("produceTime"),task->produceTime().toString(DATE_TIME_FORMAT));
        onetask.insert(QString("doTime"),task->doTime().toString(DATE_TIME_FORMAT));
        onetask.insert(QString("excuteCar"),QString("%1").arg(task->excuteCar()));
        onetask.insert(QString("status"),QString("%1").arg(task->status()));

        responseDatalists.push_back(onetask);
    }
}
//已经完成任务列表(today)
void UserMsgProcessor::Task_ListDoneToday(const QyhMsgDateItem &item, QMap<QString, QString> &requestDatas, QList<QMap<QString, QString> > &datalists,QMap<QString,QString> &responseParams,QList<QMap<QString,QString> > &responseDatalists,LoginUserInfo &loginUserInfo){
    responseParams.insert(QString("info"),QString(""));
    responseParams.insert(QString("result"),QString("success"));

    QString querySql = "select id,task_produceTime,task_doneTime,task_doTime,task_excuteCar,task_status from agv_task where task_status = ? and task_doneTime between ? and ?;";
    QDate today = QDate::currentDate();
    QDate tomorrow = today.addDays(1);
    QList<QVariant> params;
    params<<AGV_TASK_STATUS_DONE;

    QDateTime to(tomorrow);
    QDateTime from(today);

    params<<from;
    params<<to;

    QList<QList<QVariant> > result = g_sql->query(querySql,params);

    for(int i=0;i<result.length();++i)
    {
        QList<QVariant> qsl = result.at(i);
        if(qsl.length() == 6)
        {
            QMap<QString,QString> task;
            task.insert(QString("id"),qsl.at(0).toString());
            task.insert(QString("produceTime"),qsl.at(1).toDateTime().toString(DATE_TIME_FORMAT));
            task.insert(QString("doneTime"),qsl.at(2).toDateTime().toString(DATE_TIME_FORMAT));
            task.insert(QString("doTime"),qsl.at(3).toDateTime().toString(DATE_TIME_FORMAT));
            task.insert(QString("excuteCar"),qsl.at(4).toString());
            task.insert(QString("status"),qsl.at(5).toString());
            responseDatalists.push_back(task);
        }
    }
}

//已经完成任务列表(all)
void UserMsgProcessor::Task_ListDoneAll(const QyhMsgDateItem &item, QMap<QString, QString> &requestDatas, QList<QMap<QString, QString> > &datalists,QMap<QString,QString> &responseParams,QList<QMap<QString,QString> > &responseDatalists,LoginUserInfo &loginUserInfo)
{
    responseParams.insert(QString("info"),QString(""));
    responseParams.insert(QString("result"),QString("success"));

    QString querySql = "select id,task_produceTime,task_doneTime,task_doTime,task_excuteCar,task_status from agv_task where task_status = ? ";

    QList<QVariant> params;
    params<<AGV_TASK_STATUS_DONE;

    QList<QList<QVariant> > result = g_sql->query(querySql,params);

    for(int i=0;i<result.length();++i)
    {
        QList<QVariant> qsl = result.at(i);
        if(qsl.length() == 6)
        {
            QMap<QString,QString> task;
            task.insert(QString("id"),qsl.at(0).toString());
            task.insert(QString("produceTime"),qsl.at(1).toDateTime().toString(DATE_TIME_FORMAT));
            task.insert(QString("doneTime"),qsl.at(2).toDateTime().toString(DATE_TIME_FORMAT));
            task.insert(QString("doTime"),qsl.at(3).toDateTime().toString(DATE_TIME_FORMAT));
            task.insert(QString("excuteCar"),qsl.at(4).toString());
            task.insert(QString("status"),qsl.at(5).toString());
            responseDatalists.push_back(task);
        }
    }
}

//已经完成任务列表(from to 时间)
void UserMsgProcessor::Task_ListDoneDuring(const QyhMsgDateItem &item, QMap<QString, QString> &requestDatas, QList<QMap<QString, QString> > &datalists,QMap<QString,QString> &responseParams,QList<QMap<QString,QString> > &responseDatalists,LoginUserInfo &loginUserInfo)
{
    //要求带有from和to
    if(checkParamExistAndNotNull(requestDatas,responseParams,"from","to",NULL)){
        responseParams.insert(QString("info"),QString(""));
        responseParams.insert(QString("result"),QString("success"));

        QString querySql = "select id,task_produceTime,task_doneTime,task_doTime,task_excuteCar,task_status from agv_task where task_status = ? and task_doneTime between ? and ?;";
        QList<QVariant> params;
        params<<AGV_TASK_STATUS_DONE;
        QDateTime from = QDateTime::fromString(requestDatas["from"]);
        QDateTime to = QDateTime::fromString(requestDatas["to"]);
        params<<from;
        params<<to;

        QList<QList<QVariant> > result = g_sql->query(querySql,params);

        for(int i=0;i<result.length();++i){
            QList<QVariant> qsl = result.at(i);
            if(qsl.length() == 6)
            {
                QMap<QString,QString> task;
                task.insert(QString("id"),qsl.at(0).toString());
                task.insert(QString("produceTime"),qsl.at(1).toDateTime().toString(DATE_TIME_FORMAT));
                task.insert(QString("doneTime"),qsl.at(2).toDateTime().toString(DATE_TIME_FORMAT));
                task.insert(QString("doTime"),qsl.at(3).toDateTime().toString(DATE_TIME_FORMAT));
                task.insert(QString("excuteCar"),qsl.at(4).toString());
                task.insert(QString("status"),qsl.at(5).toString());
                responseDatalists.push_back(task);
            }
        }
    }
}

void UserMsgProcessor::Task_Detail(const QyhMsgDateItem &item, QMap<QString, QString> &requestDatas, QList<QMap<QString, QString> > &datalists,QMap<QString,QString> &responseParams,QList<QMap<QString,QString> > &responseDatalists,LoginUserInfo &loginUserInfo)
{
    //要求带有taskid
    bool needDelete = false;
    AgvTask *task = NULL;
    if(checkParamExistAndNotNull(requestDatas,responseParams,"taskid",NULL)){
        int taskId = requestDatas["taskid"].toInt();

        task = g_taskCenter.queryUndoTask(taskId);
        if(task == NULL){
            task = g_taskCenter.queryDoingTask(taskId);
            if(task == NULL){
                task = g_taskCenter.queryDoneTask(taskId);
                needDelete = true;
            }
        }

        if(task == NULL){
            //未找到该任务
            responseParams.insert(QString("info"),QString("not found task with taskid"));
            responseParams.insert(QString("result"),QString("fail"));
        }else{
            responseParams.insert(QString("info"),QString(""));
            responseParams.insert(QString("result"),QString("success"));

            {
                responseParams.insert(QString("id"),QString("%1").arg(task->id()));
                responseParams.insert(QString("produceTime"),task->produceTime().toString(DATE_TIME_FORMAT));
                responseParams.insert(QString("doneTime"),task->doneTime().toString(DATE_TIME_FORMAT));
                responseParams.insert(QString("doTime"),task->doTime().toString(DATE_TIME_FORMAT));
                responseParams.insert(QString("excuteCar"),QString("%1").arg(task->excuteCar()));
                responseParams.insert(QString("status"),QString("%1").arg(task->status()));
            }
            //装入节点
            for(int i=0;i<task->taskNodes.length();++i)
            {
                TaskNode *tn = task->taskNodes.at(i);

                QMap<QString,QString> node;

                node.insert(QString("status"),QString("%1").arg(tn->status));
                node.insert(QString("queueNumber"),QString("%1").arg(tn->queueNumber));
                node.insert(QString("aimStation"),QString("%1").arg(tn->aimStation));
                node.insert(QString("waitType"),QString("%1").arg(tn->waitType));
                node.insert(QString("waitTime"),QString("%1").arg(tn->waitTime));
                node.insert(QString("arriveTime"),tn->arriveTime.toString(DATE_TIME_FORMAT));
                node.insert(QString("leaveTime"),tn->leaveTime.toString(DATE_TIME_FORMAT));

                responseDatalists.push_back(node);
            }

            if(needDelete)
                delete task;
        }
    }
}

//查询日志 from to时间
void UserMsgProcessor::Log_ListDuring(const QyhMsgDateItem &item, QMap<QString, QString> &requestDatas, QList<QMap<QString, QString> > &datalists,QMap<QString,QString> &responseParams,QList<QMap<QString,QString> > &responseDatalists,LoginUserInfo &loginUserInfo)
{
    //要求带有from和to <trace> <debug> <info> <warn> <error> <fatal>
    if(checkParamExistAndNotNull(requestDatas,responseParams,"from","to","trace","debug","info","warn","error","fatal",NULL))
    {
        responseParams.insert(QString("info"),QString(""));
        responseParams.insert(QString("result"),QString("success"));

        bool trace = requestDatas["trace"]=="1";
        bool debug = requestDatas["debug"]=="1";
        bool info = requestDatas["info"]=="1";
        bool warn = requestDatas["warn"]=="1";
        bool error = requestDatas["error"]=="1";
        bool fatal = requestDatas["fatal"]=="1";

        bool firstappend = true;

        QString querySql = "select log_level,log_time,log_msg from agv_log where log_time between ? and ? ";

        QList<QVariant> params;
        QDateTime from = QDateTime::fromString(requestDatas["from"],DATE_TIME_FORMAT);
        QDateTime to = QDateTime::fromString(requestDatas["to"],DATE_TIME_FORMAT);
        params<<from;
        params<<to;

        if(!trace&&!debug&&!info&&!warn&&!error&&!fatal){

        }else{
            querySql += "and (";
            if(trace){
                if(firstappend){
                    querySql += QString(" log_level=? ");
                }
                else{
                    querySql += QString("or log_level=? ");
                }
                params<<AGV_LOG_LEVEL_TRACE;
                firstappend = false;
            }

            if(debug){
                if(firstappend){
                    querySql += QString(" log_level=? ");
                }
                else{
                    querySql += QString("or log_level=? ");
                }
                params<<AGV_LOG_LEVEL_TRACE;
                firstappend = false;
            }

            if(info){
                if(firstappend){
                    querySql += QString(" log_level=? ");
                }
                else{
                    querySql += QString("or log_level=? ");
                }
                params<<AGV_LOG_LEVEL_TRACE;
                firstappend = false;
            }

            if(warn){
                if(firstappend){
                    querySql += QString(" log_level=? ");
                }
                else{
                    querySql += QString("or log_level=? ");
                }
                params<<AGV_LOG_LEVEL_TRACE;
                firstappend = false;
            }

            if(error){
                if(firstappend){
                    querySql += QString(" log_level=? ");
                }
                else{
                    querySql += QString("or log_level=? ");
                }
                params<<AGV_LOG_LEVEL_TRACE;
                firstappend = false;
            }

            if(fatal){
                if(firstappend){
                    querySql += QString(" log_level=? ");
                }
                else{
                    querySql += QString("or log_level=? ");
                }
                params<<AGV_LOG_LEVEL_TRACE;
                firstappend = false;
            }
            querySql += ");";
        }

        QList<QList<QVariant> > result = g_sql->query(querySql,params);

        for(int i=0;i<result.length();++i){
            QList<QVariant> qsl = result.at(i);
            if(qsl.length() == 3)
            {
                QMap<QString,QString> log;
                log.insert(QString("level"),qsl.at(0).toString());
                //这里注意啦！！！查询出来的日期，有可能是带有T的。
                QString timestr = qsl.at(1).toDateTime().toString(DATE_TIME_FORMAT);
                timestr = timestr.replace("T"," ");
                log.insert(QString("time"),timestr);
                log.insert(QString("msg"),qsl.at(2).toString());
                responseDatalists.push_back(log);
            }
        }
    }
}

//查询所有日志
void UserMsgProcessor::Log_ListAll(const QyhMsgDateItem &item, QMap<QString, QString> &requestDatas, QList<QMap<QString, QString> > &datalists,QMap<QString,QString> &responseParams,QList<QMap<QString,QString> > &responseDatalists,LoginUserInfo &loginUserInfo)
{
    ////<trace> <debug> <info> <warn> <error> <fatal>
    if(checkParamExistAndNotNull(requestDatas,responseParams,"trace","debug","info","warn","error","fatal",NULL))
    {
        responseParams.insert(QString("info"),QString(""));
        responseParams.insert(QString("result"),QString("success"));

        QString querySql = "select log_level,log_time,log_msg from agv_log where log_time between ? and ?;";
        QList<QVariant> params;
        QDateTime from = QDateTime::fromString(requestDatas["from"],DATE_TIME_FORMAT);
        QDateTime to = QDateTime::fromString(requestDatas["to"],DATE_TIME_FORMAT);
        params<<from;
        params<<to;

        QList<QList<QVariant> > result = g_sql->query(querySql,params);

        for(int i=0;i<result.length();++i){
            QList<QVariant> qsl = result.at(i);
            if(qsl.length() == 3)
            {
                QMap<QString,QString> log;
                log.insert(QString("level"),qsl.at(0).toString());
                log.insert(QString("time"),qsl.at(1).toDateTime().toString(DATE_TIME_FORMAT));
                log.insert(QString("msg"),qsl.at(2).toString());
                responseDatalists.push_back(log);
            }
        }
    }
}
//订阅日志
void UserMsgProcessor::Log_Subscribe(const QyhMsgDateItem &item, QMap<QString, QString> &requestDatas, QList<QMap<QString, QString> > &datalists,QMap<QString,QString> &responseParams,QList<QMap<QString,QString> > &responseDatalists,LoginUserInfo &loginUserInfo)
{
    if(checkParamExistAndNotNull(requestDatas,responseParams,"trace","debug","info","warn","error","fatal",NULL))
    {
        responseParams.insert(QString("info"),QString(""));
        responseParams.insert(QString("result"),QString("success"));
        g_logProcess->removeSubscribe(item.sock);//先去掉原来的订阅
        //加入现在的订阅
        SubNode subnode;
        subnode.trace = requestDatas["trace"] == "1";
        subnode.debug = requestDatas["debug"] == "1";
        subnode.info = requestDatas["info"] == "1";
        subnode.warn = requestDatas["warn"] == "1";
        subnode.error = requestDatas["error"] == "1";
        subnode.fatal = requestDatas["fatal"] == "1";
        g_logProcess->addSubscribe(item.sock,subnode);
    }
}

//取消订阅日志
void UserMsgProcessor::Log_CancelSubscribe(const QyhMsgDateItem &item, QMap<QString, QString> &requestDatas, QList<QMap<QString, QString> > &datalists,QMap<QString,QString> &responseParams,QList<QMap<QString,QString> > &responseDatalists,LoginUserInfo &loginUserInfo)
{

    responseParams.insert(QString("info"),QString(""));
    responseParams.insert(QString("result"),QString("success"));
    g_logProcess->removeSubscribe(item.sock);

}



