#include "global.h"
#include <QTime>
#include <QCoreApplication>
#include <sstream>
#include "pugixml.hpp"

//全局的一些common变量
Sql *g_sql = NULL;//数据库执行
AgvLog *g_log = NULL;//日志调用
AgvLogProcess *g_logProcess = NULL;//日志存库、publish
QyhZmqServer *g_server;//

//所有的业务处理
MapCenter g_agvMapCenter;//地图管理(地图载入，地图保存，地图计算)
TaskCenter g_taskCenter;//任务管理(任务分配，任务保存，任务调度)
AgvCenter g_hrgAgvCenter;//车辆管理(车辆载入。车辆保存。车辆增加。车辆删除)
UserMsgProcessor *userMsgProcessor = NULL;
TaskMaker *g_taskMaker;

const QString DATE_TIME_FORMAT = "yyyy-MM-dd hh:mm:ss";//统一时间格式

//公共函数
void QyhSleep(int msec)
{
    QTime dieTime = QTime::currentTime().addMSecs(msec);

    while( QTime::currentTime() < dieTime )
        QCoreApplication::processEvents(QEventLoop::AllEvents, 1);
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

QMap<int,std::string> client2serverBuffer;
moodycamel::ConcurrentQueue<OneLog> g_log_queue;

std::string getResponseXml(QMap<QString,QString> &responseDatas, QList<QMap<QString,QString> > &responseDatalists)
{
    pugi::xml_document doc;
    pugi::xml_node root  = doc.append_child("xml");
    //type
    pugi::xml_node type  = root.append_child("type");
    type.text().set(responseDatas["type"].toStdString().c_str());

    //todo
    pugi::xml_node todo  = root.append_child("todo");
    todo.text().set(responseDatas["todo"].toStdString().c_str());

    //queuenumber
    if(responseDatas.find("queuenumber")!=responseDatas.end()){
        pugi::xml_node queuenumber  = root.append_child("queuenumber");
        queuenumber.text().set(responseDatas["queuenumber"].toStdString().c_str());
    }

    //data
    pugi::xml_node data  = root.append_child("data");
    for (QMap<QString,QString>::iterator itr=responseDatas.begin(); itr!=responseDatas.end(); ++itr)
    {
        if(itr.key() == "todo"||itr.key()=="type"||itr.key()=="queuenumber")continue;
        data.append_child(itr.key().toStdString().c_str()).text().set(itr.value().toStdString().c_str());
    }

    //datalist
    if(responseDatalists.size()>0){
        pugi::xml_node datalist  = data.append_child("datalist");
        for(QList<QMap<QString,QString> >::iterator itr=responseDatalists.begin();itr!=responseDatalists.end();++itr){
            pugi::xml_node list  = datalist.append_child("list");
            for(QMap<QString,QString>::iterator pos = itr->begin();pos!=itr->end();++pos){
                list.append_child(pos.key().toStdString().c_str()).text().set(pos.value().toStdString().c_str());
            }
        }
    }

    //封装完成
    std::stringstream result;
    doc.print(result, "", pugi::format_raw);
    return result.str();
}

bool getRequestParam(const std::string &xmlStr,QMap<QString,QString> &params,QList<QMap<QString,QString> > &datalist)
{
    pugi::xml_document doc;
    pugi::xml_parse_result parseResult =  doc.load_buffer(xmlStr.c_str(), xmlStr.length());
    if(parseResult.status != pugi::status_ok){
        g_log->log(AGV_LOG_LEVEL_ERROR,QStringLiteral("收到的xml解析错误:")+QString::fromStdString(xmlStr));
        return false;//解析错误，说明xml格式不正确
    }

    pugi::xml_node xmlRoot = doc.child("xml");
    for (pugi::xml_node child: xmlRoot.children())
    {
        if(strcmp(child.name(),"data")!=0){
            params.insert(QString(child.name()),QString(child.child_value()));
        }else{
            for (pugi::xml_node ccchild: child.children())
            {
                if(strcmp(ccchild.name(),"datalist")!=0){
                    params.insert(QString(ccchild.name()),QString(ccchild.child_value()));
                }else{
                    for (pugi::xml_node ccccchild: ccchild.children())
                    {
                        QMap<QString,QString> datalist_list;
                        if(strcmp(ccccchild.name(),"list")==0){
                            for (pugi::xml_node cccccccchild: ccccchild.children())
                            {
                                if(strcmp(ccccchild.name(),"list")==0){
                                    datalist_list.insert(QString(cccccccchild.name()),QString(cccccccchild.child_value()));
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

std::string intToStdString(int x)
{
    std::string result;
    std::stringstream ss;
    ss<<x;
    ss>>result;
    return result;
}

bool agvTaskLessThan( const Task *a, const Task *b )
{
    Task aa = *a;
    Task bb = *b;
    return aa < bb;
}

unsigned char crc(unsigned char *data,int len)
{
    int sum=0;
    for(int i=0;i<len;++i)
    {
        sum+=data[i]&0xFF;
        sum&=0xFF;
    }
    return sum;
}
