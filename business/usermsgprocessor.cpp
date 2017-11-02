#include "usermsgprocessor.h"
#include "util/global.h"
#include "pugixml.hpp"
#include <sstream>
#include <iostream>
#include <QUuid>


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

//对接收到的消息，进行处理！
void UserMsgProcessor::responseOneMsg(const QyhMsgDateItem &item,std::map<string,string> requestDatas,std::vector<std::map<std::string,std::string> > datalists)
{

    std::map<std::string,std::string> responseParams;
    std::vector<std::map<std::string,std::string> > responseDatalists;
    //先把头列好
    responseParams.insert(std::make_pair(std::string("type"),requestDatas["type"]));
    responseParams.insert(std::make_pair(std::string("todo"),requestDatas["todo"]));
    responseParams.insert(std::make_pair(std::string("queuenumber"),requestDatas["queuenumber"]));

    ////如果未登录，则进行登录
    //////////////////请求登录(做特殊处理，因为这里不包含access_token!)
    if(requestDatas["type"]=="user" && requestDatas["todo"]=="login")
    {
        //要求含有username和password
        if(requestDatas.find("username")==requestDatas.end()){
            //用户名没有
            responseParams.insert(std::make_pair(std::string("info"),std::string("param lack:username.")));
            responseParams.insert(std::make_pair(std::string("result"),std::string("fail")));
        }else if(requestDatas.find("password")==requestDatas.end()){
            //密码没有
            responseParams.insert(std::make_pair(std::string("info"),std::string("param lack:password.")));
            responseParams.insert(std::make_pair(std::string("result"),std::string("fail")));

        }else if(requestDatas.at("username").length()==0){
            responseParams.insert(std::make_pair(std::string("info"),std::string("null of:username.")));
            responseParams.insert(std::make_pair(std::string("result"),std::string("fail")));
        }else if(requestDatas.at("password").length()==0){
            responseParams.insert(std::make_pair(std::string("info"),std::string("null of:password.")));
            responseParams.insert(std::make_pair(std::string("result"),std::string("fail")));
        }
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

                //TODO:设置登录状态和登录时间


            }else{
                //登录失败
                responseParams.insert(std::make_pair(std::string("info"),std::string("not correct:password")));
                responseParams.insert(std::make_pair(std::string("result"),std::string("fail")));
            }
        }
        //封装
        std::string xml = getResponseXml(responseParams,responseDatalists);
        //发送
        g_netWork->sendToOne(item.sock,xml.c_str(),xml.length());
        return ;
    }

    /////如果已经登录，安全验证 随机码
    /////对之后的所有的消息内容判断之前，要求判断access——token。
    if(requestDatas.find("access_token")==requestDatas.end()){
        //没有包含access_token
        std::map<std::string,std::string> datas;
        datas.insert(std::make_pair(std::string("result"),std::string("fail")));
        datas.insert(std::make_pair(std::string("info"),std::string("not find access_token")));
        datalists.push_back(datas);
        //封装
        std::string xml = getResponseXml(responseParams,responseDatalists);
        //发送
        g_netWork->sendToOne(item.sock,xml.c_str(),xml.length());
        return;
    }

    ////对access_token判断是否正确
    bool access_token_correct = false;
    for(std::list<LoginUserInfo>::iterator itr = loginUserIdSock.begin();itr!=loginUserIdSock.end();++itr){
        LoginUserInfo info = *itr;
        if(info.sock == item.sock && info.access_tocken == requestDatas["access_token"]){
            access_token_correct = true;
        }
    }
    if(!access_token_correct){
        //没有包含access_token
        std::map<std::string,std::string> datas;
        datas.insert(std::make_pair(std::string("result"),std::string("fail")));
        datas.insert(std::make_pair(std::string("info"),std::string("not correct:access_token,please relogin!")));
        datalists.push_back(datas);
        //封装
        std::string xml = getResponseXml(responseParams,responseDatalists);
        //发送
        g_netWork->sendToOne(item.sock,xml.c_str(),xml.length());
        return;
    }

    //////////////////退出登录
    if(requestDatas["type"]=="user" && requestDatas["todo"]=="logout"){
        //要求含有username和password
        if(requestDatas.find("username")==requestDatas.end()){
            //用户名没有
            responseParams.insert(std::make_pair(std::string("info"),std::string("param lack:username.")));
            responseParams.insert(std::make_pair(std::string("result"),std::string("fail")));
        }else if(requestDatas.at("username").length()==0){
            responseParams.insert(std::make_pair(std::string("info"),std::string("null of:username.")));
            responseParams.insert(std::make_pair(std::string("result"),std::string("fail")));
        }

        //TODO:loginUserIdSock.insert(std::make_pair(id,item.sock));
        QString querySqlA = "select status from agv_user where username=?";
        QStringList params;
        params<<QString::fromStdString(requestDatas["username"]);
        QList<QStringList> queryresult = g_sql->query(querySqlA,params);
        if(queryresult.length()==0){
            responseParams.insert(std::make_pair(std::string("info"),std::string("null of:username.")));
            responseParams.insert(std::make_pair(std::string("result"),std::string("fail")));
        }else{
            ////TODO:设置登录状态
            responseParams.insert(std::make_pair(std::string("info"),std::string("")));
            responseParams.insert(std::make_pair(std::string("result"),std::string("success")));
        }
    }

    ///////////////////修改密码
    else if(requestDatas["type"]=="user" && requestDatas["todo"]=="changepassword"){
        //要求含有username和oldpassword newpassword
        if(requestDatas.find("username")==requestDatas.end()){
            responseParams.insert(std::make_pair(std::string("info"),std::string("param lack:username.")));
            responseParams.insert(std::make_pair(std::string("result"),std::string("fail")));
        }else if(requestDatas.find("oldpassword")==requestDatas.end()){
            responseParams.insert(std::make_pair(std::string("info"),std::string("param lack:oldpassword.")));
            responseParams.insert(std::make_pair(std::string("result"),std::string("fail")));
        }else if(requestDatas.find("newpassword")==requestDatas.end()){
            responseParams.insert(std::make_pair(std::string("info"),std::string("param lack:newpassword.")));
            responseParams.insert(std::make_pair(std::string("result"),std::string("fail")));
        }else if(requestDatas.at("username").length()==0){
            responseParams.insert(std::make_pair(std::string("info"),std::string("null of:username.")));
            responseParams.insert(std::make_pair(std::string("result"),std::string("fail")));
        }else if(requestDatas.at("oldpassword").length()==0){
            responseParams.insert(std::make_pair(std::string("info"),std::string("null of:oldpassword.")));
            responseParams.insert(std::make_pair(std::string("result"),std::string("fail")));
        }else if(requestDatas.at("newpassword").length()==0){
            responseParams.insert(std::make_pair(std::string("info"),std::string("null of:newpassword.")));
            responseParams.insert(std::make_pair(std::string("result"),std::string("fail")));
        }

        QString querySqlA = "select password from agv_user where username=?";
        QStringList params;
        params<<QString::fromStdString(requestDatas["username"]);
        QList<QStringList> queryresult = g_sql->query(querySqlA,params);
        if(queryresult.length()==0){
            responseParams.insert(std::make_pair(std::string("info"),std::string("not exist:username.")));
            responseParams.insert(std::make_pair(std::string("result"),std::string("fail")));
        }else{
            if(queryresult.at(0).contains(QString::fromStdString(requestDatas["oldpassword"]))){
                /////TODO:设置新的密码
                responseParams.insert(std::make_pair(std::string("info"),std::string("")));
                responseParams.insert(std::make_pair(std::string("result"),std::string("success")));
            }else{
                responseParams.insert(std::make_pair(std::string("info"),std::string("not correct:oldpassword.")));
                responseParams.insert(std::make_pair(std::string("result"),std::string("fail")));
            }
        }
    }

    ////////////////////////获取用户列表
    else if(requestDatas["type"]=="user" && requestDatas["todo"]=="list"){
        //要求含有username和password
        if(requestDatas.find("username")==requestDatas.end()){
            //用户名没有
            responseParams.insert(std::make_pair(std::string("info"),std::string("param lack:username.")));
            responseParams.insert(std::make_pair(std::string("result"),std::string("fail")));
        }else if(requestDatas.find("password")==requestDatas.end()){
            //密码没有
            responseParams.insert(std::make_pair(std::string("info"),std::string("param lack:password.")));
            responseParams.insert(std::make_pair(std::string("result"),std::string("fail")));
        }else if(requestDatas.at("username").length()==0){
            responseParams.insert(std::make_pair(std::string("info"),std::string("null of:username.")));
            responseParams.insert(std::make_pair(std::string("result"),std::string("fail")));
        }else if(requestDatas.at("password").length()==0){
            responseParams.insert(std::make_pair(std::string("info"),std::string("null of:password.")));
            responseParams.insert(std::make_pair(std::string("result"),std::string("fail")));
        }
        QString querySqlA = "select password,role from agv_user where username=?";
        QStringList params;
        params<<QString::fromStdString(requestDatas["username"]);
        QList<QStringList> queryresult = g_sql->query(querySqlA,params);
        if(queryresult.length()==0){
            responseParams.insert(std::make_pair(std::string("info"),std::string("not exist:username")));
            responseParams.insert(std::make_pair(std::string("result"),std::string("fail")));
        }else{
            if(queryresult.at(0).at(0) != QString::fromStdString(requestDatas["password"])){
                //登录失败
                responseParams.insert(std::make_pair(std::string("info"),std::string("not correct:password")));
                responseParams.insert(std::make_pair(std::string("result"),std::string("fail")));
            }else{
                //校验完成，开始查询用户
                //只能查看到同等级或者低等级的用户
                QString querySqlB = "select username,password,status,lastSignTime,createTime,role  from agv_user where role<=?";
                QStringList paramsB;
                paramsB<<queryresult.at(0).at(1);
                QList<QStringList> queryresultB = g_sql->query(querySqlB,paramsB);

                responseParams.insert(std::make_pair(std::string("info"),std::string("")));
                responseParams.insert(std::make_pair(std::string("result"),std::string("success")));

                for(int i=0;i<queryresultB.length();++i)
                {
                    if(queryresultB.at(i).length() == 6){
                        std::map<std::string,std::string> userinfo;
                        userinfo.insert(std::make_pair(std::string("username"),queryresultB.at(i).at(0).toStdString()));
                        userinfo.insert(std::make_pair(std::string("password"),queryresultB.at(i).at(1).toStdString()));
                        userinfo.insert(std::make_pair(std::string("status"),queryresultB.at(i).at(2).toStdString()));
                        userinfo.insert(std::make_pair(std::string("lastSignTime"),queryresultB.at(i).at(3).toStdString()));
                        userinfo.insert(std::make_pair(std::string("createTime"),queryresultB.at(i).at(4).toStdString()));
                        userinfo.insert(std::make_pair(std::string("role"),queryresultB.at(i).at(5).toStdString()));
                        responseDatalists.push_back(userinfo);
                    }
                }
            }
        }
    }

    ////删除用户
    else if(requestDatas["type"]=="user" && requestDatas["todo"]=="delete"){
        //要求含有username和password
        if(requestDatas.find("username")==requestDatas.end()){
            //用户名没有
            responseParams.insert(std::make_pair(std::string("info"),std::string("param lack:username.")));
            responseParams.insert(std::make_pair(std::string("result"),std::string("fail")));
        }else if(requestDatas.find("password")==requestDatas.end()){
            //密码没有
            responseParams.insert(std::make_pair(std::string("info"),std::string("param lack:password.")));
            responseParams.insert(std::make_pair(std::string("result"),std::string("fail")));

        }else if(requestDatas.at("username").length()==0){
            responseParams.insert(std::make_pair(std::string("info"),std::string("null of:username.")));
            responseParams.insert(std::make_pair(std::string("result"),std::string("fail")));
        }else if(requestDatas.at("password").length()==0){
            responseParams.insert(std::make_pair(std::string("info"),std::string("null of:password.")));
            responseParams.insert(std::make_pair(std::string("result"),std::string("fail")));
        }
        QString querySqlA = "select count(*) from agv_user where username=?";
        QStringList params;
        params<<QString::fromStdString(requestDatas["username"]);
        QList<QStringList> queryresult = g_sql->query(querySqlA,params);
        if(queryresult.length()==0||queryresult.at(0).at(0).toInt()<=0){
            responseParams.insert(std::make_pair(std::string("info"),std::string("not exist:username")));
            responseParams.insert(std::make_pair(std::string("result"),std::string("fail")));
        }else{
            /////删除这个用户 计入日志
            QString deleteSql = "delete from agv_user where username=?";
            if(!g_sql->exec(deleteSql,params)){
                responseParams.insert(std::make_pair(std::string("info"),std::string("delete fail!")));
                responseParams.insert(std::make_pair(std::string("result"),std::string("fail")));
            }else{
                responseParams.insert(std::make_pair(std::string("info"),std::string("")));
                responseParams.insert(std::make_pair(std::string("result"),std::string("success")));
            }
        }
    }

    /////添加用户
    else if(requestDatas["type"]=="user" && requestDatas["todo"]=="add"){//<username><password><realname><sex><age><role>

        //////三项必填的:username,password,role
        if(requestDatas.find("username")==requestDatas.end()){
            responseParams.insert(std::make_pair(std::string("info"),std::string("param lack:username.")));
            responseParams.insert(std::make_pair(std::string("result"),std::string("fail")));
        }else if(requestDatas.find("password")==requestDatas.end()){
            responseParams.insert(std::make_pair(std::string("info"),std::string("param lack:password.")));
            responseParams.insert(std::make_pair(std::string("result"),std::string("fail")));
        }else if(requestDatas.find("role")==requestDatas.end()){
            responseParams.insert(std::make_pair(std::string("info"),std::string("param lack:role.")));
            responseParams.insert(std::make_pair(std::string("result"),std::string("fail")));
        }else if(requestDatas.at("username").length()==0){
            responseParams.insert(std::make_pair(std::string("info"),std::string("null of:username.")));
            responseParams.insert(std::make_pair(std::string("result"),std::string("fail")));
        }else if(requestDatas.at("password").length()==0){
            responseParams.insert(std::make_pair(std::string("info"),std::string("null of:password.")));
            responseParams.insert(std::make_pair(std::string("result"),std::string("fail")));
        }else if(requestDatas.at("password").length()==0){
            responseParams.insert(std::make_pair(std::string("info"),std::string("null of:role.")));
            responseParams.insert(std::make_pair(std::string("result"),std::string("fail")));
        }

        QString username =QString::fromStdString( requestDatas.at("username"));
        QString password =QString::fromStdString( requestDatas.at("password"));
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

        QString addSql = "insert into agv_user(username,password,role,realname,sex,age)values(?,?,?,?,?,?);";
        QStringList params;
        params<<username<<password<<role<<realName<<QString("%1").arg(sex)<<QString("%1").arg(age);
        if(g_sql->exec(addSql,params)){
            //成功
            responseParams.insert(std::make_pair(std::string("info"),std::string("")));
            responseParams.insert(std::make_pair(std::string("result"),std::string("success")));
        }else{
            //失败
            responseParams.insert(std::make_pair(std::string("info"),std::string("insert into fail")));
            responseParams.insert(std::make_pair(std::string("result"),std::string("fail")));
        }
    }

    /////////////////////////////地图部分
    /// 站点列表
    else if(requestDatas["type"]=="map" && requestDatas["todo"]=="stationlist"){
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
    /// 线路列表
    else if(requestDatas["type"]=="map" && requestDatas["todo"]=="linelist"){
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
    /// 订阅车辆位置信息
    else if(requestDatas["type"]=="map" && requestDatas["todo"]=="subscribe"){
        //将sock加入到车辆位置订阅者丢列中
        if(g_msgCenter.addAgvPostionSubscribe(item.sock)){
            responseParams.insert(std::make_pair(std::string("info"),std::string("")));
            responseParams.insert(std::make_pair(std::string("result"),std::string("success")));
        }else{
            responseParams.insert(std::make_pair(std::string("info"),std::string("unknow error")));
            responseParams.insert(std::make_pair(std::string("result"),std::string("fail")));
        }
    }

    //封装
    std::string xml = getResponseXml(responseParams,responseDatalists);
    //发送
    g_netWork->sendToOne(item.sock,xml.c_str(),xml.length());

}
