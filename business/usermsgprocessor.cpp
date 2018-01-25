#include "usermsgprocessor.h"
#include "util/global.h"
#include "pugixml.hpp"
#include <sstream>
#include <iostream>
#include <QUuid>
#include <stdarg.h>
#include <QDebug>

UserMsgProcessor::UserMsgProcessor(QObject *parent) : QObject(parent)
{
}

UserMsgProcessor::~UserMsgProcessor(){
}

QString UserMsgProcessor::makeAccessToken()
{
    //生成一个随机的16位字符串
    return QUuid::createUuid().toString().replace("{","").replace("}","");
}

//对接收到的xml消息进行转换
//这里，采用速度最最快的pluginXml.以保证效率，解析单个xml的时间应该在3ms之内
std::string UserMsgProcessor::parseOneMsg(zmq::context_t *ctx, const std::string &oneMsg)
{
    QMap<QString,QString> params;
    QList<QMap<QString,QString> > datalist;
    //解析
    if(!getRequestParam(oneMsg,params,datalist))return "";

    //初步判断，如果不合格，那就直接淘汰
    QMap<QString,QString>::iterator itr;
    if((itr=params.find("type"))!=params.end()
            &&(itr=params.find("todo"))!=params.end()
            &&(itr=params.find("queuenumber"))!=params.end()){
        //接下来对这条消息进行响应
        //        LARGE_INTEGER start;
        //        LARGE_INTEGER end ;
        //        LARGE_INTEGER frequency;
        //        QueryPerformanceFrequency(&frequency);
        //        QueryPerformanceCounter(&start); //开始计时
        return  responseOneMsg(ctx,params,datalist);
        //        QueryPerformanceCounter(&end); //结束计时
        //        qDebug()<<"reponse one msg total time= "<<1000*(double)(end.QuadPart - start.QuadPart) / (double)frequency.QuadPart<<" ms";
    }
    return "";
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

bool UserMsgProcessor::checkAccessToken(zmq::context_t *ctx,QMap<QString,QString> &requestDatas,QMap<QString,QString> &responseParams,LoginUserInfo &loginUserinfo)
{
    if(!checkParamExistAndNotNull(requestDatas,responseParams,"access_token",NULL))
        return false;

    ////对access_token判断是否正确
    bool access_token_correct = false;

    loginUserIdSockMutex.lock();
    for(QList<LoginUserInfo>::iterator itr = loginUserIdSock.begin();itr!=loginUserIdSock.end();++itr)
    {
        if(itr->ctx == ctx && itr->access_tocken == requestDatas["access_token"])
        {
            access_token_correct = true;
            loginUserinfo = *itr;
            break;
        }
    }
    loginUserIdSockMutex.unlock();

    if(!access_token_correct){
        //access_token错误
        responseParams.insert(QString("info"),QString("not correct:access_token,please relogin!"));
        responseParams.insert(QString("result"),QString("fail"));
        return false;
    }
    return true;
}

//对接收到的消息，进行处理！
std::string UserMsgProcessor::responseOneMsg(zmq::context_t *ctx, QMap<QString, QString> requestDatas, QList<QMap<QString,QString> > datalists)
{
    if(requestDatas["type"] == "user"){
        return clientMsgUserProcess(ctx,requestDatas,datalists);
    }else if(requestDatas["type"] == "map"){
        return clientMsgMapProcess(ctx,requestDatas,datalists);
    }else if(requestDatas["type"] == "agv"){
        return clientMsgAgvProcess(ctx,requestDatas,datalists);
    }else if(requestDatas["type"] == "agvManage"){
        return clientMsgAgvManageProcess(ctx,requestDatas,datalists);
    }else if(requestDatas["type"] == "task"){
        return clientMsgTaskProcess(ctx,requestDatas,datalists);
    }else if(requestDatas["type"] == "log"){
        return clientMsgLogProcess(ctx,requestDatas,datalists);
    }
    return "";
}

std::string UserMsgProcessor::clientMsgUserProcess(zmq::context_t *ctx,QMap<QString,QString> &requestDatas,QList<QMap<QString,QString> > &datalists)
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
        User_Login(ctx,requestDatas,datalists,responseParams,responseDatalists,loginUserinfo);
        return  getResponseXml(responseParams,responseDatalists);
    }
    /////////////////////////////////////这段是和掐所有地方不应的一个点

    /////所有的非登录消息，需要进行，安全验证 随机码
    if(!checkAccessToken(ctx,requestDatas,responseParams,loginUserinfo)){
        return getResponseXml(responseParams,responseDatalists);
    }

    if(requestDatas["todo"]=="logout")
    {
        User_Logout(requestDatas,datalists,responseParams,responseDatalists);
    }
    else if(requestDatas["todo"]=="changepassword"){
        User_ChangePassword(ctx,requestDatas,datalists,responseParams,responseDatalists);
    }
    else if(requestDatas["todo"]=="list"){
        User_List(ctx,requestDatas,datalists,responseParams,responseDatalists,loginUserinfo);
    }
    else if(requestDatas["todo"]=="delete"){
        User_Delete(ctx,requestDatas,datalists,responseParams,responseDatalists);
    }
    else if(requestDatas["todo"]=="add"){
        User_Add(ctx,requestDatas,datalists,responseParams,responseDatalists);
    }
    else if(requestDatas["todo"]=="modify"){
        User_Modify(ctx,requestDatas,datalists,responseParams,responseDatalists);
    }

    return getResponseXml(responseParams,responseDatalists);
}

std::string UserMsgProcessor::clientMsgMapProcess(zmq::context_t *ctx, QMap<QString,QString> &requestDatas, QList<QMap<QString,QString> > &datalists)
{
    QMap<QString,QString> responseParams;
    QList<QMap<QString,QString> > responseDatalists;
    responseParams.insert(QString("type"),requestDatas["type"]);
    responseParams.insert(QString("todo"),requestDatas["todo"]);
    responseParams.insert(QString("queuenumber"),requestDatas["queuenumber"]);
    LoginUserInfo loginUserinfo;

    /////所有的非登录消息，需要进行，安全验证 随机码
    if(!checkAccessToken(ctx,requestDatas,responseParams,loginUserinfo)){
        return getResponseXml(responseParams,responseDatalists);
    }

    /// 创建地图
    if(requestDatas["todo"]=="create"){
        Map_Create(ctx,requestDatas,datalists,responseParams,responseDatalists);
    }
    /// 站点列表
    if(requestDatas["todo"]=="stationlist"){
        Map_StationList(ctx,requestDatas,datalists,responseParams,responseDatalists);
    }

    /// 线路列表
    else if(requestDatas["todo"]=="linelist"){
        Map_LineList(ctx,requestDatas,datalists,responseParams,responseDatalists);
    }

    return  getResponseXml(responseParams,responseDatalists);

}

std::string UserMsgProcessor::clientMsgAgvProcess(zmq::context_t *ctx,QMap<QString,QString> &requestDatas,QList<QMap<QString,QString> > &datalists)
{
    QMap<QString,QString> responseParams;
    QList<QMap<QString,QString> > responseDatalists;
    responseParams.insert(QString("type"),requestDatas["type"]);
    responseParams.insert(QString("todo"),requestDatas["todo"]);
    responseParams.insert(QString("queuenumber"),requestDatas["queuenumber"]);
    LoginUserInfo loginUserinfo;

    /////所有的非登录消息，需要进行，安全验证 随机码
    if(!checkAccessToken(ctx,requestDatas,responseParams,loginUserinfo)){
        return getResponseXml(responseParams,responseDatalists);
    }

    /// 请求控制权
    if(requestDatas["todo"]=="hand"){
        //Agv_Hand(item,requestDatas,datalists,responseParams,responseDatalists,loginUserinfo);
    }
    /// 释放控制权
    else  if(requestDatas["todo"]=="release"){
        //Agv_Release(item,requestDatas,datalists,responseParams,responseDatalists,loginUserinfo);
    }
    /// 小车前进
    else  if(requestDatas["todo"]=="forward"){
        //Agv_Forward(item,requestDatas,datalists,responseParams,responseDatalists,loginUserinfo);
    }
    /// 小车后退
    else  if(requestDatas["todo"]=="backward"){
        //Agv_Backward(item,requestDatas,datalists,responseParams,responseDatalists,loginUserinfo);
    }
    /// 小车左转
    else  if(requestDatas["todo"]=="turnleft"){
        //Agv_Turnleft(item,requestDatas,datalists,responseParams,responseDatalists,loginUserinfo);
    }
    /// 小车右转
    else  if(requestDatas["todo"]=="turnright"){
        //Agv_Turnright(item,requestDatas,datalists,responseParams,responseDatalists,loginUserinfo);
    }
    /// 小车灯带
    else  if(requestDatas["todo"]=="turnright"){
        //Agv_Light(item,requestDatas,datalists,responseParams,responseDatalists);
    }

    return getResponseXml(responseParams,responseDatalists);
}

std::string UserMsgProcessor::clientMsgAgvManageProcess(zmq::context_t *ctx,QMap<QString,QString> &requestDatas,QList<QMap<QString,QString> > &datalists)
{
    QMap<QString,QString> responseParams;
    QList<QMap<QString,QString> > responseDatalists;
    responseParams.insert(QString("type"),requestDatas["type"]);
    responseParams.insert(QString("todo"),requestDatas["todo"]);
    responseParams.insert(QString("queuenumber"),requestDatas["queuenumber"]);
    LoginUserInfo loginUserinfo;

    /////所有的非登录消息，需要进行，安全验证 随机码
    if(!checkAccessToken(ctx,requestDatas,responseParams,loginUserinfo)){
        return getResponseXml(responseParams,responseDatalists);
    }

    /// agv列表
    if(requestDatas["todo"]=="list"){
        AgvManage_List(ctx,requestDatas,datalists,responseParams,responseDatalists);
    }
    /// 增加agv
    else  if(requestDatas["todo"]=="add"){
        AgvManage_Add(ctx,requestDatas,datalists,responseParams,responseDatalists);
    }
    /// 删除agv
    else  if(requestDatas["todo"]=="delete"){
        AgvManage_Delete(ctx,requestDatas,datalists,responseParams,responseDatalists);
    }
    /// 修改agv
    else  if(requestDatas["todo"]=="modify"){
        AgvManage_Modify(ctx,requestDatas,datalists,responseParams,responseDatalists);
    }
    return getResponseXml(responseParams,responseDatalists);
}

std::string UserMsgProcessor::clientMsgTaskProcess(zmq::context_t *ctx,QMap<QString,QString> &requestDatas,QList<QMap<QString,QString> > &datalists)
{
    QMap<QString,QString> responseParams;
    QList<QMap<QString,QString> > responseDatalists;
    responseParams.insert(QString("type"),requestDatas["type"]);
    responseParams.insert(QString("todo"),requestDatas["todo"]);
    responseParams.insert(QString("queuenumber"),requestDatas["queuenumber"]);
    LoginUserInfo loginUserinfo;

    /////所有的非登录消息，需要进行，安全验证 随机码
    if(!checkAccessToken(ctx,requestDatas,responseParams,loginUserinfo)){
        return getResponseXml(responseParams,responseDatalists);
    }

    /// 创建任务(创建到X点的任务)
    else if(requestDatas["todo"]=="toX"){
        Task_CreateToX(ctx,requestDatas,datalists,responseParams,responseDatalists);
    }
    /// 创建任务(创建指定车辆到X点的任务)
    else  if(requestDatas["todo"]=="agvToX"){
        Task_CreateAgvToX(ctx,requestDatas,datalists,responseParams,responseDatalists);
    }
    /// 创建任务(创建经过Y点到X点的任务)
    else  if(requestDatas["todo"]=="passYtoX"){
        Task_CreateYToX(ctx,requestDatas,datalists,responseParams,responseDatalists);
    }
    /// 创建任务(创建指定车辆经过Y点到X点的任务)
    else  if(requestDatas["todo"]=="agvPassYtoX"){
        Task_CreateAgvYToX(ctx,requestDatas,datalists,responseParams,responseDatalists);
    }
    /// 创建任务(创建指定车辆经过Y点到X点的任务)
    else  if(requestDatas["todo"]=="agvPassYtoXCircle"){
        Task_CreateAgvYToXCircle(ctx,requestDatas,datalists,responseParams,responseDatalists);
    }
    /// 查询任务状态
    if(requestDatas["todo"]=="queryStatus"){
        Task_QueryStatus(ctx,requestDatas,datalists,responseParams,responseDatalists);
    }
    /// 取消任务
    else  if(requestDatas["todo"]=="cancel"){
        Task_Cancel(ctx,requestDatas,datalists,responseParams,responseDatalists);
    }
    /// 未分配任务列表
    else  if(requestDatas["todo"]=="listUndo"){
        Task_ListUnassigned(ctx,requestDatas,datalists,responseParams,responseDatalists);
    }
    /// 正在执行任务列表
    else  if(requestDatas["todo"]=="listDoing"){
        Task_ListDoing(ctx,requestDatas,datalists,responseParams,responseDatalists);
    }
    /// 已经完成任务列表(today)
    if(requestDatas["todo"]=="listDoneToday"){
        Task_ListDoneToday(ctx,requestDatas,datalists,responseParams,responseDatalists);
    }
    /// 已经完成任务列表(all)
    else  if(requestDatas["todo"]=="listDone"){
        Task_ListDoneAll(ctx,requestDatas,datalists,responseParams,responseDatalists);
    }
    /// 已经完成任务列表(from to 时间)
    else  if(requestDatas["todo"]=="listDuring"){
        Task_ListDoneDuring(ctx,requestDatas,datalists,responseParams,responseDatalists);
    }
    /// 查询任务详情
    else  if(requestDatas["todo"]=="detail"){
        Task_Detail(ctx,requestDatas,datalists,responseParams,responseDatalists);
    }
    return getResponseXml(responseParams,responseDatalists);
}

std::string UserMsgProcessor::clientMsgLogProcess(zmq::context_t *ctx,QMap<QString,QString> &requestDatas,QList<QMap<QString,QString> > &datalists)
{
    QMap<QString,QString> responseParams;
    QList<QMap<QString,QString> > responseDatalists;
    responseParams.insert(QString("type"),requestDatas["type"]);
    responseParams.insert(QString("todo"),requestDatas["todo"]);
    responseParams.insert(QString("queuenumber"),requestDatas["queuenumber"]);
    LoginUserInfo loginUserinfo;

    /////所有的非登录消息，需要进行，安全验证 随机码
    if(!checkAccessToken(ctx,requestDatas,responseParams,loginUserinfo)){
        return getResponseXml(responseParams,responseDatalists);
    }

    if(requestDatas["todo"]=="listDuring"){
        Log_ListDuring(ctx,requestDatas,datalists,responseParams,responseDatalists);
    }
    else  if(requestDatas["todo"]=="listAll"){
        Log_ListAll(ctx,requestDatas,datalists,responseParams,responseDatalists);
    }
    return getResponseXml(responseParams,responseDatalists);
}

//接下来是具体的业务
/////////////////////////////关于用户部分
//用户登录
void UserMsgProcessor::User_Login(zmq::context_t *ctx, QMap<QString, QString> &requestDatas, QList<QMap<QString, QString> > &datalists,QMap<QString,QString> &responseParams,QList<QMap<QString,QString> > &responseDatalists,LoginUserInfo &loginUserInfo)
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
                    loginUserInfo.ctx = ctx;
                    loginUserInfo.access_tocken = makeAccessToken();
                    loginUserInfo.password = requestDatas["username"];
                    loginUserInfo.username = requestDatas["password"];
                    loginUserInfo.role = queryresult.at(0).at(2).toInt();
                    loginUserIdSockMutex.lock();
                    loginUserIdSock.push_back(loginUserInfo);
                    loginUserIdSockMutex.unlock();

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
void UserMsgProcessor:: User_Logout(QMap<QString, QString> &requestDatas, QList<QMap<QString, QString> > &datalists,QMap<QString,QString> &responseParams,QList<QMap<QString,QString> > &responseDatalists)
{
    //////////////////退出登录
    if(checkParamExistAndNotNull(requestDatas,responseParams,"id",NULL)){
        //设置它的数据库中的状态
        int id = requestDatas["id"].toInt();
        QString updateSql = "update agv_user set user_signState = 0 where id=?";
        QList<QVariant> param;
        param<<id;
        g_sql->exeSql(updateSql,param);
        loginUserIdSockMutex.lock();
        for(QList<LoginUserInfo>::iterator itr=loginUserIdSock.begin();itr!=loginUserIdSock.end();++itr)
        {
            if(itr->id == id){
                loginUserIdSock.erase(itr);
                break;
            }
        }
        loginUserIdSockMutex.unlock();
        responseParams.insert(QString("info"),QString(""));
        responseParams.insert(QString("result"),QString("success"));
    }
}

//修改密码
void UserMsgProcessor:: User_ChangePassword(zmq::context_t *ctx, QMap<QString, QString> &requestDatas, QList<QMap<QString, QString> > &datalists,QMap<QString,QString> &responseParams,QList<QMap<QString,QString> > &responseDatalists){
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
                /////设置新的密码
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
void UserMsgProcessor:: User_List(zmq::context_t *ctx, QMap<QString, QString> &requestDatas, QList<QMap<QString, QString> > &datalists,QMap<QString,QString> &responseParams,QList<QMap<QString,QString> > &responseDatalists,LoginUserInfo &loginUserInfo){
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
void UserMsgProcessor:: User_Delete(zmq::context_t *ctx, QMap<QString, QString> &requestDatas, QList<QMap<QString, QString> > &datalists,QMap<QString,QString> &responseParams,QList<QMap<QString,QString> > &responseDatalists){
    ////////////////////////////////删除用户
    if(checkParamExistAndNotNull(requestDatas,responseParams,"id",NULL)){
        QString deleteSql = "delete from agv_user where id=?";
        QList<QVariant> params;
        params<<requestDatas["id"];
        if(!g_sql->exeSql(deleteSql,params)){
            responseParams.insert(QString("info"),QString("delete fail for sql fail"));
            responseParams.insert(QString("result"),QString("fail"));
        }else{
            loginUserIdSockMutex.lock();
            for(QList<LoginUserInfo>::iterator itr=loginUserIdSock.begin();itr!=loginUserIdSock.end();++itr)
            {
                if(itr->id == requestDatas["id"].toInt()){
                    loginUserIdSock.erase(itr);
                    break;
                }
            }
            loginUserIdSockMutex.unlock();
            responseParams.insert(QString("info"),QString(""));
            responseParams.insert(QString("result"),QString("success"));
        }
    }
}

void UserMsgProcessor::User_Modify(zmq::context_t *ctx, QMap<QString, QString> &requestDatas, QList<QMap<QString, QString> > &datalists,QMap<QString,QString> &responseParams,QList<QMap<QString,QString> > &responseDatalists)
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
void UserMsgProcessor:: User_Add(zmq::context_t *ctx, QMap<QString, QString> &requestDatas, QList<QMap<QString, QString> > &datalists,QMap<QString,QString> &responseParams,QList<QMap<QString,QString> > &responseDatalists){
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
void UserMsgProcessor::Map_Create(zmq::context_t *ctx, QMap<QString, QString> &requestDatas, QList<QMap<QString, QString> > &datalists,QMap<QString,QString> &responseParams,QList<QMap<QString,QString> > &responseDatalists)
{
    if(checkParamExistAndNotNull(requestDatas,responseParams,"line","arc","station",NULL))
    {
        responseParams.insert(QString("info"),QString(""));
        responseParams.insert(QString("result"),QString("success"));
        g_agvMapCenter->resetMap(requestDatas["station"],requestDatas["line"],requestDatas["arc"],requestDatas["image"]);
    }
}

//地图 站点列表
void UserMsgProcessor::Map_StationList(zmq::context_t *ctx, QMap<QString, QString> &requestDatas, QList<QMap<QString, QString> > &datalists,QMap<QString,QString> &responseParams,QList<QMap<QString,QString> > &responseDatalists){
    responseParams.insert(QString("info"),QString(""));
    responseParams.insert(QString("result"),QString("success"));

    QMap<int,AgvStation *> stations = g_agvMapCenter->getAgvStations();

    for(QMap<int,AgvStation *>::iterator itr=stations.begin();itr!=stations.end();++itr)
    {
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
void UserMsgProcessor:: Map_LineList(zmq::context_t *ctx, QMap<QString, QString> &requestDatas, QList<QMap<QString, QString> > &datalists,QMap<QString,QString> &responseParams,QList<QMap<QString,QString> > &responseDatalists){
    responseParams.insert(QString("info"),QString(""));
    responseParams.insert(QString("result"),QString("success"));
    QMap<int,AgvLine *> lines = g_agvMapCenter->getAgvLines();
    for(QMap<int,AgvLine *>::iterator itr=lines.begin();itr!=lines.end();++itr){
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

/////////////////////////////////车辆管理部分
//列表
void UserMsgProcessor:: AgvManage_List(zmq::context_t *ctx, QMap<QString, QString> &requestDatas, QList<QMap<QString, QString> > &datalists,QMap<QString,QString> &responseParams,QList<QMap<QString,QString> > &responseDatalists){
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
void UserMsgProcessor:: AgvManage_Add(zmq::context_t *ctx, QMap<QString, QString> &requestDatas, QList<QMap<QString, QString> > &datalists,QMap<QString,QString> &responseParams,QList<QMap<QString,QString> > &responseDatalists){
    //要求name和ip
    if(checkParamExistAndNotNull(requestDatas,responseParams,"name","ip",NULL)){
        //TODO
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
                g_m_agvs.insert(agv->id,agv);
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
void UserMsgProcessor:: AgvManage_Delete(zmq::context_t *ctx, QMap<QString, QString> &requestDatas, QList<QMap<QString, QString> > &datalists,QMap<QString,QString> &responseParams,QList<QMap<QString,QString> > &responseDatalists){
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
void UserMsgProcessor:: AgvManage_Modify(zmq::context_t *ctx, QMap<QString, QString> &requestDatas, QList<QMap<QString, QString> > &datalists,QMap<QString,QString> &responseParams,QList<QMap<QString,QString> > &responseDatalists){
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
//创建任务(创建到X点的任务)
void UserMsgProcessor::Task_CreateToX(zmq::context_t *ctx, QMap<QString, QString> &requestDatas, QList<QMap<QString, QString> > &datalists,QMap<QString,QString> &responseParams,QList<QMap<QString,QString> > &responseDatalists){
    if(checkParamExistAndNotNull(requestDatas,responseParams,"x",NULL))
    {
        int iX = requestDatas["x"].toInt();
        AgvStation station =  g_agvMapCenter->getAgvStation(iX);

        if(station.id>0){
            int id = g_taskCenter->makeAimTask(iX);
            responseParams.insert(QString("info"),QString(""));
            responseParams.insert(QString("result"),QString("success"));
            responseParams.insert(QString("id"),QString("%1").arg(id));
        }else{
            responseParams.insert(QString("result"),QString("fail"));
            responseParams.insert(QString("info"),QString("not found station"));
        }
    }
}

//创建任务(创建指定车辆到X点的任务)
void UserMsgProcessor::Task_CreateAgvToX(zmq::context_t *ctx, QMap<QString, QString> &requestDatas, QList<QMap<QString, QString> > &datalists,QMap<QString,QString> &responseParams,QList<QMap<QString,QString> > &responseDatalists){
    if(checkParamExistAndNotNull(requestDatas,responseParams,"x","agvid",NULL)){
        int iX = requestDatas["x"].toInt();
        int iAgvId = requestDatas["agvid"].toInt();

        AgvStation station =  g_agvMapCenter->getAgvStation(iX);

        if(station.id<=0){
            responseParams.insert(QString("result"),QString("fail"));
            responseParams.insert(QString("info"),QString("not found station"));
        }else if(!g_m_agvs.contains(iAgvId)){
            responseParams.insert(QString("result"),QString("fail"));
            responseParams.insert(QString("info"),QString("not found agv"));
        }else{
            int id = g_taskCenter->makeAgvAimTask(iAgvId,iX);
            responseParams.insert(QString("info"),QString(""));
            responseParams.insert(QString("result"),QString("success"));
            responseParams.insert(QString("id"),QString("%1").arg(id));
        }
    }
}

//创建任务(创建经过Y点到X点的任务)
void UserMsgProcessor::Task_CreateYToX(zmq::context_t *ctx, QMap<QString, QString> &requestDatas, QList<QMap<QString, QString> > &datalists,QMap<QString,QString> &responseParams,QList<QMap<QString,QString> > &responseDatalists){
    if(checkParamExistAndNotNull(requestDatas,responseParams,"x","y","z",NULL)){
        int iX = requestDatas["x"].toInt();
        int iY = requestDatas["y"].toInt();
        int iZ = requestDatas["z"].toInt();

        AgvStation xStation = g_agvMapCenter->getAgvStation(iX);
        AgvStation yStation = g_agvMapCenter->getAgvStation(iY);
        AgvStation zStation = g_agvMapCenter->getAgvStation(iZ);

        //确保站点存在
        if(xStation.id<=0){
            responseParams.insert(QString("result"),QString("fail"));
            responseParams.insert(QString("info"),QString("not found station x"));
        }else if(yStation.id<=0){
            responseParams.insert(QString("result"),QString("fail"));
            responseParams.insert(QString("info"),QString("not found station y"));
        }else if(zStation.id<=0){
            responseParams.insert(QString("result"),QString("fail"));
            responseParams.insert(QString("info"),QString("not found station z"));
        }else{
            int id = g_taskCenter->makePickupTask(iX,iY,iZ);
            responseParams.insert(QString("info"),QString(""));
            responseParams.insert(QString("result"),QString("success"));
            responseParams.insert(QString("id"),QString("%1").arg(id));
        }
    }
}

//创建任务(创建指定车辆经过Y点到X点的任务)
void UserMsgProcessor::Task_CreateAgvYToX(zmq::context_t *ctx, QMap<QString, QString> &requestDatas, QList<QMap<QString, QString> > &datalists,QMap<QString,QString> &responseParams,QList<QMap<QString,QString> > &responseDatalists){

    if(checkParamExistAndNotNull(requestDatas,responseParams,"x","y","z","agvid",NULL)){
        int iX = requestDatas["x"].toInt();
        int iY = requestDatas["y"].toInt();
        int iZ = requestDatas["z"].toInt();

        int agvId = requestDatas["agvid"].toInt();

        AgvStation xStation = g_agvMapCenter->getAgvStation(iX);
        AgvStation yStation = g_agvMapCenter->getAgvStation(iY);
        AgvStation zStation = g_agvMapCenter->getAgvStation(iZ);

        if(xStation.id<=0){
            responseParams.insert(QString("result"),QString("fail"));
            responseParams.insert(QString("info"),QString("not found station x"));
        }else if(yStation.id<=0){
            responseParams.insert(QString("result"),QString("fail"));
            responseParams.insert(QString("info"),QString("not found station y"));
        }else if(zStation.id<=0){
            responseParams.insert(QString("result"),QString("fail"));
            responseParams.insert(QString("info"),QString("not found station z"));
        }else if(g_m_agvs.contains(agvId)){
            responseParams.insert(QString("result"),QString("fail"));
            responseParams.insert(QString("info"),QString("not found agv"));
        }else{
            int id = g_taskCenter->makeAgvPickupTask(agvId,iX,iY,iZ);
            responseParams.insert(QString("info"),QString(""));
            responseParams.insert(QString("result"),QString("success"));
            responseParams.insert(QString("id"),QString("%1").arg(id));
        }
    }
}

//创建任务(创建指定车辆经过Y点到X点的任务)
void UserMsgProcessor::Task_CreateAgvYToXCircle(zmq::context_t *ctx, QMap<QString, QString> &requestDatas, QList<QMap<QString, QString> > &datalists,QMap<QString,QString> &responseParams,QList<QMap<QString,QString> > &responseDatalists){

    if(checkParamExistAndNotNull(requestDatas,responseParams,"x","y","z","agvid",NULL)){
        int iX = requestDatas["x"].toInt();
        int iY = requestDatas["y"].toInt();
        int iZ = requestDatas["z"].toInt();

        int agvId = requestDatas["agvid"].toInt();

        AgvStation xStation = g_agvMapCenter->getAgvStation(iX);
        AgvStation yStation = g_agvMapCenter->getAgvStation(iY);
        AgvStation zStation = g_agvMapCenter->getAgvStation(iZ);

        if(xStation.id<=0){
            responseParams.insert(QString("result"),QString("fail"));
            responseParams.insert(QString("info"),QString("not found station x"));
        }else if(yStation.id<=0){
            responseParams.insert(QString("result"),QString("fail"));
            responseParams.insert(QString("info"),QString("not found station y"));
        }else if(zStation.id<=0){
            responseParams.insert(QString("result"),QString("fail"));
            responseParams.insert(QString("info"),QString("not found station z"));
        }else if(g_m_agvs.contains(agvId)){
            responseParams.insert(QString("result"),QString("fail"));
            responseParams.insert(QString("info"),QString("not found agv"));
        }else{
            int id = g_taskCenter->makeLoopTask(agvId,iX,iY,iZ);
            responseParams.insert(QString("info"),QString(""));
            responseParams.insert(QString("result"),QString("success"));
            responseParams.insert(QString("id"),QString("%1").arg(id));
        }
    }
}
//查询任务状态
void UserMsgProcessor::Task_QueryStatus(zmq::context_t *ctx, QMap<QString, QString> &requestDatas, QList<QMap<QString, QString> > &datalists,QMap<QString,QString> &responseParams,QList<QMap<QString,QString> > &responseDatalists){
    if(checkParamExistAndNotNull(requestDatas,responseParams,"taskid",NULL)){
        int taskid = requestDatas["taskid"].toInt();
        int status = g_taskCenter->queryTaskStatus(taskid);

        responseParams.insert(QString("status"),QString("%1").arg(status));
        responseParams.insert(QString("info"),QString(""));
        responseParams.insert(QString("result"),QString("success"));
    }
}
//取消任务
void UserMsgProcessor::Task_Cancel(zmq::context_t *ctx, QMap<QString, QString> &requestDatas, QList<QMap<QString, QString> > &datalists,QMap<QString,QString> &responseParams,QList<QMap<QString,QString> > &responseDatalists){
    if(checkParamExistAndNotNull(requestDatas,responseParams,"taskid",NULL)){
        int taskid = requestDatas["taskid"].toInt();

        if(g_taskCenter->cancelTask(taskid)){
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
void UserMsgProcessor::Task_ListUnassigned(zmq::context_t *ctx, QMap<QString, QString> &requestDatas, QList<QMap<QString, QString> > &datalists,QMap<QString,QString> &responseParams,QList<QMap<QString,QString> > &responseDatalists){
    responseParams.insert(QString("info"),QString(""));
    responseParams.insert(QString("result"),QString("success"));
    //添加列表
    QList<Task *> tasks = g_taskCenter->getUnassignedTasks();
    for(QList<Task *>::iterator itr = tasks.begin();itr!=tasks.end();++itr){
        Task * task = *itr;
        QMap<QString,QString> onetask;

        onetask.insert(QString("id"),QString("%1").arg(task->id));
        onetask.insert(QString("produceTime"),task->produceTime.toString(DATE_TIME_FORMAT));
        onetask.insert(QString("excuteCar"),QString("%1").arg(task->excuteCar));
        onetask.insert(QString("status"),QString("%1").arg(task->status));

        responseDatalists.push_back(onetask);
    }

}
//正在执行任务列表
void UserMsgProcessor::Task_ListDoing(zmq::context_t *ctx, QMap<QString, QString> &requestDatas, QList<QMap<QString, QString> > &datalists,QMap<QString,QString> &responseParams,QList<QMap<QString,QString> > &responseDatalists){
    responseParams.insert(QString("info"),QString(""));
    responseParams.insert(QString("result"),QString("success"));
    //添加列表
    QList<Task *> tasks = g_taskCenter->getDoingTasks();
    for(QList<Task *>::iterator itr = tasks.begin();itr!=tasks.end();++itr){
        Task * task = *itr;
        QMap<QString,QString> onetask;

        onetask.insert(QString("id"),QString("%1").arg(task->id));
        onetask.insert(QString("produceTime"),task->produceTime.toString(DATE_TIME_FORMAT));
        onetask.insert(QString("doTime"),task->doTime.toString(DATE_TIME_FORMAT));
        onetask.insert(QString("excuteCar"),QString("%1").arg(task->excuteCar));
        onetask.insert(QString("status"),QString("%1").arg(task->status));

        responseDatalists.push_back(onetask);
    }
}
//已经完成任务列表(today)
void UserMsgProcessor::Task_ListDoneToday(zmq::context_t *ctx, QMap<QString, QString> &requestDatas, QList<QMap<QString, QString> > &datalists,QMap<QString,QString> &responseParams,QList<QMap<QString,QString> > &responseDatalists){
    responseParams.insert(QString("info"),QString(""));
    responseParams.insert(QString("result"),QString("success"));

    QString querySql = "select id,task_produceTime,task_doneTime,task_doTime,task_excuteCar,task_status from agv_task where task_status = ? and task_doneTime between ? and ?;";
    QDate today = QDate::currentDate();
    QDate tomorrow = today.addDays(1);
    QList<QVariant> params;
    params<<Task::AGV_TASK_STATUS_DONE;

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
void UserMsgProcessor::Task_ListDoneAll(zmq::context_t *ctx, QMap<QString, QString> &requestDatas, QList<QMap<QString, QString> > &datalists,QMap<QString,QString> &responseParams,QList<QMap<QString,QString> > &responseDatalists)
{
    responseParams.insert(QString("info"),QString(""));
    responseParams.insert(QString("result"),QString("success"));

    QString querySql = "select id,task_produceTime,task_doneTime,task_doTime,task_excuteCar,task_status from agv_task where task_status = ? ";

    QList<QVariant> params;
    params<<Task::AGV_TASK_STATUS_DONE;

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
void UserMsgProcessor::Task_ListDoneDuring(zmq::context_t *ctx, QMap<QString, QString> &requestDatas, QList<QMap<QString, QString> > &datalists,QMap<QString,QString> &responseParams,QList<QMap<QString,QString> > &responseDatalists)
{
    //要求带有from和to
    if(checkParamExistAndNotNull(requestDatas,responseParams,"from","to",NULL)){
        responseParams.insert(QString("info"),QString(""));
        responseParams.insert(QString("result"),QString("success"));

        QString querySql = "select id,task_produceTime,task_doneTime,task_doTime,task_excuteCar,task_status from agv_task where task_status = ? and task_doneTime between ? and ?;";
        QList<QVariant> params;
        params<<Task::AGV_TASK_STATUS_DONE;
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

void UserMsgProcessor::Task_Detail(zmq::context_t *ctx, QMap<QString, QString> &requestDatas, QList<QMap<QString, QString> > &datalists,QMap<QString,QString> &responseParams,QList<QMap<QString,QString> > &responseDatalists)
{
    //要求带有taskid
    bool needDelete = false;
    Task *task = NULL;
    if(checkParamExistAndNotNull(requestDatas,responseParams,"taskid",NULL)){
        int taskId = requestDatas["taskid"].toInt();

        task = g_taskCenter->queryUndoTask(taskId);
        if(task == NULL){
            task = g_taskCenter->queryDoingTask(taskId);
            if(task == NULL){
                task = g_taskCenter->queryDoneTask(taskId);
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
                responseParams.insert(QString("id"),QString("%1").arg(task->id));
                responseParams.insert(QString("produceTime"),task->produceTime.toString(DATE_TIME_FORMAT));
                responseParams.insert(QString("doneTime"),task->doneTime.toString(DATE_TIME_FORMAT));
                responseParams.insert(QString("doTime"),task->doTime.toString(DATE_TIME_FORMAT));
                responseParams.insert(QString("excuteCar"),QString("%1").arg(task->excuteCar));
                responseParams.insert(QString("status"),QString("%1").arg(task->status));
            }
            //            //装入节点
            //            for(int i=0;i<task->taskNodes.length();++i)
            //            {
            //                TaskNode *tn = task->taskNodes.at(i);

            //                QMap<QString,QString> node;

            //                node.insert(QString("status"),QString("%1").arg(tn->status));
            //                node.insert(QString("queueNumber"),QString("%1").arg(tn->queueNumber));
            //                node.insert(QString("aimStation"),QString("%1").arg(tn->aimStation));
            //                node.insert(QString("waitType"),QString("%1").arg(tn->waitType));
            //                node.insert(QString("waitTime"),QString("%1").arg(tn->waitTime));
            //                node.insert(QString("arriveTime"),tn->arriveTime.toString(DATE_TIME_FORMAT));
            //                node.insert(QString("leaveTime"),tn->leaveTime.toString(DATE_TIME_FORMAT));

            //                responseDatalists.push_back(node);
            //            }

            if(needDelete)
                delete task;
        }
    }
}

//查询日志 from to时间
void UserMsgProcessor::Log_ListDuring(zmq::context_t *ctx, QMap<QString, QString> &requestDatas, QList<QMap<QString, QString> > &datalists,QMap<QString,QString> &responseParams,QList<QMap<QString,QString> > &responseDatalists)
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
                params<<AGV_LOG_LEVEL_DEBUG;
                firstappend = false;
            }

            if(info){
                if(firstappend){
                    querySql += QString(" log_level=? ");
                }
                else{
                    querySql += QString("or log_level=? ");
                }
                params<<AGV_LOG_LEVEL_INFO;
                firstappend = false;
            }

            if(warn){
                if(firstappend){
                    querySql += QString(" log_level=? ");
                }
                else{
                    querySql += QString("or log_level=? ");
                }
                params<<AGV_LOG_LEVEL_WARN;
                firstappend = false;
            }

            if(error){
                if(firstappend){
                    querySql += QString(" log_level=? ");
                }
                else{
                    querySql += QString("or log_level=? ");
                }
                params<<AGV_LOG_LEVEL_ERROR;
                firstappend = false;
            }

            if(fatal){
                if(firstappend){
                    querySql += QString(" log_level=? ");
                }
                else{
                    querySql += QString("or log_level=? ");
                }
                params<<AGV_LOG_LEVEL_FATAL;
                firstappend = false;
            }
            querySql += ");";
        }

        qDebug() << "querySql="<<querySql;

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
void UserMsgProcessor::Log_ListAll(zmq::context_t *ctx, QMap<QString, QString> &requestDatas, QList<QMap<QString, QString> > &datalists,QMap<QString,QString> &responseParams,QList<QMap<QString,QString> > &responseDatalists)
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




