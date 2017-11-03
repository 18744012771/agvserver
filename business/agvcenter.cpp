#include "agvcenter.h"
#include "util/global.h"

AgvCenter::AgvCenter(QObject *parent) : QObject(parent)
{

}

//获取空闲的车辆
QList<Agv *> AgvCenter::getIdleAgvs()
{
    QList<Agv *> result;
    for(QMap<int,Agv *>::iterator itr =g_m_agvs.begin();itr!=g_m_agvs.end();++itr)
    {
        if(itr.value()->status() == AGV_STATUS_IDLE){
            result.push_back(itr.value());
        }
    }
    return result;
}

bool AgvCenter::agvStartTask(int agvId, QList<int> path)
{
    //TODO:这里需要启动小车，告诉小车下一站和下几站，还有就是左中右信息
    g_m_agvs[agvId]->setCurrentPath(path);
    QByteArray qba =  g_msgCenter.taskControlCmd(agvId,false);
    //组包完成，发送
    return g_m_agvs[agvId]->sendToAgv(qba);
}

bool AgvCenter::load()//从数据库载入所有的agv
{
    QString querySql = "select id,name,ip from agv_agv";
    QStringList params;
    QList<QStringList> result = g_sql->query(querySql,params);
    for(int i=0;i<result.length();++i){
        QStringList qsl = result.at(i);
        if(qsl.length() == 3){
            Agv *agv = new Agv;
            agv->setId(qsl.at(0).toInt());
            agv->setName(qsl.at(1));
            agv->setIp(qsl.at(2));
            connect(agv,SIGNAL(carArrivleStation(int,int)),this,SIGNAL(carArriveStation(int,int)));
            g_m_agvs.insert(agv->id(),agv);
        }
    }
    return true;
}

bool AgvCenter::save()//将agv保存到数据库
{
    //查询所有的，
    QList<int> selectAgvIds;
    QStringList params;
    QString querySql = "select id from agv_agv";
    QList<QStringList> queryResult = g_sql->query(querySql,params);
    for(int i=0;i<queryResult.length();++i){
        QStringList qsl = queryResult.at(i);
        if(qsl.length()==1){
            int id = qsl.at(0).toInt();
            selectAgvIds.push_back(id);
        }
    }

    for(int i=0;i<selectAgvIds.length();++i)
    {
        if(g_m_agvs.contains(selectAgvIds.at(i))){
            //含有,进行更新
            QString updateSql = "update agv_agv set name=?,ip=? where id=?";
            params.clear();
            params<<g_m_agvs[selectAgvIds.at(i)]->name()<<g_m_agvs[selectAgvIds.at(i)]->ip()<<QString("%1").arg(g_m_agvs[selectAgvIds.at(i)]->id());
            if(!g_sql->exec(updateSql,params))
                return false;
        }else{
            //不含有，就删除
            QString deleteSql = "delete from agv_agv where id=?";
            params.clear();params.push_back(QString("%1").arg(selectAgvIds.at(i)));
            if(g_sql->exec(deleteSql,params)){
                selectAgvIds.removeAt(i);
                --i;
            }else{
                return false;
            }
        }
    }

    //如果在g_m_agvs中有更多的呢，怎么呢，插入
    for(QMap<int,Agv *>::iterator itr=g_m_agvs.begin();itr!=g_m_agvs.end();++itr){
        if(selectAgvIds.contains(itr.key()))continue;
        //插入操作
        QString insertSql = "insert into agv_agv(id,name,ip) values(?,?,?)";
        params.clear();
        params<<QString("%1").arg(itr.value()->id())<<itr.value()->name()<<itr.value()->ip();
        if(!g_sql->exec(insertSql,params))return false;
    }
    return true;
}
