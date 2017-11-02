#include "global.h"
#include <sstream>
#include <QTime>
#include <QCoreApplication>
#include "pugixml.hpp"

//全局的一些common变量
QString g_strExeRoot;
Sql *g_sql = NULL;
Log *g_log = NULL;
SqlServer *g_sqlServer = NULL;
AgvNetWork *g_netWork;//服务器中心

//所有的bean集合
QMap<int,Agv *> g_m_agvs;             //所有车辆们
QMap<int,AgvStation *> g_m_stations;  //所有的站点(站点+线路 = 地图)
QMap<int,AgvLine *> g_m_lines;        //所有的线路(站点+线路 = 地图)
QMap<PATH_LEFT_MIDDLE_RIGHT,int> g_m_lmr;//用来保存左中右信息，用于通知agv左中右信息
QMap<int,QVector<AgvLine*> > g_m_l_adj;  //从一条线路到另一条线路的关联表。用来计算可到达的位置
QMap<int,int> g_reverseLines;           //线路和它的反方向线路的集合。

//所有的业务处理
MapCenter g_agvMapCenter;//地图管理(地图载入，地图保存，地图计算)
TaskCenter g_taskCenter;//任务管理(任务分配，任务保存，任务调度)
AgvCenter g_hrgAgvCenter;//车辆管理(车辆载入。车辆保存。车辆增加。车辆删除)
MsgCenter g_msgCenter;   //消息处理中心，对所有的消息进行解析和组装等

///登录的客户端的信息
std::list<LoginUserInfo> loginUserIdSock;

//公共函数
void QyhSleep(int msec)
{
    QTime dieTime = QTime::currentTime().addMSecs(msec);

    while( QTime::currentTime() < dieTime )
        QCoreApplication::processEvents(QEventLoop::AllEvents, 100);
}

int getRandom(int maxRandom)
{
    QTime t;
    t= QTime::currentTime();
    qsrand(t.msec()+t.second()*1000);
    if(maxRandom>0)
        return qrand()%maxRandom;
    return qrand();
}

moodycamel::ConcurrentQueue<QyhMsgDateItem> g_user_msg_queue;
std::map<int,std::string> client2serverBuffer;

std::string getResponseXml(std::map<std::string,std::string> &responseDatas, std::vector<std::map<std::string,std::string> > &responseDatalists)
{
    pugi::xml_document doc;
    pugi::xml_node root  = doc.append_child("xml");
    //type
    pugi::xml_node type  = root.append_child("type");
    type.text().set(responseDatas.at("type").c_str());

    //todo
    pugi::xml_node todo  = root.append_child("todo");
    todo.text().set(responseDatas.at("todo").c_str());

    //queuenumber
    if(responseDatas.find("queuenumber")!=responseDatas.end()){
        pugi::xml_node queuenumber  = root.append_child("queuenumber");
        queuenumber.text().set(responseDatas.at("queuenumber").c_str());
    }

    //data
    pugi::xml_node data  = root.append_child("data");
    for (std::map<std::string,std::string>::iterator itr=responseDatas.begin(); itr!=responseDatas.end(); ++itr)
    {
        if(itr->first == "todo"||itr->first=="type"||itr->first=="queuenumber")continue;
        data.append_child(itr->first.c_str()).text().set(itr->second.c_str());
    }

    //datalist
    if(responseDatalists.size()>0){
        pugi::xml_node datalist  = data.append_child("datalist");
        for(std::vector<std::map<std::string,std::string> >::iterator itr=responseDatalists.begin();itr!=responseDatalists.end();++itr){
            pugi::xml_node list  = datalist.append_child("list");
            for(std::map<std::string,std::string>::iterator pos = itr->begin();pos!=itr->end();++pos){
                list.append_child(pos->first.c_str()).text().set(pos->second.c_str());
            }
        }
    }

    //封装完成
    std::stringstream result;
    doc.print(result, "", pugi::format_raw);
    return result.str();
}

bool getRequestParam(const std::string &xmlStr,std::map<std::string,std::string> &params,std::vector<std::map<std::string,std::string> > &datalist)
{
    pugi::xml_document doc;
    pugi::xml_parse_result parseResult =  doc.load_buffer(xmlStr.c_str(), xmlStr.length());
    if(parseResult.status != pugi::status_ok){
        qDebug() << QStringLiteral("收到的xml解析错误:")<<xmlStr.c_str();
        return false;//解析错误，说明xml格式不正确
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
    return true;
}


