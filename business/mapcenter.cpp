#include "mapcenter.h"

#define _USE_MATH_DEFINES
#include <math.h>
#include "util/global.h"

#include "util/bezierarc.h"

MapCenter::MapCenter(QObject *parent) : QObject(parent)
{

}

void MapCenter::clear()
{
    qDeleteAll(g_m_lines.values());
    qDeleteAll(g_m_stations.values());

    g_m_stations.clear();
    g_m_lines.clear();
    g_m_lmr.clear();
    g_m_l_adj.clear();
    g_reverseLines.clear();

    QString deleteStationSql = "delete from agv_station;";
    QList<QVariant> params;

    bool b = g_sql->exeSql(deleteStationSql,params);
    if(!b){
        g_log->log(AGV_LOG_LEVEL_ERROR,"can not clear table agv_station!");
    }
    QString deleteLineSql = "delete from agv_line;";
    b = g_sql->exeSql(deleteLineSql,params);
    if(!b){
        g_log->log(AGV_LOG_LEVEL_ERROR,"can not clear table agv_line!");
    }
    QString deleteLmrSql = "delete from agv_lmr;";
    b = g_sql->exeSql(deleteLmrSql,params);
    if(!b){
        g_log->log(AGV_LOG_LEVEL_ERROR,"can not clear table agv_lmr!");
    }
    QString deleteAdjSql = "delete from agv_adj;";
    b = g_sql->exeSql(deleteAdjSql,params);
    if(!b){
        g_log->log(AGV_LOG_LEVEL_ERROR,"can not clear table agv_adj!");
    }
}

void MapCenter::addStation(QString s)
{
    s = s.trimmed();
    if(s.endsWith(";"))s=s.left(s.length()-1);
    QStringList qsl = s.split(";");
    if(qsl.length()>=2){
        QString amoutStr = qsl.at(0);
        bool ok = false;
        int amount = amoutStr.toInt(&ok);
        if(!ok)return ;
        if(amount == qsl.length()-1){
            for(int i=1;i<qsl.length();++i){
                QString ss = qsl.at(i);
                QStringList pp = ss.split(",");
                if(pp.length()==8){
                    AgvStation *aStation = new AgvStation;
                    aStation->id = (pp.at(0).toInt());
                    aStation->name = pp.at(1);
                    aStation->x = (pp.at(2).toDouble());
                    aStation->y = (pp.at(3).toDouble());
                    aStation->rfid = (pp.at(4).toInt());
                    aStation->color_r = (pp.at(5).toInt());
                    aStation->color_g = (pp.at(6).toInt());
                    aStation->color_b = (pp.at(7).toInt());

                    QString insertSql = "INSERT INTO agv_station (id,station_name, station_x,station_y,station_rfid,station_color_r,station_color_g,station_color_b) VALUES (?,?,?,?,?,?,?,?);";
                    QList<QVariant> params;
                    params<<aStation->id<<aStation->name<<aStation->x<<aStation->y
                         <<aStation->rfid<<aStation->color_r<<aStation->color_g<<aStation->color_b;
                    if(g_sql->exeSql(insertSql,params)){
                        g_m_stations.insert(aStation->id,aStation);
                    }else{
                        g_log->log(AGV_LOG_LEVEL_ERROR,"save agv statiom to database fail!");
                        delete aStation;
                    }
                }
            }
        }
    }
    QString ss = "g_m_agvstations.length="+g_m_stations.keys().length();
    g_log->log(AGV_LOG_LEVEL_DEBUG,ss);

}

void MapCenter::addLine(QString s)
{
    s = s.trimmed();
    if(s.endsWith(";"))s=s.left(s.length()-1);
    QStringList qsl = s.split(";");
    if(qsl.length()>=2){
        QString amoutStr = qsl.at(0);
        bool ok = false;
        int amount = amoutStr.toInt(&ok);
        if(!ok)return ;
        if(amount == qsl.length()-1){
            for(int i=1;i<qsl.length();++i){
                QString ss = qsl.at(i);
                QStringList pp = ss.split(",");
                if(pp.length()==7){
                    AgvLine *aLine = new AgvLine;

                    aLine->id = (pp.at(0).toInt());
                    aLine->startStation = (pp.at(1).toInt());
                    aLine->endStation = (pp.at(2).toInt());
                    aLine->rate = (pp.at(3).toDouble());
                    aLine->line = (true);
                    aLine->draw = (true);
                    aLine->color_r = (pp.at(4).toInt());
                    aLine->color_g = (pp.at(5).toInt());
                    aLine->color_b = (pp.at(6).toInt());

                    if(!g_m_stations.contains(aLine->startStation)
                            ||!g_m_stations.contains(aLine->endStation)){
                        delete aLine;
                        continue;
                    }
                    double startX = g_m_stations[aLine->startStation]->x;
                    double startY = g_m_stations[aLine->startStation]->y;
                    double endX = g_m_stations[aLine->endStation]->x;
                    double endY = g_m_stations[aLine->endStation]->y;

                    aLine->length = sqrt((startX-endX)*(startX-endX)+(startY-endY)*(startY-endY));

                    QString insertSql = "INSERT INTO agv_line (id,line_startStation,line_endStation,line_rate,line_color_r,line_color_g,line_color_b,line_line,line_length,line_draw) VALUES (?,?,?,?,?,?,?,?,?,?);";
                    QList<QVariant> params;
                    params<<aLine->id<<aLine->startStation<<aLine->endStation<<aLine->rate<<aLine->color_r<<aLine->color_g<<aLine->color_b<<aLine->line<<aLine->length<<aLine->draw;

                    if(g_sql->exeSql(insertSql,params))
                    {
                        g_m_lines.insert(aLine->id,aLine);
                    }else{
                        g_log->log(AGV_LOG_LEVEL_ERROR,"save agv line to database fail!");
                        delete aLine;
                    }
                }
            }
        }
    }
    QString ss = "g_m_agvlines.length="+g_m_lines.size();
    g_log->log(AGV_LOG_LEVEL_DEBUG,ss);
}

void MapCenter::addArc(QString s)
{
    if(s.endsWith(";"))s=s.left(s.length()-1);
    QStringList qsl = s.split(";");
    if(qsl.length()>=2){
        QString amoutStr = qsl.at(0);
        bool ok = false;
        int amount = amoutStr.toInt(&ok);
        if(!ok)return ;
        if(amount == qsl.length()-1){
            for(int i=1;i<qsl.length();++i){
                QString ss = qsl.at(i);
                QStringList pp = ss.split(",");
                if(pp.length()==11){

                    AgvLine *aLine = new AgvLine;

                    aLine->id = (pp.at(0).toInt());
                    aLine->startStation = (pp.at(1).toInt());
                    aLine->endStation = (pp.at(2).toInt());
                    aLine->rate = (pp.at(3).toDouble());
                    aLine->line = (false);
                    aLine->draw = (true);
                    aLine->color_r = (pp.at(4).toInt());
                    aLine->color_g = (pp.at(5).toInt());
                    aLine->color_b = (pp.at(6).toInt());

                    aLine->p1x = (pp.at(7).toDouble());
                    aLine->p1y = (pp.at(8).toDouble());
                    aLine->p2x = (pp.at(9).toDouble());
                    aLine->p2y = (pp.at(10).toDouble());


                    if(!g_m_stations.contains(aLine->startStation)
                            ||!g_m_stations.contains(aLine->endStation)){
                        delete aLine;
                        continue;
                    }
                    double startX = g_m_stations[aLine->startStation]->x;
                    double startY = g_m_stations[aLine->startStation]->y;
                    double endX = g_m_stations[aLine->endStation]->x;
                    double endY = g_m_stations[aLine->endStation]->y;

                    aLine->length = BezierArc::BezierArcLength(QPointF(startX,startY),QPointF(aLine->p1x,aLine->p1y),QPointF(aLine->p2x,aLine->p2y),QPointF(endX,endY));

                    QString insertSql = "INSERT INTO agv_line (id,line_startStation,line_endStation,line_rate,line_color_r,line_color_g,line_color_b,line_line,line_length,line_draw,line_p1x,line_p1y,line_p2x,line_p2y) VALUES (?,?,?,?,?,?,?,?,?,?,?,?,?,?);";
                    QList<QVariant> params;
                    params<<aLine->id
                         <<aLine->startStation
                        <<aLine->endStation
                       <<aLine->rate
                      <<aLine->color_r
                     <<aLine->color_g
                    <<aLine->color_b
                    <<aLine->line
                    <<aLine->length
                    <<aLine->draw
                    <<aLine->p1x
                    <<aLine->p1y
                    <<aLine->p2x
                    <<aLine->p2y;

                    if(g_sql->exeSql(insertSql,params))
                    {
                        g_m_lines.insert(aLine->id,aLine);
                    }else{
                        g_log->log(AGV_LOG_LEVEL_ERROR,"save agv line to database fail!");
                        delete aLine;
                    }
                }
            }
        }
    }
    QString ss =  "g_m_agvlines.length="+g_m_lines.keys().size();
    g_log->log(AGV_LOG_LEVEL_DEBUG,ss);
}



int MapCenter::getLMR(AgvLine *lastLine,AgvLine *nextLine)
{
    if(lastLine->endStation != nextLine->startStation)return PATH_LMF_NOWAY;

    if(lastLine->id == 18||nextLine->id == 18){
        qDebug() << "18hao!";
    }


    double lastAngle,nextAngle;

    double l_startX = g_m_stations[lastLine->startStation]->x;
    double l_startY = g_m_stations[lastLine->startStation]->y;
    double l_endX = g_m_stations[lastLine->endStation]->x;
    double l_endY = g_m_stations[lastLine->endStation]->y;
    double n_startX = g_m_stations[nextLine->startStation]->x;
    double n_startY = g_m_stations[nextLine->startStation]->y;
    double n_endX = g_m_stations[nextLine->endStation]->x;
    double n_endY = g_m_stations[nextLine->endStation]->y;

    if(lastLine->line){
        lastAngle = atan2(l_endY - l_startY,l_endX-l_startX);
    }else{
        lastAngle = atan2(l_endY - lastLine->p2y,l_endX - lastLine->p2x);
    }

    if(nextLine->line){
        nextAngle = atan2(n_endY - n_startY,n_endX-n_startX);
    }else{
        nextAngle = atan2(nextLine->p1y - n_startY,nextLine->p1x - n_startX);
    }

    double changeAngle = nextAngle-lastAngle;

    while(changeAngle>M_PI){
        changeAngle-=2*M_PI;
    }
    while(changeAngle<-1*M_PI){
        changeAngle+=2*M_PI;
    }

    //夹角小于20° 认为直线行驶
    if(abs(changeAngle)<=20*M_PI/180){
        //角度基本一致
        //如果只有一条线路，那就不算左中右，为了将来的东西防止出问题。如果不只一条线路，并且下一条线路是弧线，则endAngle要重新计算
        if(!nextLine->line)
        {
            //这种情况比较多见，怎么优化呢？？回头再说吧
            nextAngle = atan2(n_endY - n_startY,n_endX-n_startX);
            double changeAngle = nextAngle-lastAngle;

            while(changeAngle>M_PI){
                changeAngle-=2*M_PI;
            }
            while(changeAngle<-1*M_PI){
                changeAngle+=2*M_PI;
            }
            if(abs(changeAngle)<=20*M_PI/180){
                return PATH_LMR_MIDDLE;
            }else if(changeAngle>0){
                return PATH_LMR_RIGHT;
            }else{
                return PATH_LMR_LEFT;
            }
        }
        return PATH_LMR_MIDDLE;
    }

    //夹角大于80°，认为拐的弧度过大，不能过去
    if(abs(changeAngle)>=100*M_PI/180){
        //拐角特别大！
        return PATH_LMF_NOWAY;
    }

    if(changeAngle>0){
        return PATH_LMR_RIGHT;
    }else{
        return PATH_LMR_LEFT;
    }

}

void MapCenter::create()
{
    //1. 计算线路的长度
    //已经计算过了

    //2. 构建反向线 有A--B的线路，然后将B--A的线路推算出来
    //获取现有线路的最大的ID值
    int maxId = 0;
    for(QMap<int,AgvLine *>::iterator itr =  g_m_lines.begin();itr!=g_m_lines.end();++itr)
    {
        AgvLine *line = itr.value();
        if(line->id>maxId)maxId = line->id;
    }

    QMap<int,AgvLine *> reverseLines;
    for(QMap<int,AgvLine *>::iterator itr =  g_m_lines.begin();itr!=g_m_lines.end();++itr)
    {
        AgvLine *line = itr.value();
        //构造一个反向的line。和原来的line形成有向的两个线。A-->B 。这里就构建一个B-->A
        AgvLine *rLine = new AgvLine;
        rLine->id = ++maxId;
        rLine->length = (line->length);
        rLine->line = (line->line);
        rLine->rate = (line->rate);
        rLine->color_r = line->color_r;
        rLine->color_g = line->color_g;
        rLine->color_b = line->color_b;
        //起止点相反
        rLine->startStation=(line->endStation);
        rLine->endStation=(line->startStation);
        rLine->draw = (false);

        if(!line->line){
            //弧线
            rLine->p1x = line->p2x;
            rLine->p1y = line->p2y;
            rLine->p2x = line->p1x;
            rLine->p2y = line->p1y;

            QString insertSql = "INSERT INTO agv_line (id,line_startStation,line_endStation,line_line,line_length,line_draw,line_rate,line_p1x,line_p1y,line_p2x,line_p2y,line_color_r,line_color_g,line_color_b) VALUES (?,?,?,?,?,?,?,?,?,?,?,?,?,?)";
            QList<QVariant> params;
            params<<rLine->id
                 <<rLine->startStation
                <<rLine->endStation
               <<rLine->line
              <<rLine->length
             <<rLine->draw
            <<rLine->rate
            <<rLine->p1x
            <<rLine->p1y
            <<rLine->p2x
            <<rLine->p2y
            <<rLine->color_r
            <<rLine->color_g
            <<rLine->color_b;

            if(g_sql->exeSql(insertSql,params))
            {
                reverseLines.insert(rLine->id,rLine);
            }else{
                g_log->log(AGV_LOG_LEVEL_ERROR,"save agv line to database fail!");
                delete rLine;
                continue;
            }
        }else{
            QString insertSql = "INSERT INTO agv_line (id,line_startStation,line_endStation,line_line,line_length,line_draw,line_rate,line_color_r,line_color_g,line_color_b) VALUES (?,?,?,?,?,?,?,?,?,?)";
            QList<QVariant> params;
            params<<rLine->id
                 <<rLine->startStation
                <<rLine->endStation
               <<rLine->line
              <<rLine->length
             <<rLine->draw
            <<rLine->rate
            <<rLine->color_r
            <<rLine->color_g
            <<rLine->color_b;

            if(g_sql->exeSql(insertSql,params))
            {
                reverseLines.insert(rLine->id,rLine);
            }else{
                g_log->log(AGV_LOG_LEVEL_ERROR,"save agv line to database fail!");
                delete rLine;
                continue;
            }
        }

        g_reverseLines[line->id] = rLine->id;
        g_reverseLines[rLine->id] = line->id;
    }

    for(QMap<int,AgvLine *>::iterator itr = reverseLines.begin();itr!=reverseLines.end();++itr){
        g_m_lines.insert(itr.key(),itr.value());
    }


    //4.构建左中右信息 上一线路的key，下一下路的key，然后是 LMRN  L:left,M:middle,R:right,N:noway;就是不通的意思
    //对每个站点的所有连线进行匹配
    for(QMap<int,AgvLine *>::iterator itr =  g_m_lines.begin();itr!=g_m_lines.end();++itr){
        AgvLine *a = itr.value();
        for(QMap<int,AgvLine *>::iterator pos =  g_m_lines.begin();pos!=g_m_lines.end();++pos){
            AgvLine *b = pos.value();
            if(a == b)continue;
            //a-->station -->b （a线路的终点是b线路的起点。那么计算一下这三个点的左中右信息）
            if(a->endStation == b->startStation && a->startStation!=b->endStation){
                PATH_LEFT_MIDDLE_RIGHT p;
                p.lastLine = a->id;
                p.nextLine = b->id;
                if(g_m_lmr.keys().contains(p))continue;
                g_m_lmr[p]=getLMR(a,b);
                //保存到数据库
                QString insertSql = "insert into agv_lmr(lmr_lastLine,lmr_nextLine,lmr_lmr) values(?,?,?);";
                QList<QVariant> params;
                params<<p.lastLine<<p.nextLine<<g_m_lmr[p];
                g_sql->exeSql(insertSql,params);
            }
        }
    }

    //////////////////......................有向图构建完成....................
    //5 构建adj
    for(QMap<int,AgvLine *>::iterator itr =  g_m_lines.begin();itr!=g_m_lines.end();++itr){
        AgvLine *a = itr.value();
        for(QMap<int,AgvLine *>::iterator pos =  g_m_lines.begin();pos!=g_m_lines.end();++pos){
            AgvLine *b = pos.value();
            if(a == b)continue;

            PATH_LEFT_MIDDLE_RIGHT p;
            p.lastLine = a->id;
            p.nextLine = b->id;

            if(a->endStation == b->startStation && a->startStation != b->endStation){
                if(g_m_l_adj.keys().contains(a->id)){
                    if(g_m_l_adj[a->id].contains(b))continue;

                    if(g_m_lmr.keys().contains(p) && g_m_lmr[p] != PATH_LMF_NOWAY){
                        g_m_l_adj[a->id].append(b);
                    }
                }else{
                    //插入
                    if(g_m_lmr[p] != PATH_LMF_NOWAY){
                        QList<AgvLine*> v;
                        v.append(b);
                        g_m_l_adj[a->id] = v;
                    }
                }
            }
        }
    }
    //将adj保存到数据库
    for(QMap<int,QList<AgvLine*> >::iterator itr = g_m_l_adj.begin();itr!=g_m_l_adj.end();++itr){
        QList<AgvLine*> lines = itr.value();
        QString insertSql = "insert into agv_adj (adj_startLine,adj_endLine) values(?,?)";
        QList<QVariant> params;
        for(QList<AgvLine *>::iterator pos = lines.begin();pos!=lines.end();++pos)
        {
            AgvLine *l = *pos;
            params.clear();
            params<<itr.key()<<l->id;
            g_sql->exeSql(insertSql,params);
        }
    }
}


bool MapCenter::resetMap(QString stationStr, QString lineStr, QString arcStr, QString imagestr)//站点、直线、弧线
{
    mutex.lock();
    clear();

    //添加站点
    addStation(stationStr);

    //添加直线
    addLine(lineStr);

    //添加弧线
    addArc(arcStr);

    //添加反向线路。设置站点的lineAmount、设置线路的起止站点
    //生成lmr信息存库
    //生成adj信息存库
    create();

    mutex.unlock();

    emit mapUpdate();

    return true;
}

bool MapCenter::load()
{
    mutex.lock();

    qDeleteAll(g_m_lines.values());
    qDeleteAll(g_m_stations.values());

    g_m_stations.clear();
    g_m_lines.clear();
    g_m_lmr.clear();
    g_m_l_adj.clear();
    g_reverseLines.clear();

    /// 算法 线路 QMap<int,AgvLine *> g_m_agvlines;
    /// 算法 站点 QMap<int,AgvStation *> g_m_agvstations
    /// 左中右信息 QMap<PATH_LEFT_MIDDLE_RIGHT,int> g_m_leftRightMiddle;
    /// adj信息 QMap<int,QList<AgvLine *> > g_m_adj;

    //stations
    QString queryStationSql = "select id,station_x,station_y,station_name,station_rfid,station_color_r,station_color_g,station_color_b from agv_station";
    QList<QVariant> params;
    QList<QList<QVariant> > result = g_sql->query(queryStationSql,params);
    for(int i=0;i<result.length();++i){
        QList<QVariant> qsl = result.at(i);
        if(qsl.length()!=8){
            QString ss =  "select error!!!!!!" + queryStationSql;
            g_log->log(AGV_LOG_LEVEL_ERROR,ss);
            mutex.unlock();
            return false;
        }
        AgvStation *station = new AgvStation;
        station->id = (qsl.at(0).toInt());
        station->x = (qsl.at(1).toDouble());
        station->y = (qsl.at(2).toDouble());
        station->name = (qsl.at(3).toString());
        station->rfid = (qsl.at(4).toInt());
        station->color_r = (qsl.at(5).toInt());
        station->color_g = (qsl.at(6).toInt());
        station->color_b = (qsl.at(7).toInt());
        g_m_stations.insert(station->id,station);
    }

    //lines
    QString squeryLineSql = "select id,line_startStation,line_endStation,line_line,line_length,line_draw,line_rate,line_p1x,line_p1y,line_p2x,line_p2y,line_color_r,line_color_g,line_color_b from agv_line";
    result = g_sql->query(squeryLineSql,params);
    for(int i=0;i<result.length();++i){
        QList<QVariant> qsl = result.at(i);
        if(qsl.length()!=14){
            QString ss;
            ss = "select error!!!!!!"+squeryLineSql;
            g_log->log(AGV_LOG_LEVEL_ERROR,ss);
            mutex.unlock();
            return false;
        }
        AgvLine *line = new AgvLine;
        line->id=(qsl.at(0).toInt());
        line->startStation=(qsl.at(1).toInt());
        line->endStation=(qsl.at(2).toInt());
        line->line = (qsl.at(3).toBool());
        line->length=(qsl.at(4).toInt());
        line->draw=(qsl.at(5).toInt());
        line->rate=(qsl.at(6).toDouble());
        line->p1x=(qsl.at(7).toDouble());
        line->p1y=(qsl.at(8).toDouble());
        line->p2x=(qsl.at(9).toDouble());
        line->p2y=(qsl.at(10).toDouble());
        line->color_r=(qsl.at(11).toInt());
        line->color_g=(qsl.at(12).toInt());
        line->color_b=(qsl.at(13).toInt());

        g_m_lines.insert(line->id,line);
    }
    //反方向线路
    for(QMap<int,AgvLine *>::iterator itr = g_m_lines.begin();itr!=g_m_lines.end();++itr){
        for(QMap<int,AgvLine *>::iterator pos = g_m_lines.begin();pos!=g_m_lines.end();++pos){
            if(itr.key()!=pos.key()
                    &&itr.value()->startStation == pos.value()->endStation
                    &&itr.value()->endStation == pos.value()->startStation ){
                g_reverseLines[itr.key()] = pos.key();
                g_reverseLines[pos.key()] = itr.key();
            }
        }
    }

    //lmr
    QString queryLmrSql = "select lmr_lastLine,lmr_nextLine,lmr_lmr from agv_lmr";
    result = g_sql->query(queryLmrSql,params);
    for(int i=0;i<result.length();++i){
        QList<QVariant> qsl = result.at(i);
        if(qsl.length()!=3){
            QString ss =  "select error!!!!!!"+queryLmrSql;
            g_log->log(AGV_LOG_LEVEL_ERROR,ss);
            mutex.unlock();
            return false;
        }
        PATH_LEFT_MIDDLE_RIGHT ll;
        ll.lastLine = qsl.at(0).toInt();
        ll.nextLine = qsl.at(1).toInt();
        g_m_lmr.insert(ll,qsl.at(2).toInt());
    }

    //adj
    QString queryAdjSql = "select adj_startLine,adj_endLine from agv_adj";
    result = g_sql->query(queryAdjSql,params);
    for(int i=0;i<result.length();++i)
    {
        QList<QVariant> qsl = result.at(i);
        if(qsl.length()!=2){
            QString ss = "select error!!!!!!"+queryAdjSql;
            g_log->log(AGV_LOG_LEVEL_ERROR,ss);
            mutex.unlock();
            return false;
        }
        AgvLine* endLine = g_m_lines[qsl.at(1).toInt()];
        int startLine = qsl.at(0).toInt();
        if(g_m_l_adj.contains(startLine)){
            g_m_l_adj[startLine].push_back(endLine);
        }else{
            QList<AgvLine*> lines;
            lines.push_back(endLine);
            g_m_l_adj[startLine] = lines;
        }
    }
    mutex.unlock();
    return true;
}

//设置lineid的反向线路的占用agv
void MapCenter::setReverseOccuAgv(int lineid, int occagv)
{
    mutex.lock();
    int reverseLineKey = g_reverseLines[lineid];
    //将这条线路的可用性置为false
    g_m_lines[reverseLineKey]->occuAgv=occagv;
    mutex.unlock();
}

QList<int> MapCenter::getBestPath(int agvId, int lastStation, int startStation, int endStation, int &distance, bool canChangeDirect)//最后一个参数是是否可以换个方向
{
    distance = distance_infinity;
    int disA=distance_infinity;
    int disB=distance_infinity;
    //先找到小车不掉头的线路
    QList<int> a = getPath(agvId,lastStation,startStation,endStation,disA,false);
    QList<int> b;
    if(canChangeDirect){//如果可以掉向，那么计算一下掉向的
        b = getPath(agvId,startStation,lastStation,endStation,disB,true);
        if(disA!=distance_infinity  && disB!=distance_infinity){
            distance = disA<disB?disA:disB;
            if(disA<disB)return a;
            return b;
        }
    }
    if(disA!=distance_infinity){
        distance = disA;
        return a;
    }
    distance = disB;
    return b;
}

QList<int> MapCenter::getPath(int agvId,int lastPoint,int startPoint,int endPoint,int &distance,bool changeDirect)
{
    distance = distance_infinity;
    mutex.lock();
    QList<int> result;
    //异常检查
    //如果上一站是未知的，例如第一次开机！
    if(lastPoint == 0){
        lastPoint = startPoint;
    }
    if(!g_m_stations.keys().contains(lastPoint)){
        mutex.unlock();
        return result;
    }
    if(!g_m_stations.keys().contains(startPoint)){
        mutex.unlock();
        return result;
    }
    if(!g_m_stations.keys().contains(endPoint)){
        mutex.unlock();
        return result;
    }

    if(startPoint==endPoint){
        //如果反向了
        if(changeDirect && lastPoint!=startPoint)
        {
            if(g_m_stations[endPoint]->occuAgv!=0 && g_m_stations[endPoint]->occuAgv!=agvId)
            {
                mutex.unlock();
                return result;
            }
            //那么返回一个结果
            for(QMap<int,AgvLine *>::iterator itr =  g_m_lines.begin();itr!=g_m_lines.end();++itr){
                //所有线路为白色，距离为未知
                AgvLine *line =itr.value();
                if(line->startStation == lastPoint  && line->endStation == startPoint ){
                    result.push_back(line->id);
                    distance = line->length;
                }
            }
        }else{
            distance = 0;
        }
        mutex.unlock();
        return result;
    }

    QMultiMap<int,int> Q;//按照distance和stationKey的方式自动排序。distance可以重复,所以用MultiMap!!!

    //初始化 距离和父节点、还有颜色
    for(QMap<int,AgvLine *>::iterator itr =  g_m_lines.begin();itr!=g_m_lines.end();++itr){
        //所有线路为白色，距离为未知
        AgvLine *t =itr.value();
        t->father = -1;
        t->distance = distance_infinity;
        t->color = AGV_LINE_COLOR_WHITE;
    }

    //对于起始线路开始着色并标记距离
    //两种情况，
    //AgvStation *lastStation = g_m_stations[lastPoint];
    if(lastPoint == startPoint){
        //如果lastPoint和startPoint相同，那么说明每个方向都可以
        for(QMap<int,AgvLine *>::iterator itr =  g_m_lines.begin();itr!=g_m_lines.end();++itr){
            //所有线路为白色，距离为未知
            AgvLine *line =itr.value();
            if(line->startStation == startPoint && (line->occuAgv==0 || line->occuAgv == agvId)){//以改点未起点，并且未被占用(或者被当前车辆占用的)
                line->distance = line->length;
                line->color= AGV_LINE_COLOR_GRAY;
                Q.insert(line->length,line->id);
            }
        }
    }else{
        //找到那个对应的线路
        for(QMap<int,AgvLine *>::iterator itr =  g_m_lines.begin();itr!=g_m_lines.end();++itr){
            //所有线路为白色，距离为未知
            AgvLine *line =itr.value();
            if(line->occuAgv!=0 && line->occuAgv!=agvId)continue;//逆向占用了（而且是被不是当前车辆的车占用了）
            if(g_m_stations[line->endStation]->occuAgv!=0 && g_m_stations[line->endStation]->occuAgv!=agvId)continue;//这条线路的终点被占用了
            if(line->endStation == startPoint){
                line->distance = 0;
                line->color = AGV_LINE_COLOR_GRAY;
                Q.insert(line->length,line->id);
            }
        }
    }

    while(Q.size()>0)
    {
        QMultiMap<int,int>::iterator front;
        front = Q.begin();
        int tLine = front.value();
        //这条线的下一条线路
        for(int i=0;i<g_m_l_adj[tLine].length();++i){
            AgvLine *l = g_m_l_adj[tLine][i];
            if(l->occuAgv!=0 && l->occuAgv!=agvId)continue;//反向被占用，不考虑他了
            if(g_m_stations[l->endStation]->occuAgv!=0 && g_m_stations[l->endStation]->occuAgv!=agvId)continue;//这条线路的终点被占用了
            if(l->color == AGV_LINE_COLOR_BLACK)
            {
                continue;
            }else if(l->color == AGV_LINE_COLOR_WHITE){
                //白色直接赋值，并加入Q中
                l->distance = g_m_lines[tLine]->distance + l->length;
                Q.insert(l->distance,l->id);
                l->color = AGV_LINE_COLOR_GRAY;
                l->father = tLine;
            }else if(l->color == AGV_LINE_COLOR_GRAY){
                if(l->distance > g_m_lines[tLine]->distance + l->length)
                {
                    l->distance = g_m_lines[tLine]->distance + l->length;
                    l->father = tLine;
                    //更新Q中的节点的distance
                    //这里要执行一次删除和插入的操作

                    //删除
                    for(QMultiMap<int,int>::iterator iitr = Q.begin();iitr!=Q.end();++iitr){
                        if(iitr.value() == l->id){
                            Q.erase(iitr);
                            break;
                        }
                    }
                    //插入
                    Q.insert(l->distance,l->id);
                }
            }
        }
        //子节点都赋完值了，那他就黑了
        g_m_lines[tLine]->color = AGV_LINE_COLOR_BLACK;
        //他黑了就要把它请出Q的队列
        for(front = Q.begin();front!=Q.end();++front){
            if(front.value() == tLine){
                Q.erase(front);
                break;
            }
        }
    }

    //    //最后对结果进行输出
    //    ///到这里就算出了最小距离
    int index = -1;
    int minDis = distance_infinity;
    for(QMap<int,AgvLine *>::iterator itr =  g_m_lines.begin();itr!=g_m_lines.end();++itr){
        AgvLine *t = itr.value();
        if(t->endStation == endPoint){
            if(t->distance<minDis){
                minDis = t->distance;
                index = t->id;
            }
        }
    }
    distance = minDis;
    //找到了最后的线路，往前推之前的线路
    while(true){
        if(index == -1){
            break;
        }
        result.push_front(index);
        index = g_m_lines[index]->father;
    }
    //去除第一条线路(因为已经到达了)
    if(result.length()>0 && lastPoint!=startPoint){
        if(!changeDirect){
            if(g_m_lines[result.at(0)]->startStation==lastPoint && g_m_lines[result.at(0)]->endStation==startPoint){
                result.erase(result.begin());
            }
        }else{
            //找到对赢得线路，放进去。理论上是不用的
            //            if(getLine(result.at(0))->startStation() != lastPoint || getLine(result.at(0))->endStation()!=startPoint){
            //                //找到对赢得线路，放进去。理论上是不用的
            //                result.push_front(index);
            //            }
        }
    }
    mutex.unlock();
    return result;
}

