#include "usermsgprocessor.h"
#include "util/global.h"
#include "pugixml.hpp"
#include <sstream>
#include <iostream>
#include <QUuid>
#include <stdarg.h>


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

            if(item.data==NULL)continue;
            if(item.size<=0)
            {
                free(item.data);continue;
            }
            std::string ss((char *)item.data,item.size);
            free(item.data);
            item.data = NULL;
            qDebug()<<"ss======================"<<ss.length();
            qDebug()<<ss.c_str();
            qDebug()<<"ss======================";
            parseOneMsg(item,ss);
        }
        QyhSleep(50);
    }
}


std::string UserMsgProcessor::makeAccessToken()
{
    //生成一个随机的16位字符串
    return QUuid::createUuid().toString().replace("{","").replace("}","").toStdString();
}


void UserMsgProcessor::myquit()
{
    isQuit=true;
}

//对接收到的xml消息进行转换
//这里，采用速度最最快的pluginXml.以保证效率，解析单个xml的时间应该在3ms之内
void UserMsgProcessor::parseOneMsg(const QyhMsgDateItem &item, const std::string &oneMsg)
{
    std::map<std::string,std::string> params;
    std::vector<std::map<std::string,std::string> > datalist;
    //解析
    if(!getRequestParam(oneMsg,params,datalist))return ;

    //初步判断，如果不合格，那就直接淘汰
    std::map<std::string,std::string>::iterator itr;
    if((itr=params.find("type"))!=params.end()
            &&(itr=params.find("todo"))!=params.end()
            &&(itr=params.find("queuenumber"))!=params.end()){
        qDebug()<<"get a good msg"<<params.size()<<datalist.size();
        //接下来对这条消息进行响应
        responseOneMsg(item,params,datalist);
    }
}

bool UserMsgProcessor::checkParamExistAndNotNull(std::map<std::string,std::string> &requestDatas,std::map<std::string,std::string> &responseParams,const char* s,...)
{
    bool result = true;
    //argno代表第几个参数 para就是这个参数
    va_list argp;
    int argno = 0;

    char *para;
    va_start(argp, s);
    while (1)
    {
        para = va_arg(argp, char *);
        if (strcmp(para, "") == 0 )
            break;

        //检查参数
        std::string strParam = std::string(para);
        if(requestDatas.find(strParam)==requestDatas.end()){
            responseParams.insert(std::make_pair(std::string("info"),std::string("param lack:")+strParam));
            responseParams.insert(std::make_pair(std::string("result"),std::string("fail")));
            result =  false;
            break;
        }else if(requestDatas.at(strParam).length()<=0){
            responseParams.insert(std::make_pair(std::string("info"),std::string("null of:")+strParam));
            responseParams.insert(std::make_pair(std::string("result"),std::string("fail")));
            result =  false;
            break;
        }
        argno++;
    }
    va_end(argp);

    return result;
}

bool UserMsgProcessor::checkAccessToken(const QyhMsgDateItem &item,std::map<std::string,std::string> &requestDatas,std::map<std::string,std::string> &responseParams,LoginUserInfo &loginUserInfo)
{
    if(!checkParamExistAndNotNull(requestDatas,responseParams,"access_token")) return false;

    ////对access_token判断是否正确
    bool access_token_correct = false;

    for(std::list<LoginUserInfo>::iterator itr = loginUserIdSock.begin();itr!=loginUserIdSock.end();++itr){
        LoginUserInfo info = *itr;
        if(info.sock == item.sock && info.access_tocken == requestDatas["access_token"]){
            access_token_correct = true;
            loginUserInfo = info;
        }
    }

    if(!access_token_correct){
        //access_token错误
        responseParams.insert(std::make_pair(std::string("info"),std::string("not correct:access_token,please relogin!")));
        responseParams.insert(std::make_pair(std::string("result"),std::string("fail")));

        return false;
    }
    return true;
}

//对接收到的消息，进行处理！
void UserMsgProcessor::responseOneMsg(const QyhMsgDateItem &item,std::map<string,string> requestDatas,std::vector<std::map<std::string,std::string> > datalists)
{
    if(requestDatas["type"] == "user"){
        clientMsgUserProcess(item,requestDatas,datalists);
    }else if(requestDatas["type"] == "map"){
        clientMsgMapProcess(item,requestDatas,datalists);
    }else if(requestDatas["type"] == "agv"){
        clientMsgAgvProcess(item,requestDatas,datalists);
    }else if(requestDatas["type"] == "agvManagge"){
        clientMsgAgvManageProcess(item,requestDatas,datalists);
    }else if(requestDatas["type"] == "task"){
        clientMsgTaskProcess(item,requestDatas,datalists);
    }else if(requestDatas["type"] == "log"){
        clientMsgLogProcess(item,requestDatas,datalists);
    }
}

void UserMsgProcessor::clientMsgUserProcess(const QyhMsgDateItem &item,std::map<string,string> &requestDatas,std::vector<std::map<std::string,std::string> > &datalists)
{
    std::map<std::string,std::string> responseParams;
    std::vector<std::map<std::string,std::string> > responseDatalists;
    responseParams.insert(std::make_pair(std::string("type"),requestDatas["type"]));
    responseParams.insert(std::make_pair(std::string("todo"),requestDatas["todo"]));
    responseParams.insert(std::make_pair(std::string("queuenumber"),requestDatas["queuenumber"]));
    LoginUserInfo loginUserinfo;

    /////////////////////////////////////这段是和掐所有地方不应的一个点
    if(requestDatas["todo"]=="login")
    {
        User_Login(item,requestDatas,datalists,responseParams,responseDatalists,loginUserinfo);
        //封装        //发送
        std::string xml = getResponseXml(responseParams,responseDatalists);
        g_netWork->sendToOne(item.sock,xml.c_str(),xml.length());
        return ;
    }
    /////////////////////////////////////这段是和掐所有地方不应的一个点

    /////所有的非登录消息，需要进行，安全验证 随机码
    if(!checkAccessToken(item,requestDatas,responseParams,loginUserinfo)){
        std::string xml = getResponseXml(responseParams,responseDatalists);
        g_netWork->sendToOne(item.sock,xml.c_str(),xml.length());
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

    //封装
    std::string xml = getResponseXml(responseParams,responseDatalists);
    //发送
    g_netWork->sendToOne(item.sock,xml.c_str(),xml.length());
}

void UserMsgProcessor::clientMsgMapProcess(const QyhMsgDateItem &item,std::map<string,string> &requestDatas,std::vector<std::map<std::string,std::string> > &datalists)
{
    std::map<std::string,std::string> responseParams;
    std::vector<std::map<std::string,std::string> > responseDatalists;
    responseParams.insert(std::make_pair(std::string("type"),requestDatas["type"]));
    responseParams.insert(std::make_pair(std::string("todo"),requestDatas["todo"]));
    responseParams.insert(std::make_pair(std::string("queuenumber"),requestDatas["queuenumber"]));
    LoginUserInfo loginUserinfo;

    /////所有的非登录消息，需要进行，安全验证 随机码
    if(!checkAccessToken(item,requestDatas,responseParams,loginUserinfo)){
        std::string xml = getResponseXml(responseParams,responseDatalists);
        g_netWork->sendToOne(item.sock,xml.c_str(),xml.length());
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

    //封装
    std::string xml = getResponseXml(responseParams,responseDatalists);
    //发送
    g_netWork->sendToOne(item.sock,xml.c_str(),xml.length());

}

void UserMsgProcessor::clientMsgAgvProcess(const QyhMsgDateItem &item,std::map<string,string> &requestDatas,std::vector<std::map<std::string,std::string> > &datalists)
{
    std::map<std::string,std::string> responseParams;
    std::vector<std::map<std::string,std::string> > responseDatalists;
    responseParams.insert(std::make_pair(std::string("type"),requestDatas["type"]));
    responseParams.insert(std::make_pair(std::string("todo"),requestDatas["todo"]));
    responseParams.insert(std::make_pair(std::string("queuenumber"),requestDatas["queuenumber"]));
    LoginUserInfo loginUserinfo;

    /////所有的非登录消息，需要进行，安全验证 随机码
    if(!checkAccessToken(item,requestDatas,responseParams,loginUserinfo)){
        std::string xml = getResponseXml(responseParams,responseDatalists);
        g_netWork->sendToOne(item.sock,xml.c_str(),xml.length());
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
    std::string xml = getResponseXml(responseParams,responseDatalists);
    //发送
    g_netWork->sendToOne(item.sock,xml.c_str(),xml.length());

}

void UserMsgProcessor::clientMsgAgvManageProcess(const QyhMsgDateItem &item,std::map<string,string> &requestDatas,std::vector<std::map<std::string,std::string> > &datalists)
{
    std::map<std::string,std::string> responseParams;
    std::vector<std::map<std::string,std::string> > responseDatalists;
    responseParams.insert(std::make_pair(std::string("type"),requestDatas["type"]));
    responseParams.insert(std::make_pair(std::string("todo"),requestDatas["todo"]));
    responseParams.insert(std::make_pair(std::string("queuenumber"),requestDatas["queuenumber"]));
    LoginUserInfo loginUserinfo;

    /////所有的非登录消息，需要进行，安全验证 随机码
    if(!checkAccessToken(item,requestDatas,responseParams,loginUserinfo)){
        std::string xml = getResponseXml(responseParams,responseDatalists);
        g_netWork->sendToOne(item.sock,xml.c_str(),xml.length());
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
    std::string xml = getResponseXml(responseParams,responseDatalists);
    //发送
    g_netWork->sendToOne(item.sock,xml.c_str(),xml.length());
}

void UserMsgProcessor::clientMsgTaskProcess(const QyhMsgDateItem &item,std::map<string,string> &requestDatas,std::vector<std::map<std::string,std::string> > &datalists)
{
    std::map<std::string,std::string> responseParams;
    std::vector<std::map<std::string,std::string> > responseDatalists;
    responseParams.insert(std::make_pair(std::string("type"),requestDatas["type"]));
    responseParams.insert(std::make_pair(std::string("todo"),requestDatas["todo"]));
    responseParams.insert(std::make_pair(std::string("queuenumber"),requestDatas["queuenumber"]));
    LoginUserInfo loginUserinfo;

    /////所有的非登录消息，需要进行，安全验证 随机码
    if(!checkAccessToken(item,requestDatas,responseParams,loginUserinfo)){
        std::string xml = getResponseXml(responseParams,responseDatalists);
        g_netWork->sendToOne(item.sock,xml.c_str(),xml.length());
    }

    /// 创建任务(创建到X点的任务)
    if(requestDatas["todo"]=="toX"){
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
    std::string xml = getResponseXml(responseParams,responseDatalists);
    //发送
    g_netWork->sendToOne(item.sock,xml.c_str(),xml.length());
}

void UserMsgProcessor::clientMsgLogProcess(const QyhMsgDateItem &item,std::map<string,string> &requestDatas,std::vector<std::map<std::string,std::string> > &datalists)
{


}

//接下来是具体的业务
/////////////////////////////关于用户部分
//用户登录
void UserMsgProcessor::User_Login(const QyhMsgDateItem &item, std::map<std::string, std::string> &requestDatas, std::vector<std::map<std::string, std::string> > &datalists,std::map<std::string,std::string> &responseParams,std::vector<std::map<std::string,std::string> > &responseDatalists,LoginUserInfo &loginUserInfo)
{
    //////////////////请求登录(做特殊处理，因为这里不需要验证access_token!)
    if(checkParamExistAndNotNull(requestDatas,responseParams,"username","password")){//要求包含用户名和密码
        QString querySqlA = "select id,password,role from agv_user where username=?";
        QStringList params;
        params<<QString::fromStdString(requestDatas["username"]);
        QList<QStringList> queryresult = g_sql->query(querySqlA,params);
        if(queryresult.length()==0){
            responseParams.insert(std::make_pair(std::string("info"),std::string("not exist:username")));
            responseParams.insert(std::make_pair(std::string("result"),std::string("fail")));
        }else{
            if(queryresult.at(0).at(1) ==QString::fromStdString(requestDatas["password"])){
                //加入已登录的队伍中
                LoginUserInfo loginUserInfo;
                loginUserInfo.id = queryresult.at(0).at(0).toInt();
                loginUserInfo.sock = item.sock;
                loginUserInfo.access_tocken = makeAccessToken();
                loginUserInfo.password = requestDatas["username"];
                loginUserInfo.username = requestDatas["password"];
                loginUserInfo.role = queryresult.at(0).at(2).toInt();
                loginUserIdSock.push_back(loginUserInfo);

                //登录成功
                responseParams.insert(std::make_pair(std::string("info"),std::string("")));
                responseParams.insert(std::make_pair(std::string("result"),std::string("success")));
                responseParams.insert(std::make_pair(std::string("role"),queryresult.at(0).at(2).toStdString()));
                responseParams.insert(std::make_pair(std::string("id"),queryresult.at(0).at(0).toStdString()));
                responseParams.insert(std::make_pair(std::string("access_token"),loginUserInfo.access_tocken));

                //设置登录状态和登录时间
                QString updateSql = "update agv_user set status=1,lastSignTime=? where id=?";
                params.clear();
                params<<QDateTime::currentDateTime().toString()<<QString("%1").arg(loginUserInfo.id);
                g_sql->exec(updateSql,params);
            }else{
                //登录失败
                responseParams.insert(std::make_pair(std::string("info"),std::string("not correct:password")));
                responseParams.insert(std::make_pair(std::string("result"),std::string("fail")));
            }
        }
    }
}
//用户登出
void UserMsgProcessor:: User_Logout(const QyhMsgDateItem &item, std::map<std::string, std::string> &requestDatas, std::vector<std::map<std::string, std::string> > &datalists,std::map<std::string,std::string> &responseParams,std::vector<std::map<std::string,std::string> > &responseDatalists,LoginUserInfo &loginUserInfo){
    //////////////////退出登录
    if(checkParamExistAndNotNull(requestDatas,responseParams,"id")){
        //取消的订阅
        g_msgCenter.removeAgvPositionSubscribe(item.sock);
        g_msgCenter.removeAgvStatusSubscribe(item.sock);
        //设置它的数据库中的状态
        QString updateSql = "update agv_user set status = 0 where id=?";
        QStringList param;
        param<<QString::fromStdString(requestDatas.at("id"));
        g_sql->exec(updateSql,param);
        responseParams.insert(std::make_pair(std::string("info"),std::string("")));
        responseParams.insert(std::make_pair(std::string("result"),std::string("success")));
    }
}
//修改密码
void UserMsgProcessor:: User_ChangePassword(const QyhMsgDateItem &item, std::map<std::string, std::string> &requestDatas, std::vector<std::map<std::string, std::string> > &datalists,std::map<std::string,std::string> &responseParams,std::vector<std::map<std::string,std::string> > &responseDatalists,LoginUserInfo &loginUserInfo){
    ///////////////////修改密码
    if(checkParamExistAndNotNull(requestDatas,responseParams,"username")&& checkParamExistAndNotNull(requestDatas,responseParams,"oldpassword")&& checkParamExistAndNotNull(requestDatas,responseParams,"newpassword")){
        QString querySqlA = "select password from agv_user where username=?";
        QStringList params;
        params<<QString::fromStdString(requestDatas["username"]);
        QList<QStringList> queryresult = g_sql->query(querySqlA,params);
        if(queryresult.length()==0){
            responseParams.insert(std::make_pair(std::string("info"),std::string("not exist:username.")));
            responseParams.insert(std::make_pair(std::string("result"),std::string("fail")));
        }else{
            if(queryresult.at(0).at(0)==QString::fromStdString(requestDatas["oldpassword"])){
                /////TODO:设置新的密码
                QString updateSql = "update agv_user set password=? where username=?";
                params.clear();
                params<<QString::fromStdString(requestDatas["newpassword"])<<QString::fromStdString(requestDatas["username"]);
                if(!g_sql->exec(updateSql,params)){
                    responseParams.insert(std::make_pair(std::string("info"),std::string("")));
                    responseParams.insert(std::make_pair(std::string("result"),std::string("success")));
                }else{
                    responseParams.insert(std::make_pair(std::string("info"),std::string("sql exec fail")));
                    responseParams.insert(std::make_pair(std::string("result"),std::string("fail")));
                }
            }else{
                responseParams.insert(std::make_pair(std::string("info"),std::string("not correct:oldpassword.")));
                responseParams.insert(std::make_pair(std::string("result"),std::string("fail")));
            }
        }
    }

}
//用户列表
void UserMsgProcessor:: User_List(const QyhMsgDateItem &item, std::map<std::string, std::string> &requestDatas, std::vector<std::map<std::string, std::string> > &datalists,std::map<std::string,std::string> &responseParams,std::vector<std::map<std::string,std::string> > &responseDatalists,LoginUserInfo &loginUserInfo){
    ////////////////////////获取用户列表
    //开始查询用户 只能查看到同等级或者低等级的用户
    QString querySqlB = "select id,username,password,status,lastSignTime,createTime,role from agv_user where role<=?";
    QStringList paramsB;
    paramsB<<QString("%1").arg(loginUserInfo.role);
    QList<QStringList> queryresultB = g_sql->query(querySqlB,paramsB);

    responseParams.insert(std::make_pair(std::string("info"),std::string("")));
    responseParams.insert(std::make_pair(std::string("result"),std::string("success")));

    for(int i=0;i<queryresultB.length();++i)
    {
        if(queryresultB.at(i).length() == 7)
        {
            std::map<std::string,std::string> userinfo;
            userinfo.insert(std::make_pair(std::string("id"),queryresultB.at(i).at(0).toStdString()));
            userinfo.insert(std::make_pair(std::string("username"),queryresultB.at(i).at(1).toStdString()));
            userinfo.insert(std::make_pair(std::string("password"),queryresultB.at(i).at(2).toStdString()));
            userinfo.insert(std::make_pair(std::string("status"),queryresultB.at(i).at(3).toStdString()));
            userinfo.insert(std::make_pair(std::string("lastSignTime"),queryresultB.at(i).at(4).toStdString()));
            userinfo.insert(std::make_pair(std::string("createTime"),queryresultB.at(i).at(5).toStdString()));
            userinfo.insert(std::make_pair(std::string("role"),queryresultB.at(i).at(6).toStdString()));
            responseDatalists.push_back(userinfo);
        }
    }
}
//删除用户
void UserMsgProcessor:: User_Delete(const QyhMsgDateItem &item, std::map<std::string, std::string> &requestDatas, std::vector<std::map<std::string, std::string> > &datalists,std::map<std::string,std::string> &responseParams,std::vector<std::map<std::string,std::string> > &responseDatalists,LoginUserInfo &loginUserInfo){
    ////////////////////////////////删除用户
    if(checkParamExistAndNotNull(requestDatas,responseParams,"id")){
        QString deleteSql = "delete from agv_user where id=?";
        QStringList params;
        params<<QString::fromStdString(requestDatas["id"]);
        if(!g_sql->exec(deleteSql,params)){
            responseParams.insert(std::make_pair(std::string("info"),std::string("delete fail for sql fail")));
            responseParams.insert(std::make_pair(std::string("result"),std::string("fail")));
        }else{
            responseParams.insert(std::make_pair(std::string("info"),std::string("")));
            responseParams.insert(std::make_pair(std::string("result"),std::string("success")));
        }
    }
}
//添加用户
void UserMsgProcessor:: User_Add(const QyhMsgDateItem &item, std::map<std::string, std::string> &requestDatas, std::vector<std::map<std::string, std::string> > &datalists,std::map<std::string,std::string> &responseParams,std::vector<std::map<std::string,std::string> > &responseDatalists,LoginUserInfo &loginUserInfo){
    ///////////////////////////////////添加用户
    if(checkParamExistAndNotNull(requestDatas,responseParams,"username")
            &&checkParamExistAndNotNull(requestDatas,responseParams,"password")
            &&checkParamExistAndNotNull(requestDatas,responseParams,"role"))
    {
        QString username =QString::fromStdString(requestDatas.at("username"));
        QString password =QString::fromStdString(requestDatas.at("password"));
        QString role =QString::fromStdString( requestDatas.at("role"));

        //查看剩余项目是否存在
        QString realName="";
        bool sex = true;
        int age=20;
        if(requestDatas.find("realname")!=requestDatas.end() &&requestDatas.at("realname").length()>0){
            realName = QString::fromStdString(requestDatas.at("realname"));
        }
        if(requestDatas.find("sex")!=requestDatas.end() &&requestDatas.at("sex").length()>0){
            sex = requestDatas.at("sex")=="1";
        }
        if(requestDatas.find("age")!=requestDatas.end() &&requestDatas.at("age").length()>0){
            age = QString::fromStdString(requestDatas.at("sex")).toInt();
        }

        QString addSql = "insert into agv_user(username,password,role,realname,sex,age,createTime)values(?,?,?,?,?,?,?);";
        QStringList params;
        params<<username<<password<<role<<realName<<QString("%1").arg(sex)<<QString("%1").arg(age)<<QDateTime::currentDateTime().toString();
        if(g_sql->exec(addSql,params)){
            //成功
            responseParams.insert(std::make_pair(std::string("info"),std::string("")));
            responseParams.insert(std::make_pair(std::string("result"),std::string("success")));
        }else{
            //失败
            responseParams.insert(std::make_pair(std::string("info"),std::string("sql insert into fail")));
            responseParams.insert(std::make_pair(std::string("result"),std::string("fail")));
        }
    }

}

/////////////////////////////关于地图部分
//地图 站点列表
void UserMsgProcessor:: Map_StationList(const QyhMsgDateItem &item, std::map<std::string, std::string> &requestDatas, std::vector<std::map<std::string, std::string> > &datalists,std::map<std::string,std::string> &responseParams,std::vector<std::map<std::string,std::string> > &responseDatalists,LoginUserInfo &loginUserInfo){
    responseParams.insert(std::make_pair(std::string("info"),std::string("")));
    responseParams.insert(std::make_pair(std::string("result"),std::string("success")));
    for(QMap<int,AgvStation *>::iterator itr=g_m_stations.begin();itr!=g_m_stations.end();++itr){
        std::map<string,string> list;

        std::stringstream ssX;
        std::string strX;
        ssX<<itr.value()->x();
        ssX>>strX;
        list.insert(std::make_pair(std::string("x"),strX));

        std::stringstream ssY;
        std::string strY;
        ssY<<itr.value()->y();
        ssY>>strY;
        list.insert(std::make_pair(std::string("y"),strY));

        std::stringstream ssType;
        std::string strType;
        ssType<<itr.value()->type();
        ssType>>strType;
        list.insert(std::make_pair(std::string("type"),strType));

        list.insert(std::make_pair(std::string("name"),itr.value()->name().toStdString()));

        std::stringstream ssId;
        std::string strId;
        ssId<<itr.value()->id();
        ssId>>strId;
        list.insert(std::make_pair(std::string("id"),strId));

        std::stringstream ssRfid;
        std::string strRfid;
        ssRfid<<itr.value()->rfid();
        ssRfid>>strRfid;
        list.insert(std::make_pair(std::string("rfid"),strRfid));

        responseDatalists.push_back(list);
    }
}
//地图 线路列表
void UserMsgProcessor:: Map_LineList(const QyhMsgDateItem &item, std::map<std::string, std::string> &requestDatas, std::vector<std::map<std::string, std::string> > &datalists,std::map<std::string,std::string> &responseParams,std::vector<std::map<std::string,std::string> > &responseDatalists,LoginUserInfo &loginUserInfo){
    responseParams.insert(std::make_pair(std::string("info"),std::string("")));
    responseParams.insert(std::make_pair(std::string("result"),std::string("success")));
    for(QMap<int,AgvLine *>::iterator itr=g_m_lines.begin();itr!=g_m_lines.end();++itr){
        std::map<string,string> list;

        std::stringstream ss_startX;
        std::string str_startX;
        ss_startX<<itr.value()->startX();
        ss_startX>>str_startX;
        list.insert(std::make_pair(std::string("startX"),str_startX));

        std::stringstream ss_startY;
        std::string str_startY;
        ss_startY<<itr.value()->startY();
        ss_startY>>str_startY;
        list.insert(std::make_pair(std::string("startY"),str_startY));

        std::stringstream ss_endX;
        std::string str_endX;
        ss_endX<<itr.value()->endX();
        ss_endX>>str_endX;
        list.insert(std::make_pair(std::string("endX"),str_endX));

        std::stringstream ss_endY;
        std::string str_endY;
        ss_endY<<itr.value()->endY();
        ss_endY>>str_endY;
        list.insert(std::make_pair(std::string("endY"),str_endY));

        std::stringstream ss_radius;
        std::string str_radius;
        ss_radius<<itr.value()->radius();
        ss_radius>>str_radius;
        list.insert(std::make_pair(std::string("radius"),str_radius));

        std::stringstream ss_clockwise;
        std::string str_clockwise;
        ss_clockwise<<itr.value()->clockwise();
        ss_clockwise>>str_clockwise;
        list.insert(std::make_pair(std::string("clockwise"),str_clockwise));

        std::stringstream ss_line;
        std::string str_line;
        ss_line<<itr.value()->line();
        ss_line>>str_line;
        list.insert(std::make_pair(std::string("line"),str_line));

        std::stringstream ss_midX;
        std::string str_midX;
        ss_midX<<itr.value()->midX();
        ss_midX>>str_midX;
        list.insert(std::make_pair(std::string("midX"),str_midX));

        std::stringstream ss_midY;
        std::string str_midY;
        ss_midY<<itr.value()->midY();
        ss_midY>>str_midY;
        list.insert(std::make_pair(std::string("midY"),str_midY));

        std::stringstream ss_centerX;
        std::string str_centerX;
        ss_centerX<<itr.value()->centerX();
        ss_centerX>>str_centerX;
        list.insert(std::make_pair(std::string("centerX"),str_centerX));

        std::stringstream ss_centerY;
        std::string str_centerY;
        ss_centerY<<itr.value()->centerY();
        ss_centerY>>str_centerY;
        list.insert(std::make_pair(std::string("centerY"),str_centerY));

        std::stringstream ss_angle;
        std::string str_angle;
        ss_angle<<itr.value()->angle();
        ss_angle>>str_angle;
        list.insert(std::make_pair(std::string("angle"),str_angle));

        std::stringstream ss_id;
        std::string str_id;
        ss_id<<itr.value()->id();
        ss_id>>str_id;
        list.insert(std::make_pair(std::string("id"),str_id));

        std::stringstream ss_draw;
        std::string str_draw;
        ss_draw<<itr.value()->draw();
        ss_draw>>str_draw;
        list.insert(std::make_pair(std::string("draw"),str_draw));

        std::stringstream ss_length;
        std::string str_length;
        ss_length<<itr.value()->length();
        ss_length>>str_length;
        list.insert(std::make_pair(std::string("length"),str_length));

        std::stringstream ss_startStation;
        std::string str_startStation;
        ss_startStation<<itr.value()->startStation();
        ss_startStation>>str_startStation;
        list.insert(std::make_pair(std::string("startStation"),str_startStation));

        std::stringstream ss_endStation;
        std::string str_endStation;
        ss_endStation<<itr.value()->endStation();
        ss_endStation>>str_endStation;
        list.insert(std::make_pair(std::string("endStation"),str_endStation));

        std::stringstream ss_rate;
        std::string str_rate;
        ss_rate<<itr.value()->rate();
        ss_rate>>str_rate;
        list.insert(std::make_pair(std::string("rate"),str_rate));

        responseDatalists.push_back(list);
    }

}
//订阅车辆位置信息
void UserMsgProcessor:: Map_AgvPositionSubscribe(const QyhMsgDateItem &item, std::map<std::string, std::string> &requestDatas, std::vector<std::map<std::string, std::string> > &datalists,std::map<std::string,std::string> &responseParams,std::vector<std::map<std::string,std::string> > &responseDatalists,LoginUserInfo &loginUserInfo){
    //将sock加入到车辆位置订阅者丢列中
    if(g_msgCenter.addAgvPostionSubscribe(item.sock)){
        responseParams.insert(std::make_pair(std::string("info"),std::string("")));
        responseParams.insert(std::make_pair(std::string("result"),std::string("success")));
    }else{
        responseParams.insert(std::make_pair(std::string("info"),std::string("unknow error")));
        responseParams.insert(std::make_pair(std::string("result"),std::string("fail")));
    }

}
//取消订阅车辆位置信息
void UserMsgProcessor:: Map_AgvPositionCancelSubscribe(const QyhMsgDateItem &item, std::map<std::string, std::string> &requestDatas, std::vector<std::map<std::string, std::string> > &datalists,std::map<std::string,std::string> &responseParams,std::vector<std::map<std::string,std::string> > &responseDatalists,LoginUserInfo &loginUserInfo){
    //将sock加入到车辆位置订阅者丢列中
    if(g_msgCenter.removeAgvPositionSubscribe(item.sock)){
        responseParams.insert(std::make_pair(std::string("info"),std::string("")));
        responseParams.insert(std::make_pair(std::string("result"),std::string("success")));
    }else{
        responseParams.insert(std::make_pair(std::string("info"),std::string("unknow error")));
        responseParams.insert(std::make_pair(std::string("result"),std::string("fail")));
    }

}

/////////////////////////////关于手控部分
//请求小车控制权
void UserMsgProcessor:: Agv_Hand(const QyhMsgDateItem &item, std::map<std::string, std::string> &requestDatas, std::vector<std::map<std::string, std::string> > &datalists,std::map<std::string,std::string> &responseParams,std::vector<std::map<std::string,std::string> > &responseDatalists,LoginUserInfo &loginUserInfo){
    ////该用户要求控制id为agvid的小车，
    if(checkParamExistAndNotNull(requestDatas,responseParams,"agvid")){
        int iAgvId;
        std::stringstream ss;
        ss<<requestDatas["agvid"];
        ss>>iAgvId;
        //查找这辆车
        Agv *agv = NULL;
        for(QMap<int,Agv *>::iterator itr = g_m_agvs.begin();itr!=g_m_agvs.end();++itr){
            if(itr.value()->id() == iAgvId){
                agv = itr.value();
                break;
            }
        }
        if(agv==NULL){
            //不存在这辆车
            responseParams.insert(std::make_pair(std::string("info"),std::string("not found agv with id:")+requestDatas["agvid"]));
            responseParams.insert(std::make_pair(std::string("result"),std::string("fail")));
        }else if(agv->currentHandUser == loginUserInfo.id||agv->currentHandUser <= 0){
            //OK
            agv->currentHandUser = loginUserInfo.id;
            agv->currentHandUserRole = loginUserInfo.role;
            responseParams.insert(std::make_pair(std::string("info"),std::string("")));
            responseParams.insert(std::make_pair(std::string("result"),std::string("success")));
        }else{
            //判断两个用户的权限，如果申请的人更高，OK。如果没有更高的权限，失败
            if(agv->currentHandUserRole<loginUserInfo.role){
                //新用户权限更高
                agv->currentHandUser = loginUserInfo.id;
                agv->currentHandUserRole = loginUserInfo.role;
                responseParams.insert(std::make_pair(std::string("info"),std::string("")));
                responseParams.insert(std::make_pair(std::string("result"),std::string("success")));
            }else{
                //新用户权限并不高//旧用户继续占用这辆车的手动控制权
                responseParams.insert(std::make_pair(std::string("info"),std::string("agv already handed by other user")));
                responseParams.insert(std::make_pair(std::string("result"),std::string("fail")));
            }
        }
    }
}
//释放小车控制权
void UserMsgProcessor:: Agv_Release(const QyhMsgDateItem &item, std::map<std::string, std::string> &requestDatas, std::vector<std::map<std::string, std::string> > &datalists,std::map<std::string,std::string> &responseParams,std::vector<std::map<std::string,std::string> > &responseDatalists,LoginUserInfo &loginUserInfo){
    ////该用户要求控制id为agvid的小车，
    if(checkParamExistAndNotNull(requestDatas,responseParams,"agvid")){
        int iAgvId;
        std::stringstream ss;
        ss<<requestDatas["agvid"];
        ss>>iAgvId;
        //查找这辆车
        Agv *agv = NULL;
        for(QMap<int,Agv *>::iterator itr = g_m_agvs.begin();itr!=g_m_agvs.end();++itr){
            if(itr.value()->id() == iAgvId){
                agv = itr.value();
                break;
            }
        }
        if(agv==NULL){
            //不存在这辆车
            responseParams.insert(std::make_pair(std::string("info"),std::string("not found agv with id:")+requestDatas["agvid"]));
            responseParams.insert(std::make_pair(std::string("result"),std::string("fail")));
        }else if(agv->currentHandUser == loginUserInfo.id){
            //OK
            agv->currentHandUser=0;
            agv->currentHandUserRole=0;
            responseParams.insert(std::make_pair(std::string("info"),std::string("")));
            responseParams.insert(std::make_pair(std::string("result"),std::string("success")));
        }else{
            //这辆车并不受你控制
            responseParams.insert(std::make_pair(std::string("info"),std::string("agv is not under your control")));
            responseParams.insert(std::make_pair(std::string("result"),std::string("fail")));
        }
    }
}
//前进
void UserMsgProcessor:: Agv_Forward(const QyhMsgDateItem &item, std::map<std::string, std::string> &requestDatas, std::vector<std::map<std::string, std::string> > &datalists,std::map<std::string,std::string> &responseParams,std::vector<std::map<std::string,std::string> > &responseDatalists,LoginUserInfo &loginUserInfo){
    ////该用户要求控制id为agvid的小车，
    if(checkParamExistAndNotNull(requestDatas,responseParams,"agvid","speed")){
        int iAgvId;
        std::stringstream ss;
        ss<<requestDatas["agvid"];
        ss>>iAgvId;
        int iSpeed;
        std::stringstream ssSpeed;
        ssSpeed<<requestDatas["speed"];
        ssSpeed>>iSpeed;
        //查找这辆车
        Agv *agv = NULL;
        for(QMap<int,Agv *>::iterator itr = g_m_agvs.begin();itr!=g_m_agvs.end();++itr){
            if(itr.value()->id() == iAgvId){
                agv = itr.value();
                break;
            }
        }
        if(agv==NULL){
            //不存在这辆车
            responseParams.insert(std::make_pair(std::string("info"),std::string("not found agv with id:")+requestDatas["agvid"]));
            responseParams.insert(std::make_pair(std::string("result"),std::string("fail")));
        }else if(agv->currentHandUser == loginUserInfo.id){
            //OK
            //下发指令
            g_msgCenter.handControlCmd(iAgvId,AGV_HAND_TYPE_FORWARD,iSpeed);
        }else{
            //这辆车并不受你控制
            responseParams.insert(std::make_pair(std::string("info"),std::string("agv is not under your control")));
            responseParams.insert(std::make_pair(std::string("result"),std::string("fail")));
        }
    }
}

//后退
void UserMsgProcessor:: Agv_Backward(const QyhMsgDateItem &item, std::map<std::string, std::string> &requestDatas, std::vector<std::map<std::string, std::string> > &datalists,std::map<std::string,std::string> &responseParams,std::vector<std::map<std::string,std::string> > &responseDatalists,LoginUserInfo &loginUserInfo){
    ////该用户要求控制id为agvid的小车，
    if(checkParamExistAndNotNull(requestDatas,responseParams,"agvid","speed")){
        int iAgvId;
        std::stringstream ss;
        ss<<requestDatas["agvid"];
        ss>>iAgvId;
        int iSpeed;
        std::stringstream ssSpeed;
        ssSpeed<<requestDatas["speed"];
        ssSpeed>>iSpeed;
        //查找这辆车
        Agv *agv = NULL;
        for(QMap<int,Agv *>::iterator itr = g_m_agvs.begin();itr!=g_m_agvs.end();++itr){
            if(itr.value()->id() == iAgvId){
                agv = itr.value();
                break;
            }
        }
        if(agv==NULL){
            //不存在这辆车
            responseParams.insert(std::make_pair(std::string("info"),std::string("not found agv with id:")+requestDatas["agvid"]));
            responseParams.insert(std::make_pair(std::string("result"),std::string("fail")));
        }else if(agv->currentHandUser == loginUserInfo.id){
            //OK
            //下发指令
            g_msgCenter.handControlCmd(iAgvId,AGV_HAND_TYPE_BACKWARD,iSpeed);
        }else{
            //这辆车并不受你控制
            responseParams.insert(std::make_pair(std::string("info"),std::string("agv is not in your hand")));
            responseParams.insert(std::make_pair(std::string("result"),std::string("fail")));
        }
    }
}
//左转
void UserMsgProcessor:: Agv_Turnleft(const QyhMsgDateItem &item, std::map<std::string, std::string> &requestDatas, std::vector<std::map<std::string, std::string> > &datalists,std::map<std::string,std::string> &responseParams,std::vector<std::map<std::string,std::string> > &responseDatalists,LoginUserInfo &loginUserInfo){
    ////该用户要求控制id为agvid的小车，
    if(checkParamExistAndNotNull(requestDatas,responseParams,"agvid","speed")){
        int iAgvId;
        std::stringstream ss;
        ss<<requestDatas["agvid"];
        ss>>iAgvId;
        int iSpeed;
        std::stringstream ssSpeed;
        ssSpeed<<requestDatas["speed"];
        ssSpeed>>iSpeed;
        //查找这辆车
        Agv *agv = NULL;
        for(QMap<int,Agv *>::iterator itr = g_m_agvs.begin();itr!=g_m_agvs.end();++itr){
            if(itr.value()->id() == iAgvId){
                agv = itr.value();
                break;
            }
        }
        if(agv==NULL){
            //不存在这辆车
            responseParams.insert(std::make_pair(std::string("info"),std::string("not found agv with id:")+requestDatas["agvid"]));
            responseParams.insert(std::make_pair(std::string("result"),std::string("fail")));
        }else if(agv->currentHandUser == loginUserInfo.id){
            //OK
            //下发指令
            g_msgCenter.handControlCmd(iAgvId,AGV_HAND_TYPE_TURNLEFT,iSpeed);
        }else{
            //这辆车并不受你控制
            responseParams.insert(std::make_pair(std::string("info"),std::string("agv is not in your hand")));
            responseParams.insert(std::make_pair(std::string("result"),std::string("fail")));
        }
    }
}
//右转
void UserMsgProcessor:: Agv_Turnright(const QyhMsgDateItem &item, std::map<std::string, std::string> &requestDatas, std::vector<std::map<std::string, std::string> > &datalists,std::map<std::string,std::string> &responseParams,std::vector<std::map<std::string,std::string> > &responseDatalists,LoginUserInfo &loginUserInfo){
    ////该用户要求控制id为agvid的小车，
    if(checkParamExistAndNotNull(requestDatas,responseParams,"agvid","speed")){
        int iAgvId;
        std::stringstream ss;
        ss<<requestDatas["agvid"];
        ss>>iAgvId;
        int iSpeed;
        std::stringstream ssSpeed;
        ssSpeed<<requestDatas["speed"];
        ssSpeed>>iSpeed;
        //查找这辆车
        Agv *agv = NULL;
        for(QMap<int,Agv *>::iterator itr = g_m_agvs.begin();itr!=g_m_agvs.end();++itr){
            if(itr.value()->id() == iAgvId){
                agv = itr.value();
                break;
            }
        }
        if(agv==NULL){
            //不存在这辆车
            responseParams.insert(std::make_pair(std::string("info"),std::string("not found agv with id:")+requestDatas["agvid"]));
            responseParams.insert(std::make_pair(std::string("result"),std::string("fail")));
        }else if(agv->currentHandUser == loginUserInfo.id){
            //OK
            //下发指令
            g_msgCenter.handControlCmd(iAgvId,AGV_HAND_TYPE_TURNRIGHT,iSpeed);
        }else{
            //这辆车并不受你控制
            responseParams.insert(std::make_pair(std::string("info"),std::string("agv is not in your hand")));
            responseParams.insert(std::make_pair(std::string("result"),std::string("fail")));
        }
    }
}
//灯带
void UserMsgProcessor:: Agv_Light(const QyhMsgDateItem &item, std::map<std::string, std::string> &requestDatas, std::vector<std::map<std::string, std::string> > &datalists,std::map<std::string,std::string> &responseParams,std::vector<std::map<std::string,std::string> > &responseDatalists,LoginUserInfo &loginUserInfo){}
//小车状态订阅
void UserMsgProcessor:: Agv_StatusSubscribte(const QyhMsgDateItem &item, std::map<std::string, std::string> &requestDatas, std::vector<std::map<std::string, std::string> > &datalists,std::map<std::string,std::string> &responseParams,std::vector<std::map<std::string,std::string> > &responseDatalists,LoginUserInfo &loginUserInfo){
    ////订阅id未agvid的小车的状态信息
    if(checkParamExistAndNotNull(requestDatas,responseParams,"agvid")){
        int iAgvId;
        std::stringstream ss;
        ss<<requestDatas["agvid"];
        ss>>iAgvId;
        //查找这辆车
        Agv *agv = NULL;
        for(QMap<int,Agv *>::iterator itr = g_m_agvs.begin();itr!=g_m_agvs.end();++itr){
            if(itr.value()->id() == iAgvId){
                agv = itr.value();
                break;
            }
        }
        if(agv==NULL){
            //不存在这辆车
            responseParams.insert(std::make_pair(std::string("info"),std::string("not found agv with id:")+requestDatas["agvid"]));
            responseParams.insert(std::make_pair(std::string("result"),std::string("fail")));
        }else if(agv->currentHandUser == loginUserInfo.id){
            //OK
            //下发指令
            g_msgCenter.addAgvStatusSubscribe(item.sock,iAgvId);
            responseParams.insert(std::make_pair(std::string("info"),std::string("")));
            responseParams.insert(std::make_pair(std::string("result"),std::string("success")));
        }
    }
}

//取消小车状态订阅
void UserMsgProcessor:: Agv_CancelStatusSubscribe(const QyhMsgDateItem &item, std::map<std::string, std::string> &requestDatas, std::vector<std::map<std::string, std::string> > &datalists,std::map<std::string,std::string> &responseParams,std::vector<std::map<std::string,std::string> > &responseDatalists,LoginUserInfo &loginUserInfo){
    ////该用户要求控制id为agvid的小车，
    if(checkParamExistAndNotNull(requestDatas,responseParams,"agvid")){
        int iAgvId;
        std::stringstream ss;
        ss<<requestDatas["agvid"];
        ss>>iAgvId;

        //查找这辆车
        Agv *agv = NULL;
        for(QMap<int,Agv *>::iterator itr = g_m_agvs.begin();itr!=g_m_agvs.end();++itr){
            if(itr.value()->id() == iAgvId){
                agv = itr.value();
                break;
            }
        }
        if(agv==NULL){
            //不存在这辆车
            responseParams.insert(std::make_pair(std::string("info"),std::string("not found agv with id:")+requestDatas["agvid"]));
            responseParams.insert(std::make_pair(std::string("result"),std::string("fail")));
        }else if(agv->currentHandUser == loginUserInfo.id){
            //OK
            //下发指令
            g_msgCenter.removeAgvStatusSubscribe(item.sock,iAgvId);
            responseParams.insert(std::make_pair(std::string("info"),std::string("")));
            responseParams.insert(std::make_pair(std::string("result"),std::string("success")));
        }
    }
}

/////////////////////////////////车辆管理部分
//列表
void UserMsgProcessor:: AgvManage_List(const QyhMsgDateItem &item, std::map<std::string, std::string> &requestDatas, std::vector<std::map<std::string, std::string> > &datalists,std::map<std::string,std::string> &responseParams,std::vector<std::map<std::string,std::string> > &responseDatalists,LoginUserInfo &loginUserInfo){
    responseParams.insert(std::make_pair(std::string("info"),std::string("")));
    responseParams.insert(std::make_pair(std::string("result"),std::string("success")));
    for(QMap<int,Agv *>::iterator itr = g_m_agvs.begin();itr!=g_m_agvs.end();++itr){
        std::map<std::string,std::string> list;
        Agv *agv = itr.value();

        std::stringstream ss_id;
        std::string str_id;
        ss_id<<agv->id();
        ss_id>>str_id;
        list.insert(std::make_pair(std::string("id"),str_id));

        std::stringstream ss_name;
        std::string str_name;
        ss_name<<agv->name().toStdString();
        ss_name>>str_name;
        list.insert(std::make_pair(std::string("name"),str_name));

        std::stringstream ss_ip;
        std::string str_ip;
        ss_ip<<agv->ip().toStdString();
        ss_ip>>str_ip;
        list.insert(std::make_pair(std::string("ip"),str_ip));

        responseDatalists.push_back(list);
    }
}
//增加
void UserMsgProcessor:: AgvManage_Add(const QyhMsgDateItem &item, std::map<std::string, std::string> &requestDatas, std::vector<std::map<std::string, std::string> > &datalists,std::map<std::string,std::string> &responseParams,std::vector<std::map<std::string,std::string> > &responseDatalists,LoginUserInfo &loginUserInfo){
    //要求name和ip
    if(checkParamExistAndNotNull(requestDatas,responseParams,"name","ip")){
        QString insertSql = "insert into agv_agv (name,ip)values(?,?)";
        QStringList tempParams;
        tempParams<<QString::fromStdString(requestDatas.at("name"))<<QString::fromStdString(requestDatas.at("ip"));
        if(g_sql->exec(insertSql,tempParams)){
            int newId;
            QString querySql = "select id from agv_agv where name = ? and ip = ?";
            QList<QStringList> queryresult = g_sql->query(querySql,tempParams);
            if(queryresult.length()>0 &&queryresult.at(0).length()>0)
            {
                newId = queryresult.at(0).at(0).toInt();
                Agv *agv = new Agv;
                agv->setId(newId);
                agv->setName(QString::fromStdString(requestDatas.at("name")));
                agv->setIp(QString::fromStdString(requestDatas.at("ip")));
                g_m_agvs.insert(newId,agv);
                responseParams.insert(std::make_pair(std::string("info"),std::string("")));
                responseParams.insert(std::make_pair(std::string("result"),std::string("success")));
                responseParams.insert(std::make_pair(std::string("id"),queryresult.at(0).at(0).toStdString()));
            }else{
                responseParams.insert(std::make_pair(std::string("info"),std::string("sql insert fail")));
                responseParams.insert(std::make_pair(std::string("result"),std::string("fail")));
            }
        }else{
            responseParams.insert(std::make_pair(std::string("info"),std::string("sql insert fail")));
            responseParams.insert(std::make_pair(std::string("result"),std::string("fail")));
        }
    }

}
//删除
void UserMsgProcessor:: AgvManage_Delete(const QyhMsgDateItem &item, std::map<std::string, std::string> &requestDatas, std::vector<std::map<std::string, std::string> > &datalists,std::map<std::string,std::string> &responseParams,std::vector<std::map<std::string,std::string> > &responseDatalists,LoginUserInfo &loginUserInfo){
    //要求agvid
    if(checkParamExistAndNotNull(requestDatas,responseParams,"agvid")){
        int iAgvId;
        std::stringstream ss_id;
        ss_id<<requestDatas.at("agvid");
        ss_id>>iAgvId;
        //查找是否存在
        if(g_m_agvs.contains(iAgvId)){
            //从数据库中清除
            QString deleteSql = "delete from agv_agv where id=?";
            QStringList tempParams;
            tempParams<<QString("%1").arg(iAgvId);
            if(g_sql->exec(deleteSql,tempParams)){
                //从列表中清楚
                Agv *agv = g_m_agvs[iAgvId];
                delete agv;
                agv = NULL;
                g_m_agvs.remove(iAgvId);
                responseParams.insert(std::make_pair(std::string("info"),std::string("")));
                responseParams.insert(std::make_pair(std::string("result"),std::string("success")));
            }else{
                responseParams.insert(std::make_pair(std::string("info"),std::string("delete sql exec fail")));
                responseParams.insert(std::make_pair(std::string("result"),std::string("fail")));
            }
        }else{
            responseParams.insert(std::make_pair(std::string("info"),std::string("not exist of this agvid.")));
            responseParams.insert(std::make_pair(std::string("result"),std::string("fail")));
        }

        QString insertSql = "delete from agv_agv where id=?";
        QStringList tempParams;
        tempParams<<QString::fromStdString(requestDatas.at("name"))<<QString::fromStdString(requestDatas.at("ip"));
        if(g_sql->exec(insertSql,tempParams)){
            responseParams.insert(std::make_pair(std::string("info"),std::string("")));
            responseParams.insert(std::make_pair(std::string("result"),std::string("success")));
        }else{
            responseParams.insert(std::make_pair(std::string("info"),std::string("sql insert fail")));
            responseParams.insert(std::make_pair(std::string("result"),std::string("fail")));
        }
    }
}
//修改
void UserMsgProcessor:: AgvManage_Modify(const QyhMsgDateItem &item, std::map<std::string, std::string> &requestDatas, std::vector<std::map<std::string, std::string> > &datalists,std::map<std::string,std::string> &responseParams,std::vector<std::map<std::string,std::string> > &responseDatalists,LoginUserInfo &loginUserInfo){
    if(checkParamExistAndNotNull(requestDatas,responseParams,"agvid","name","ip")){
        int iAgvId;
        std::stringstream ss_id;
        ss_id<<requestDatas.at("agvid");
        ss_id>>iAgvId;
        Agv *agv = NULL;
        for(QMap<int,Agv *>::iterator itr = g_m_agvs.begin();itr!=g_m_agvs.end();++itr){
            if(itr.value()->id() == iAgvId){
                agv = itr.value();
                break;
            }
        }
        if(agv==NULL){
            //不存在这辆车
            responseParams.insert(std::make_pair(std::string("info"),std::string("not exist of this agvid.")));
            responseParams.insert(std::make_pair(std::string("result"),std::string("fail")));
        }else{
            QString updateSql = "update agv_agv set name=?,set ip=? where id=?";
            QStringList params;
            params<<(QString::fromStdString(requestDatas["name"]))<<(QString::fromStdString(requestDatas["ip"]))<<(QString::fromStdString(requestDatas["agvid"]));
            if(g_sql->exec(updateSql,params)){
                agv->setName(QString::fromStdString(requestDatas["name"]));
                agv->setIp(QString::fromStdString(requestDatas["ip"]));
                responseParams.insert(std::make_pair(std::string("info"),std::string("")));
                responseParams.insert(std::make_pair(std::string("result"),std::string("success")));
            }else{
                responseParams.insert(std::make_pair(std::string("info"),std::string("sql update exec fail")));
                responseParams.insert(std::make_pair(std::string("result"),std::string("fail")));
            }
        }
    }
}


////////////////////////////////任务部分
//创建任务(创建到X点的任务)
void UserMsgProcessor::Task_CreateToX(const QyhMsgDateItem &item, std::map<std::string, std::string> &requestDatas, std::vector<std::map<std::string, std::string> > &datalists,std::map<std::string,std::string> &responseParams,std::vector<std::map<std::string,std::string> > &responseDatalists,LoginUserInfo &loginUserInfo){
    if(checkParamExistAndNotNull(requestDatas,responseParams,"x")){
        //可选项<xWaitType><xWaitTime><yWaitType><yWaitTime>
        int iX;
        std::stringstream ss_x;
        ss_x << requestDatas["x"];
        ss_x >> iX;
        //确保站点存在
        int waitTypeX = AGV_TASK_WAIT_TYPE_NOWAIT;
        int watiTimeX = 30;

        //判断是否设置了等待时间和等待时长
        if(requestDatas.find("xWaitType")!=requestDatas.end() && requestDatas.at("xWaitType").length()>0){
            std::stringstream ss_xWaitType;
            ss_xWaitType << requestDatas.at("xWaitType");
            ss_xWaitType>>waitTypeX;
        }
        if(requestDatas.find("xWaitTime")!=requestDatas.end() && requestDatas.at("xWaitTime").length()>0){
            std::stringstream ss_xWaitTime;
            ss_xWaitTime << requestDatas.at("xWaitTime");
            ss_xWaitTime>>watiTimeX;
        }

        if(!g_m_stations.contains(iX)){
            responseParams.insert(std::make_pair(std::string("result"),std::string("fail")));
            responseParams.insert(std::make_pair(std::string("info"),std::string("not found station")));
        }else{
            int id = g_taskCenter.makeAimTask(iX,waitTypeX,watiTimeX);
            std::stringstream ss_id;
            std::string str_id;
            ss_id<<id;
            ss_id>>str_id;
            responseParams.insert(std::make_pair(std::string("info"),std::string("")));
            responseParams.insert(std::make_pair(std::string("result"),std::string("success")));
            responseParams.insert(std::make_pair(std::string("id"),str_id));
        }
    }
}

//创建任务(创建指定车辆到X点的任务)
void UserMsgProcessor::Task_CreateAgvToX(const QyhMsgDateItem &item, std::map<std::string, std::string> &requestDatas, std::vector<std::map<std::string, std::string> > &datalists,std::map<std::string,std::string> &responseParams,std::vector<std::map<std::string,std::string> > &responseDatalists,LoginUserInfo &loginUserInfo){
    if(checkParamExistAndNotNull(requestDatas,responseParams,"x","agvid")){
        int iX;
        std::stringstream ss_x;
        ss_x << requestDatas["x"];
        ss_x >> iX;

        int iAgvid;
        std::stringstream ss_agvid;
        ss_agvid << requestDatas["agvid"];
        ss_agvid >> iAgvid;
        //确保站点存在
        int waitTypeX = AGV_TASK_WAIT_TYPE_NOWAIT;
        int watiTimeX = 30;

        //判断是否设置了等待时间和等待时长
        if(requestDatas.find("xWaitType")!=requestDatas.end() && requestDatas.at("xWaitType").length()>0){
            std::stringstream ss_xWaitType;
            ss_xWaitType << requestDatas.at("xWaitType");
            ss_xWaitType>>waitTypeX;
        }
        if(requestDatas.find("xWaitTime")!=requestDatas.end() && requestDatas.at("xWaitTime").length()>0){
            std::stringstream ss_xWaitTime;
            ss_xWaitTime << requestDatas.at("xWaitTime");
            ss_xWaitTime>>watiTimeX;
        }

        if(!g_m_stations.contains(iX)){
            responseParams.insert(std::make_pair(std::string("result"),std::string("fail")));
            responseParams.insert(std::make_pair(std::string("info"),std::string("not found station")));
        }else if(!g_m_agvs.contains(iAgvid)){
            responseParams.insert(std::make_pair(std::string("result"),std::string("fail")));
            responseParams.insert(std::make_pair(std::string("info"),std::string("not found agv")));
        }else{
            int id = g_taskCenter.makeAgvAimTask(iAgvid,iX,waitTypeX,watiTimeX);
            std::stringstream ss_id;
            std::string str_id;
            ss_id<<id;
            ss_id>>str_id;
            responseParams.insert(std::make_pair(std::string("info"),std::string("")));
            responseParams.insert(std::make_pair(std::string("result"),std::string("success")));
            responseParams.insert(std::make_pair(std::string("id"),str_id));
        }
    }
}
//创建任务(创建经过Y点到X点的任务)
void UserMsgProcessor::Task_CreateYToX(const QyhMsgDateItem &item, std::map<std::string, std::string> &requestDatas, std::vector<std::map<std::string, std::string> > &datalists,std::map<std::string,std::string> &responseParams,std::vector<std::map<std::string,std::string> > &responseDatalists,LoginUserInfo &loginUserInfo){
    if(checkParamExistAndNotNull(requestDatas,responseParams,"x","y")){
        int iX;
        std::stringstream ss_x;
        ss_x << requestDatas["x"];
        ss_x >> iX;

        int iY;
        std::stringstream ss_y;
        ss_y << requestDatas["y"];
        ss_y >> iY;

        int waitTypeX = AGV_TASK_WAIT_TYPE_NOWAIT;
        int watiTimeX = 30;
        int waitTypeY = AGV_TASK_WAIT_TYPE_NOWAIT;
        int watiTimeY = 30;

        //判断是否设置了等待时间和等待时长
        if(requestDatas.find("xWaitType")!=requestDatas.end() && requestDatas.at("xWaitType").length()>0){
            std::stringstream ss_xWaitType;
            ss_xWaitType << requestDatas.at("xWaitType");
            ss_xWaitType>>waitTypeX;
        }
        if(requestDatas.find("xWaitTime")!=requestDatas.end() && requestDatas.at("xWaitTime").length()>0){
            std::stringstream ss_xWaitTime;
            ss_xWaitTime << requestDatas.at("xWaitTime");
            ss_xWaitTime>>watiTimeX;
        }
        if(requestDatas.find("yWaitType")!=requestDatas.end() && requestDatas.at("yWaitType").length()>0){
            std::stringstream ss_yWaitType;
            ss_yWaitType << requestDatas.at("yWaitType");
            ss_yWaitType>>waitTypeY;
        }
        if(requestDatas.find("yWaitTime")!=requestDatas.end() && requestDatas.at("yWaitTime").length()>0){
            std::stringstream ss_yWaitTime;
            ss_yWaitTime << requestDatas.at("yWaitTime");
            ss_yWaitTime>>watiTimeY;
        }
        //确保站点存在
        if(!g_m_stations.contains(iX)){
            responseParams.insert(std::make_pair(std::string("result"),std::string("fail")));
            responseParams.insert(std::make_pair(std::string("info"),std::string("not found station x")));
        }else if(!g_m_stations.contains(iY)){
            responseParams.insert(std::make_pair(std::string("result"),std::string("fail")));
            responseParams.insert(std::make_pair(std::string("info"),std::string("not found station y")));
        }else{
            int id = g_taskCenter.makePickupTask(iY,iX,waitTypeX,watiTimeX,waitTypeY,watiTimeY);
            std::stringstream ss_id;
            std::string str_id;
            ss_id<<id;
            ss_id>>str_id;
            responseParams.insert(std::make_pair(std::string("info"),std::string("")));
            responseParams.insert(std::make_pair(std::string("result"),std::string("success")));
            responseParams.insert(std::make_pair(std::string("id"),str_id));
        }
    }
}
//创建任务(创建指定车辆经过Y点到X点的任务)
void UserMsgProcessor::Task_CreateAgvYToX(const QyhMsgDateItem &item, std::map<std::string, std::string> &requestDatas, std::vector<std::map<std::string, std::string> > &datalists,std::map<std::string,std::string> &responseParams,std::vector<std::map<std::string,std::string> > &responseDatalists,LoginUserInfo &loginUserInfo){
    if(checkParamExistAndNotNull(requestDatas,responseParams,"x","y","agvid")){
        int iX;
        std::stringstream ss_x;
        ss_x << requestDatas["x"];
        ss_x >> iX;

        int iY;
        std::stringstream ss_y;
        ss_y << requestDatas["y"];
        ss_y >> iY;

        int iAgvid;
        std::stringstream ss_agvid;
        ss_agvid << requestDatas["agvid"];
        ss_agvid >> iAgvid;

        int waitTypeX = AGV_TASK_WAIT_TYPE_NOWAIT;
        int watiTimeX = 30;
        int waitTypeY = AGV_TASK_WAIT_TYPE_NOWAIT;
        int watiTimeY = 30;

        //判断是否设置了等待时间和等待时长
        if(requestDatas.find("xWaitType")!=requestDatas.end() && requestDatas.at("xWaitType").length()>0){
            std::stringstream ss_xWaitType;
            ss_xWaitType << requestDatas.at("xWaitType");
            ss_xWaitType>>waitTypeX;
        }
        if(requestDatas.find("xWaitTime")!=requestDatas.end() && requestDatas.at("xWaitTime").length()>0){
            std::stringstream ss_xWaitTime;
            ss_xWaitTime << requestDatas.at("xWaitTime");
            ss_xWaitTime>>watiTimeX;
        }
        if(requestDatas.find("yWaitType")!=requestDatas.end() && requestDatas.at("yWaitType").length()>0){
            std::stringstream ss_yWaitType;
            ss_yWaitType << requestDatas.at("yWaitType");
            ss_yWaitType>>waitTypeY;
        }
        if(requestDatas.find("yWaitTime")!=requestDatas.end() && requestDatas.at("yWaitTime").length()>0){
            std::stringstream ss_yWaitTime;
            ss_yWaitTime << requestDatas.at("yWaitTime");
            ss_yWaitTime>>watiTimeY;
        }
        //确保站点存在
        if(!g_m_stations.contains(iX)){
            responseParams.insert(std::make_pair(std::string("result"),std::string("fail")));
            responseParams.insert(std::make_pair(std::string("info"),std::string("not found station x")));
        }else if(!g_m_stations.contains(iY)){
            responseParams.insert(std::make_pair(std::string("result"),std::string("fail")));
            responseParams.insert(std::make_pair(std::string("info"),std::string("not found station y")));
        }else if(!g_m_agvs.contains(iAgvid)){
            responseParams.insert(std::make_pair(std::string("result"),std::string("fail")));
            responseParams.insert(std::make_pair(std::string("info"),std::string("not found agv")));
        }else{
            int id = g_taskCenter.makePickupTask(iY,iX,waitTypeX,watiTimeX,waitTypeY,watiTimeY);
            std::stringstream ss_id;
            std::string str_id;
            ss_id<<id;
            ss_id>>str_id;
            responseParams.insert(std::make_pair(std::string("info"),std::string("")));
            responseParams.insert(std::make_pair(std::string("result"),std::string("success")));
            responseParams.insert(std::make_pair(std::string("id"),str_id));
        }
    }
}
//查询任务状态
void UserMsgProcessor::Task_QueryStatus(const QyhMsgDateItem &item, std::map<std::string, std::string> &requestDatas, std::vector<std::map<std::string, std::string> > &datalists,std::map<std::string,std::string> &responseParams,std::vector<std::map<std::string,std::string> > &responseDatalists,LoginUserInfo &loginUserInfo){
    if(checkParamExistAndNotNull(requestDatas,responseParams,"taskid")){
        std::stringstream ss_taskid;
        int taskid;
        ss_taskid<<requestDatas["taskid"];
        ss_taskid>>taskid;
        int status = g_taskCenter.queryTaskStatus(taskid);

        std::string strStatus;
        std::stringstream ss_status;
        ss_status<<status;
        ss_status>>strStatus;

        //        switch (status) {
        //        case AGV_TASK_STATUS_UNEXIST:
        //            strStatus = "AGV_TASK_STATUS_UNEXIST";
        //            break;
        //        case AGV_TASK_STATUS_UNEXCUTE:
        //            strStatus = "AGV_TASK_STATUS_UNEXCUTE";
        //            break;
        //        case AGV_TASK_STATUS_EXCUTING:
        //            strStatus = "AGV_TASK_STATUS_EXCUTING";
        //            break;
        //        case AGV_TASK_STATUS_DONE:
        //            strStatus = "AGV_TASK_STATUS_DONE";
        //            break;
        //        case AGV_TASK_STATUS_FAIL:
        //            strStatus = "AGV_TASK_STATUS_FAIL";
        //            break;
        //        case AGV_TASK_STATSU_CANCEL:
        //            strStatus = "AGV_TASK_STATSU_CANCEL";
        //            break;
        //        default:
        //            strStatus = "AGV_TASK_STATUS_UNEXIST";
        //            break;
        //        }
        responseParams.insert(std::make_pair(std::string("status"),strStatus));
        responseParams.insert(std::make_pair(std::string("info"),std::string("")));
        responseParams.insert(std::make_pair(std::string("result"),std::string("success")));
    }
}
//取消任务
void UserMsgProcessor::Task_Cancel(const QyhMsgDateItem &item, std::map<std::string, std::string> &requestDatas, std::vector<std::map<std::string, std::string> > &datalists,std::map<std::string,std::string> &responseParams,std::vector<std::map<std::string,std::string> > &responseDatalists,LoginUserInfo &loginUserInfo){
    if(checkParamExistAndNotNull(requestDatas,responseParams,"taskid")){
        std::stringstream ss_taskid;
        int taskid;
        ss_taskid<<requestDatas["taskid"];
        ss_taskid>>taskid;

        if(g_taskCenter.cancelTask(taskid)){
            responseParams.insert(std::make_pair(std::string("info"),std::string("")));
            responseParams.insert(std::make_pair(std::string("result"),std::string("success")));
        }else{
            //
            responseParams.insert(std::make_pair(std::string("info"),std::string("not find taskid in unassigned or doging tasks list")));
            responseParams.insert(std::make_pair(std::string("result"),std::string("fail")));
        }
    }
}
//未分配任务列表
void UserMsgProcessor::Task_ListUnassigned(const QyhMsgDateItem &item, std::map<std::string, std::string> &requestDatas, std::vector<std::map<std::string, std::string> > &datalists,std::map<std::string,std::string> &responseParams,std::vector<std::map<std::string,std::string> > &responseDatalists,LoginUserInfo &loginUserInfo){
    responseParams.insert(std::make_pair(std::string("info"),std::string("")));
    responseParams.insert(std::make_pair(std::string("result"),std::string("success")));
    //添加列表
    QList<AgvTask *> tasks = g_taskCenter.getUnassignedTasks();
    for(QList<AgvTask *>::iterator itr = tasks.begin();itr!=tasks.end();++itr){
        AgvTask * task = *itr;
        std::map<std::string,std::string> onetask;

        std::stringstream ss_id;
        std::string str_id;
        ss_id<<task->id();
        ss_id>>str_id;
        onetask.insert(std::make_pair(std::string("id"),str_id));

        std::stringstream ss_produceTime;
        std::string str_produceTime;
        ss_produceTime<<task->produceTime().toString("yyyy-MM-dd hh:mm:ss").toStdString();
        ss_produceTime>>str_produceTime;
        onetask.insert(std::make_pair(std::string("produceTime"),str_produceTime));

        //        std::stringstream ss_doTime;
        //        std::string str_doTime;
        //        ss_doTime<<task->doTime().toString("yyyy-MM-dd hh:mm:ss").toStdString();
        //        ss_doTime>>str_doTime;
        //        onetask.insert(std::make_pair(std::string("doTime"),str_doTime));

        //        std::stringstream ss_doneTime;
        //        std::string str_doneTime;
        //        ss_doneTime<<task->doneTime().toString("yyyy-MM-dd hh:mm:ss").toStdString();
        //        ss_doneTime>>str_doneTime;
        //        onetask.insert(std::make_pair(std::string("doneTime"),str_doneTime));

        std::stringstream ss_excuteCar;
        std::string str_excuteCar;
        ss_excuteCar<<task->excuteCar();
        ss_excuteCar>>str_excuteCar;
        onetask.insert(std::make_pair(std::string("excuteCar"),str_excuteCar));

        std::stringstream ss_status;
        std::string str_status;
        ss_status<<task->status();
        ss_status>>str_status;
        onetask.insert(std::make_pair(std::string("status"),str_status));

        responseDatalists.push_back(onetask);
    }

}
//正在执行任务列表
void UserMsgProcessor::Task_ListDoing(const QyhMsgDateItem &item, std::map<std::string, std::string> &requestDatas, std::vector<std::map<std::string, std::string> > &datalists,std::map<std::string,std::string> &responseParams,std::vector<std::map<std::string,std::string> > &responseDatalists,LoginUserInfo &loginUserInfo){
    responseParams.insert(std::make_pair(std::string("info"),std::string("")));
    responseParams.insert(std::make_pair(std::string("result"),std::string("success")));
    //添加列表
    QList<AgvTask *> tasks = g_taskCenter.getDoingTasks();
    for(QList<AgvTask *>::iterator itr = tasks.begin();itr!=tasks.end();++itr){
        AgvTask * task = *itr;
        std::map<std::string,std::string> onetask;

        std::stringstream ss_id;
        std::string str_id;
        ss_id<<task->id();
        ss_id>>str_id;
        onetask.insert(std::make_pair(std::string("id"),str_id));

        std::stringstream ss_produceTime;
        std::string str_produceTime;
        ss_produceTime<<task->produceTime().toString("yyyy-MM-dd hh:mm:ss").toStdString();
        ss_produceTime>>str_produceTime;
        onetask.insert(std::make_pair(std::string("produceTime"),str_produceTime));

        std::stringstream ss_doTime;
        std::string str_doTime;
        ss_doTime<<task->doTime().toString("yyyy-MM-dd hh:mm:ss").toStdString();
        ss_doTime>>str_doTime;
        onetask.insert(std::make_pair(std::string("doTime"),str_doTime));

        //        std::stringstream ss_doneTime;
        //        std::string str_doneTime;
        //        ss_doneTime<<task->doneTime().toString("yyyy-MM-dd hh:mm:ss").toStdString();
        //        ss_doneTime>>str_doneTime;
        //        onetask.insert(std::make_pair(std::string("doneTime"),str_doneTime));

        std::stringstream ss_excuteCar;
        std::string str_excuteCar;
        ss_excuteCar<<task->excuteCar();
        ss_excuteCar>>str_excuteCar;
        onetask.insert(std::make_pair(std::string("excuteCar"),str_excuteCar));

        std::stringstream ss_status;
        std::string str_status;
        ss_status<<task->status();
        ss_status>>str_status;
        onetask.insert(std::make_pair(std::string("status"),str_status));

        responseDatalists.push_back(onetask);
    }
}
//已经完成任务列表(today)
void UserMsgProcessor::Task_ListDoneToday(const QyhMsgDateItem &item, std::map<std::string, std::string> &requestDatas, std::vector<std::map<std::string, std::string> > &datalists,std::map<std::string,std::string> &responseParams,std::vector<std::map<std::string,std::string> > &responseDatalists,LoginUserInfo &loginUserInfo){
    responseParams.insert(std::make_pair(std::string("info"),std::string("")));
    responseParams.insert(std::make_pair(std::string("result"),std::string("success")));

    QString querySql = "select id,produceTime,doneTime,doTime,excuteCar,status from agv_task where status = ? and done time between ? and ?;";
    QDate today = QDate::currentDate();
    QDate tomorrow = today.addDays(1);
    QStringList params;
    params<<QString("%1").arg(AGV_TASK_STATUS_DONE);

    QDateTime to = QDateTime::currentDateTime();
    QDateTime from(QDate::currentDate());

    params<<from.toString(DATE_TIME_FORMAT);
    params<<to.toString(DATE_TIME_FORMAT);

    QList<QStringList> result = g_sql->query(querySql,params);

    for(int i=0;i<result.length();++i){
        QStringList qsl = result.at(i);
        if(qsl.length() == 6)
        {
            std::map<std::string,std::string> task;
            task.insert(std::make_pair(std::string("id"),qsl.at(0).toStdString()));
            task.insert(std::make_pair(std::string("produceTime"),qsl.at(1).toStdString()));
            task.insert(std::make_pair(std::string("doneTime"),qsl.at(2).toStdString()));
            task.insert(std::make_pair(std::string("doTime"),qsl.at(3).toStdString()));
            task.insert(std::make_pair(std::string("excuteCar"),qsl.at(4).toStdString()));
            task.insert(std::make_pair(std::string("status"),qsl.at(5).toStdString()));
            responseDatalists.push_back(task);
        }
    }
}

//已经完成任务列表(all)
void UserMsgProcessor::Task_ListDoneAll(const QyhMsgDateItem &item, std::map<std::string, std::string> &requestDatas, std::vector<std::map<std::string, std::string> > &datalists,std::map<std::string,std::string> &responseParams,std::vector<std::map<std::string,std::string> > &responseDatalists,LoginUserInfo &loginUserInfo)
{
    responseParams.insert(std::make_pair(std::string("info"),std::string("")));
    responseParams.insert(std::make_pair(std::string("result"),std::string("success")));

    QString querySql = "select id,produceTime,doneTime,doTime,excuteCar,status from agv_task where status = ? ";
    QDate today = QDate::currentDate();
    QDate tomorrow = today.addDays(1);
    QStringList params;
    params<<QString("%1").arg(AGV_TASK_STATUS_DONE);
    //    params<<QDateTime::currentDateTime().setDate(today).toString(DATE_TIME_FORMAT);
    //    params<<QDateTime::currentDateTime().setDate(tomorrow).toString(DATE_TIME_FORMAT);

    QList<QStringList> result = g_sql->query(querySql,params);

    for(int i=0;i<result.length();++i){
        QStringList qsl = result.at(i);
        if(qsl.length() == 6)
        {
            std::map<std::string,std::string> task;
            task.insert(std::make_pair(std::string("id"),qsl.at(0).toStdString()));
            task.insert(std::make_pair(std::string("produceTime"),qsl.at(1).toStdString()));
            task.insert(std::make_pair(std::string("doneTime"),qsl.at(2).toStdString()));
            task.insert(std::make_pair(std::string("doTime"),qsl.at(3).toStdString()));
            task.insert(std::make_pair(std::string("excuteCar"),qsl.at(4).toStdString()));
            task.insert(std::make_pair(std::string("status"),qsl.at(5).toStdString()));
            responseDatalists.push_back(task);
        }
    }
}

//已经完成任务列表(from to 时间)
void UserMsgProcessor::Task_ListDoneDuring(const QyhMsgDateItem &item, std::map<std::string, std::string> &requestDatas, std::vector<std::map<std::string, std::string> > &datalists,std::map<std::string,std::string> &responseParams,std::vector<std::map<std::string,std::string> > &responseDatalists,LoginUserInfo &loginUserInfo)
{
    //要求带有from和to
    if(checkParamExistAndNotNull(requestDatas,responseParams,"from","to")){
        responseParams.insert(std::make_pair(std::string("info"),std::string("")));
        responseParams.insert(std::make_pair(std::string("result"),std::string("success")));

        QString querySql = "select id,produceTime,doneTime,doTime,excuteCar,status from agv_task where status = ? and done time between ? and ?;";
        QStringList params;
        params<<QString("%1").arg(AGV_TASK_STATUS_DONE);
        QDateTime from = QDateTime::fromString(QString::fromStdString(requestDatas["from"]));
        QDateTime to = QDateTime::fromString(QString::fromStdString(requestDatas["to"]));
        params<<from.toString(DATE_TIME_FORMAT);
        params<<to.toString(DATE_TIME_FORMAT);

        QList<QStringList> result = g_sql->query(querySql,params);

        for(int i=0;i<result.length();++i){
            QStringList qsl = result.at(i);
            if(qsl.length() == 6)
            {
                std::map<std::string,std::string> task;
                task.insert(std::make_pair(std::string("id"),qsl.at(0).toStdString()));
                task.insert(std::make_pair(std::string("produceTime"),qsl.at(1).toStdString()));
                task.insert(std::make_pair(std::string("doneTime"),qsl.at(2).toStdString()));
                task.insert(std::make_pair(std::string("doTime"),qsl.at(3).toStdString()));
                task.insert(std::make_pair(std::string("excuteCar"),qsl.at(4).toStdString()));
                task.insert(std::make_pair(std::string("status"),qsl.at(5).toStdString()));
                responseDatalists.push_back(task);
            }
        }
    }
}

void UserMsgProcessor::Task_Detail(const QyhMsgDateItem &item, std::map<std::string, std::string> &requestDatas, std::vector<std::map<std::string, std::string> > &datalists,std::map<std::string,std::string> &responseParams,std::vector<std::map<std::string,std::string> > &responseDatalists,LoginUserInfo &loginUserInfo)
{
    //要求带有taskid
    bool needDelete = false;
    AgvTask *task = NULL;
    if(checkParamExistAndNotNull(requestDatas,responseParams,"taskid")){
        int taskId;
        std::stringstream ss_taskId;
        ss_taskId<<requestDatas["taskid"];
        ss_taskId>>taskId;

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
            responseParams.insert(std::make_pair(std::string("info"),std::string("not found task with taskid")));
            responseParams.insert(std::make_pair(std::string("result"),std::string("fail")));
        }else{
            responseParams.insert(std::make_pair(std::string("info"),std::string("")));
            responseParams.insert(std::make_pair(std::string("result"),std::string("success")));

            {
                std::stringstream ss_id;
                std::string str_id;
                ss_id<<task->id();
                ss_id>>str_id;
                responseParams.insert(std::make_pair(std::string("id"),str_id));

                std::stringstream ss_produceTime;
                std::string str_produceTime;
                ss_produceTime<<task->produceTime().toString(DATE_TIME_FORMAT).toStdString();
                ss_produceTime>>str_produceTime;
                responseParams.insert(std::make_pair(std::string("produceTime"),str_produceTime));

                std::stringstream ss_doneTime;
                std::string str_doneTime;
                ss_doneTime<<task->doneTime().toString(DATE_TIME_FORMAT).toStdString();
                ss_doneTime>>str_doneTime;
                responseParams.insert(std::make_pair(std::string("doneTime"),str_doneTime));

                std::stringstream ss_doTime;
                std::string str_doTime;
                ss_doTime<<task->doTime().toString(DATE_TIME_FORMAT).toStdString();
                ss_doTime>>str_doTime;
                responseParams.insert(std::make_pair(std::string("doTime"),str_doTime));

                std::stringstream ss_excuteCar;
                std::string str_excuteCar;
                ss_excuteCar<<task->excuteCar();
                ss_excuteCar>>str_excuteCar;
                responseParams.insert(std::make_pair(std::string("id"),str_excuteCar));

                std::stringstream ss_status;
                std::string str_status;
                ss_status<<task->status();
                ss_status>>str_status;
                responseParams.insert(std::make_pair(std::string("id"),str_status));
            }
            //装入节点
            for(int i=0;i<task->taskNodes.length();++i){
                TaskNode *tn = task->taskNodes.at(i);

                std::map<std::string,std::string> node;

                std::stringstream ss_status;
                std::string str_status;
                ss_status<<tn->status;
                ss_status>>str_status;
                node.insert(std::make_pair(std::string("status"),str_status));

//                std::stringstream ss_taskId;
//                std::string str_taskId;
//                ss_taskId<<task->id();
//                ss_taskId>>str_taskId;
//                node.insert(std::make_pair(std::string("taskId"),str_taskId));

                std::stringstream ss_queueNumber;
                std::string str_queueNumber;
                ss_queueNumber<<tn->queueNumber;
                ss_queueNumber>>str_queueNumber;
                node.insert(std::make_pair(std::string("queueNumber"),str_queueNumber));

                std::stringstream ss_aimStation;
                std::string str_aimStation;
                ss_aimStation<<tn->aimStation;
                ss_aimStation>>str_aimStation;
                node.insert(std::make_pair(std::string("aimStation"),str_aimStation));

                std::stringstream ss_waitType;
                std::string str_waitType;
                ss_waitType<<tn->waitType;
                ss_waitType>>str_waitType;
                node.insert(std::make_pair(std::string("waitType"),str_waitType));

                std::stringstream ss_waitTime;
                std::string str_waitTime;
                ss_waitTime<<tn->waitTime;
                ss_waitTime>>str_waitTime;
                node.insert(std::make_pair(std::string("waitTime"),str_waitTime));

                std::stringstream ss_arriveTime;
                std::string str_arriveTime;
                ss_arriveTime<<tn->arriveTime.toString(DATE_TIME_FORMAT).toStdString();
                ss_arriveTime>>str_arriveTime;
                node.insert(std::make_pair(std::string("arriveTime"),str_arriveTime));

                std::stringstream ss_leaveTime;
                std::string str_leaveTime;
                ss_leaveTime<<tn->leaveTime.toString(DATE_TIME_FORMAT).toStdString();
                ss_leaveTime>>str_leaveTime;
                node.insert(std::make_pair(std::string("leaveTime"),str_leaveTime));

                responseDatalists.push_back(node);
            }

            if(needDelete)
                delete task;
        }

    }
}



