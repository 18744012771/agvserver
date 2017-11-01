#include "usermsgprocessor.h"
#include "util/global.h"
#include "pugixml.hpp"


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

    pugi::xml_document doc;
    pugi::xml_parse_result parseResult =  doc.load_buffer(oneMsg.c_str(), oneMsg.length());
    if(parseResult.status != pugi::status_ok){
        qDebug() << QStringLiteral("收到的xml解析错误:")<<oneMsg.c_str();
        return ;//解析错误，说明xml格式不正确
    }
    pugi::xml_node xmlRoot = doc.child("xml");

    for (pugi::xml_node child: xmlRoot.children())
    {
        if(strcmp(child.name(),"data")!=0){
            qDebug() << child.name()<<":"<<child.child_value();
            params.insert(std::make_pair(std::string(child.name()),std::string(child.child_value())));
        }else{
            for (pugi::xml_node ccchild: child.children())
            {
                if(strcmp(ccchild.name(),"datalist")!=0){
                    qDebug() << ccchild.name()<<":"<<ccchild.child_value();
                    params.insert(std::make_pair(std::string(ccchild.name()),std::string(ccchild.child_value())));
                }else{
                    for (pugi::xml_node ccccccchild: ccchild.children())
                    {
                        std::map<std::string,std::string> datalist_list;
                        if(strcmp(ccccccchild.name(),"list")==0){
                            for (pugi::xml_node cccccccccccccccccccchild: ccccccchild.children())
                            {
                                if(strcmp(ccccccchild.name(),"list")==0){
                                    datalist_list.insert(std::make_pair(std::string(cccccccccccccccccccchild.name()),std::string(cccccccccccccccccccchild.child_value())));
                                }
                            }
                        }
                        if(datalist_list.size()>0)datalist.push_back(datalist_list);
                    }
                }
            }
        }
    }

    std::map<std::string,std::string>::iterator itr;
    if((itr=params.find("type"))!=params.end()
            &&(itr=params.find("todo"))!=params.end()
            &&(itr=params.find("queuenumber"))!=params.end()
            ){
        qDebug()<<"get a good msg"<<params.size()<<datalist.size();
        //接下来对这条消息进行响应
        responseOneMsg(item,params,datalist);
    }else{
        qDebug()<<"get a error msg";
        //什么也不做!
    }
}

//对接收到的消息，进行处理！
void UserMsgProcessor::responseOneMsg(const QyhMsgDateItem &item,std::map<string,string> requestDatas,std::vector<std::map<std::string,std::string> > datalists)
{
    std::map<std::string,std::string> responseParams;
    std::vector<std::map<std::string,std::string> > responseDatalists;
    //先把头列好
    responseParams.insert(std::make_pair(std::string("type"),requestDatas["type"]));
    responseParams.insert(std::make_pair(std::string("todo"),requestDatas["type"]));
    responseParams.insert(std::make_pair(std::string("queuenumber"),requestDatas["queuenumber"]));

    //冗长的判断要开始了

    //////////////////请求登录
    if(requestDatas["type"]=="user" && requestDatas["todo"]=="login")
    {
        //要求含有username和password
        if(requestDatas.find("username")==requestDatas.end()){
            //用户名没有
            responseParams.insert(std::make_pair(std::string("info"),std::string("param lack:username.")));
            responseParams.insert(std::make_pair(std::string("result"),requestDatas["fail"]));
        }else if(requestDatas.find("password")==requestDatas.end()){
            //密码没有
            responseParams.insert(std::make_pair(std::string("info"),std::string("param lack:password.")));
            responseParams.insert(std::make_pair(std::string("result"),requestDatas["fail"]));

        }else if(requestDatas.at("username").length()==0){
            responseParams.insert(std::make_pair(std::string("info"),std::string("null of:username.")));
            responseParams.insert(std::make_pair(std::string("result"),requestDatas["fail"]));
        }else if(requestDatas.at("password").length()==0){
            responseParams.insert(std::make_pair(std::string("info"),std::string("null of:password.")));
            responseParams.insert(std::make_pair(std::string("result"),requestDatas["fail"]));
        }
        QString querySqlA = "select password,role from agv_user where username=?";
        QStringList params;
        params<<requestDatas["username"];
        QList<QStringList> queryresult = g_sql->query(querySqlA,params);
        if(queryresult.length()==0){
            responseParams.insert(std::make_pair(std::string("info"),std::string("not exist:username")));
            responseParams.insert(std::make_pair(std::string("result"),requestDatas["fail"]));
        }else{
            if(queryresult.at(0).at(0) == requestDatas["password"]){
                //登录成功
                responseParams.insert(std::make_pair(std::string("info"),std::string("")));
                responseParams.insert(std::make_pair(std::string("role"),queryresult.at(0).at(1)));
                responseParams.insert(std::make_pair(std::string("result"),requestDatas["success"]));
                //TODO:设置登录状态和登录时间
            }else{
                //登录失败
                responseParams.insert(std::make_pair(std::string("info"),std::string("not correct:password")));
                responseParams.insert(std::make_pair(std::string("result"),requestDatas["fail"]));
            }
        }
    }

    //////////////////退出登录
    else if(requestDatas["type"]=="user" && requestDatas["todo"]=="logout"){
        //要求含有username和password
        if(requestDatas.find("username")==requestDatas.end()){
            //用户名没有
            responseParams.insert(std::make_pair(std::string("info"),std::string("param lack:username.")));
            responseParams.insert(std::make_pair(std::string("result"),requestDatas["fail"]));
        }else if(requestDatas.at("username").length()==0){
            responseParams.insert(std::make_pair(std::string("info"),std::string("null of:username.")));
            responseParams.insert(std::make_pair(std::string("result"),requestDatas["fail"]));
        }
        QString querySqlA = "select status from agv_user where username=?";
        QStringList params;
        params<<requestDatas["username"];
        QList<QStringList> queryresult = g_sql->query(querySqlA,params);
        if(queryresult.length()==0){
            responseParams.insert(std::make_pair(std::string("info"),std::string("null of:username.")));
            responseParams.insert(std::make_pair(std::string("result"),requestDatas["fail"]));
        }else{
            ////TODO:设置登录状态
            responseParams.insert(std::make_pair(std::string("info"),std::string("")));
            responseParams.insert(std::make_pair(std::string("result"),requestDatas["success"]));
        }
    }


    ////////////////////////修改密码
    else if(requestDatas["type"]=="user" && requestDatas["todo"]=="changepassword"){
        //要求含有username和oldpassword newpassword
        if(requestDatas.find("username")==requestDatas.end()){
            responseParams.insert(std::make_pair(std::string("info"),std::string("param lack:username.")));
            responseParams.insert(std::make_pair(std::string("result"),requestDatas["fail"]));
        }else if(requestDatas.find("oldpassword")==requestDatas.end()){
            responseParams.insert(std::make_pair(std::string("info"),std::string("param lack:oldpassword.")));
            responseParams.insert(std::make_pair(std::string("result"),requestDatas["fail"]));
        }else if(requestDatas.find("newpassword")==requestDatas.end()){
            responseParams.insert(std::make_pair(std::string("info"),std::string("param lack:newpassword.")));
            responseParams.insert(std::make_pair(std::string("result"),requestDatas["fail"]));
        }else if(requestDatas.at("username").length()==0){
            responseParams.insert(std::make_pair(std::string("info"),std::string("null of:username.")));
            responseParams.insert(std::make_pair(std::string("result"),requestDatas["fail"]));
        }else if(requestDatas.at("oldpassword").length()==0){
            responseParams.insert(std::make_pair(std::string("info"),std::string("null of:oldpassword.")));
            responseParams.insert(std::make_pair(std::string("result"),requestDatas["fail"]));
        }else if(requestDatas.at("newpassword").length()==0){
            responseParams.insert(std::make_pair(std::string("info"),std::string("null of:newpassword.")));
            responseParams.insert(std::make_pair(std::string("result"),requestDatas["fail"]));
        }

        QString querySqlA = "select password from agv_user where username=?";
        QStringList params;
        params<<requestDatas["username"];
        QList<QStringList> queryresult = g_sql->query(querySqlA,params);
        if(queryresult.length()==0){
            responseParams.insert(std::make_pair(std::string("info"),std::string("not exist:username.")));
            responseParams.insert(std::make_pair(std::string("result"),requestDatas["fail"]));
        }else{
            if(queryresult.at(0).contains(requestDatas["oldpassword"])){
                /////TODO:设置新的密码
                responseParams.insert(std::make_pair(std::string("info"),std::string("")));
                responseParams.insert(std::make_pair(std::string("result"),requestDatas["success"]));
            }else{
                responseParams.insert(std::make_pair(std::string("info"),std::string("not correct:oldpassword.")));
                responseParams.insert(std::make_pair(std::string("result"),requestDatas["fail"]));
            }
        }
    }

    ////////////////////////获取用户列表
    else if(requestDatas["type"]=="user" && requestDatas["todo"]=="list"){
        //要求含有username和password
        if(requestDatas.find("username")==requestDatas.end()){
            //用户名没有
            responseParams.insert(std::make_pair(std::string("info"),std::string("param lack:username.")));
            responseParams.insert(std::make_pair(std::string("result"),requestDatas["fail"]));
        }else if(requestDatas.find("password")==requestDatas.end()){
            //密码没有
            responseParams.insert(std::make_pair(std::string("info"),std::string("param lack:password.")));
            responseParams.insert(std::make_pair(std::string("result"),requestDatas["fail"]));
        }else if(requestDatas.at("username").length()==0){
            responseParams.insert(std::make_pair(std::string("info"),std::string("null of:username.")));
            responseParams.insert(std::make_pair(std::string("result"),requestDatas["fail"]));
        }else if(requestDatas.at("password").length()==0){
            responseParams.insert(std::make_pair(std::string("info"),std::string("null of:password.")));
            responseParams.insert(std::make_pair(std::string("result"),requestDatas["fail"]));
        }
        QString querySqlA = "select password,role from agv_user where username=?";
        QStringList params;
        params<<requestDatas["username"];
        QList<QStringList> queryresult = g_sql->query(querySqlA,params);
        if(queryresult.length()==0){
            responseParams.insert(std::make_pair(std::string("info"),std::string("not exist:username")));
            responseParams.insert(std::make_pair(std::string("result"),requestDatas["fail"]));
        }else{
            if(queryresult.at(0).at(0) != requestDatas["password"]){
                //登录失败
                responseParams.insert(std::make_pair(std::string("info"),std::string("not correct:password")));
                responseParams.insert(std::make_pair(std::string("result"),requestDatas["fail"]));
            }else{
                //校验完成，开始查询用户
                //只能查看到同等级或者低等级的用户
                QString querySqlB = "select username,password,status,lastSignTime,createTime,role  from agv_user where role<=?";
                QStringList paramsB;
                paramsB<<queryresult.at(0).at(1);
                QList<QStringList> queryresultB = g_sql->query(querySqlB,paramsB);

                responseParams.insert(std::make_pair(std::string("info"),std::string("")));
                responseParams.insert(std::make_pair(std::string("result"),requestDatas["success"]));

                for(int i=0;i<queryresultB.length();++i)
                {
                    if(queryresultB.at(i).length() == 6){
                        std::map<std::string,std::string> userinfo;
                        userinfo.insert(std::make_pair(std::string("username"),queryresultB.at(0)));
                        userinfo.insert(std::make_pair(std::string("password"),queryresultB.at(1)));
                        userinfo.insert(std::make_pair(std::string("status"),queryresultB.at(2)));
                        userinfo.insert(std::make_pair(std::string("lastSignTime"),queryresultB.at(3)));
                        userinfo.insert(std::make_pair(std::string("createTime"),queryresultB.at(4)));
                        userinfo.insert(std::make_pair(std::string("role"),queryresultB.at(5)));
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
            responseParams.insert(std::make_pair(std::string("result"),requestDatas["fail"]));
        }else if(requestDatas.find("password")==requestDatas.end()){
            //密码没有
            responseParams.insert(std::make_pair(std::string("info"),std::string("param lack:password.")));
            responseParams.insert(std::make_pair(std::string("result"),requestDatas["fail"]));

        }else if(requestDatas.at("username").length()==0){
            responseParams.insert(std::make_pair(std::string("info"),std::string("null of:username.")));
            responseParams.insert(std::make_pair(std::string("result"),requestDatas["fail"]));
        }else if(requestDatas.at("password").length()==0){
            responseParams.insert(std::make_pair(std::string("info"),std::string("null of:password.")));
            responseParams.insert(std::make_pair(std::string("result"),requestDatas["fail"]));
        }
        QString querySqlA = "select count(*) from agv_user where username=?";
        QStringList params;
        params<<requestDatas["username"];
        QList<QStringList> queryresult = g_sql->query(querySqlA,params);
        if(queryresult.length()==0||queryresult.at(0).at(0).toInt()<=0){
            responseParams.insert(std::make_pair(std::string("info"),std::string("not exist:username")));
            responseParams.insert(std::make_pair(std::string("result"),requestDatas["fail"]));
        }else{
            /////删除这个用户 计入日志
            QString deleteSql = "delete from agv_user where username=?";
            if(!g_sql->exec(deleteSql,params)){
                esponseParams.insert(std::make_pair(std::string("info"),std::string("delete fail!")));
                responseParams.insert(std::make_pair(std::string("result"),requestDatas["fail"]));
            }else{
                esponseParams.insert(std::make_pair(std::string("info"),std::string("")));
                responseParams.insert(std::make_pair(std::string("result"),requestDatas["success"]));
            }
        }
    }


    /////添加用户
    else if(requestDatas["type"]=="user" && requestDatas["todo"]=="add"){//<username><password><realname><sex><age><role>

        //////三项必填的:username,password,role
        if(requestDatas.find("username")==requestDatas.end()){
            responseParams.insert(std::make_pair(std::string("info"),std::string("param lack:username.")));
            responseParams.insert(std::make_pair(std::string("result"),requestDatas["fail"]));
        }else if(requestDatas.find("password")==requestDatas.end()){
            responseParams.insert(std::make_pair(std::string("info"),std::string("param lack:password.")));
            responseParams.insert(std::make_pair(std::string("result"),requestDatas["fail"]));
        }else if(requestDatas.find("role")==requestDatas.end()){
            responseParams.insert(std::make_pair(std::string("info"),std::string("param lack:role.")));
            responseParams.insert(std::make_pair(std::string("result"),requestDatas["fail"]));
        }else if(requestDatas.at("username").length()==0){
            responseParams.insert(std::make_pair(std::string("info"),std::string("null of:username.")));
            responseParams.insert(std::make_pair(std::string("result"),requestDatas["fail"]));
        }else if(requestDatas.at("password").length()==0){
            responseParams.insert(std::make_pair(std::string("info"),std::string("null of:password.")));
            responseParams.insert(std::make_pair(std::string("result"),requestDatas["fail"]));
        }else if(requestDatas.at("password").length()==0){
            responseParams.insert(std::make_pair(std::string("info"),std::string("null of:role.")));
            responseParams.insert(std::make_pair(std::string("result"),requestDatas["fail"]));
        }

        QString username =QString::fromStdString( requestDatas.at("username"));
        QString password =QString::fromStdString( requestDatas.at("password"));
        QString role =QString::fromStdString( requestDatas.at("role"));

        //查看剩余项目是否存在
        QString realName="";
        bool sex = true;
        int age=20;
        if(requestDatas.find("realname")!=requestDatas.end() &&requestDatas.at("realname").length()>0){
            realName = requestDatas.at("realname");
        }
        if(requestDatas.find("sex")!=requestDatas.end() &&requestDatas.at("sex").length()>0){
            sex = requestDatas.at("sex")=="1";
        }
        if(requestDatas.find("age")!=requestDatas.end() &&requestDatas.at("age").length()>0){
            age = QString::fromStdString(requestDatas.at("sex")).toInt;
        }

        QString addSql = "insert into agv_user(username,password,role,realname,sex,age)values(?,?,?,?,?,?);";
        QStringList params;
        params<<username<<password<<role<<realName<<sex<<age;
        if(g_sql->exec(addSql,params)){
            //成功
            responseParams.insert(std::make_pair(std::string("info"),std::string("")));
            responseParams.insert(std::make_pair(std::string("result"),requestDatas["success"]));
        }else{
            //失败
            responseParams.insert(std::make_pair(std::string("info"),std::string("insert into fail")));
            responseParams.insert(std::make_pair(std::string("result"),requestDatas["fail"]));
        }
    }

    goResponse(item,responseParams,responseDatalists);

    return true;
}

void goResponse(const QyhMsgDateItem &item,std::map<string,string> responseDatas,std::vector<std::map<std::string,std::string> > responseDatalists)
{

}
