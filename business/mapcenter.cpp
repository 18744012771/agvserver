#include "mapcenter.h"

#define _USE_MATH_DEFINES
#include <math.h>
#include "util/global.h"

#include "bezierarc.h"

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
    QStringList params;

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
    if(s.endsWith(";"))s=s.left(s.length()-1);
    QStringList qsl = s.trimmed().split(";");
    if(qsl.length()>=2){
        QString amoutStr = qsl.at(0);
        bool ok = false;
        int amount = amoutStr.toInt(&ok);
        if(!ok)return ;
        if(amount == qsl.length()-1){
            for(int i=1;i<qsl.length();++i){
                QString ss = qsl.at(i);
                QStringList pp = ss.split(",");
                if(pp.length()>=6){
                    AgvStation *aStation = new AgvStation;
                    aStation->type = ((pp.at(0).toInt()));
                    aStation->lineAmount = ((pp.at(1).toInt()));
                    aStation->x = (pp.at(2).toInt());
                    aStation->y = (pp.at(3).toInt());
                    aStation->name = (pp.at(4));
                    aStation->rfid = (pp.at(5).toInt());

                    QString insertSql = "INSERT INTO agv_station (station_x,station_y,station_type,station_name,station_lineAmount,station_rfid) VALUES (?,?,?,?,?,?);SELECT @@Identity;";
                    QStringList params;
                    params<<QString("%1").arg(aStation->x)<<QString("%1").arg(aStation->y)<<QString("%1").arg(aStation->type)<<aStation->name<<QString("%1").arg(aStation->lineAmount)<<QString("%1").arg(aStation->rfid);
                    QList<QStringList> insertResult = g_sql->query(insertSql,params);
                    if(insertResult.length()>0&&insertResult.at(0).length()>0)
                    {
                        aStation->id=(insertResult.at(0).at(0).toInt());
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
    //ss << "g_m_agvstations.length="<<g_m_stations.keys().length();
    g_log->log(AGV_LOG_LEVEL_DEBUG,ss);

}

void MapCenter::addLine(QString s)
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
                if(pp.length()==4){
                    AgvLine *aLine = new AgvLine;

                    aLine->startX = (pp.at(0).toInt());
                    aLine->startY = (pp.at(1).toInt());
                    aLine->endX = (pp.at(2).toInt());
                    aLine->endY = (pp.at(3).toInt());
                    aLine->line = (true);
                    aLine->draw = (true);
                    int length = sqrt((aLine->startX-aLine->endX)*(aLine->startX-aLine->endX)+(aLine->startY-aLine->endY)*(aLine->startY-aLine->endY));
                    aLine->length = (length);

                    QString insertSql = "INSERT INTO agv_line (line_startX,line_startY,line_endX,line_endY,line_line,line_length,line_draw) VALUES (?,?,?,?,?,?,?);SELECT @@Identity;";
                    QStringList params;
                    params<<QString("%1").arg(aLine->startX)<<QString("%1").arg(aLine->startY)
                         <<QString("%1").arg(aLine->endX)<<QString("%1").arg(aLine->endY)
                        <<QString("%1").arg(aLine->line)<<QString("%1").arg(aLine->length)
                       <<QString("%1").arg(aLine->draw);

                    QList<QStringList> insertResult = g_sql->query(insertSql,params);

                    if(insertResult.length()>0&&insertResult.at(0).length()>0)
                    {
                        aLine->id = (insertResult.at(0).at(0).toInt());
                        g_m_lines.insert(aLine->id,aLine);
                    }else{
                        g_log->log(AGV_LOG_LEVEL_ERROR,"save agv line to database fail!");
                        delete aLine;
                    }

                    //                    aLine->setId(g_m_lines.keys().size()+1);
                    //                    g_m_lines.insert(aLine->id(),aLine);
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
                    aLine->startX=((pp.at(6).toInt()));
                    aLine->startY=((pp.at(7).toInt()));
                    aLine->endX=((pp.at(8).toInt()));
                    aLine->endY=((pp.at(9).toInt()));
                    aLine->line=(false);
                    aLine->draw=((true));

                    aLine->p1x = pp.at(1).toInt();
                    aLine->p1y = pp.at(2).toInt();
                    aLine->p2x = pp.at(3).toInt();
                    aLine->p2y = pp.at(4).toInt();

                    aLine->length = BezierArc::BezierArcLength(QPoint(aLine->startX,aLine->startY),QPoint(aLine->p1x,aLine->p1y),QPoint(aLine->p2x,aLine->p2y),QPoint(aLine->endX,aLine->endY));

                    QString insertSql = "INSERT INTO agv__line (line_startX,line_startY,line_endX,line_endY,line_line,line_length,line_draw,line_p1x,line_p1y,line_p2x,line_p2y) VALUES (?,?,?,?,?,?,?,?,?,?,?);SELECT @@Identity;";
                    QStringList params;
                    params<<QString("%1").arg(aLine->startX)<<QString("%1").arg(aLine->startY)
                         <<QString("%1").arg(aLine->endX)<<QString("%1").arg(aLine->endY)
                        <<QString("%1").arg(aLine->line)<<QString("%1").arg(aLine->length)
                       <<QString("%1").arg(aLine->draw)
                      <<QString("%1").arg(aLine->p1x)<<QString("%1").arg(aLine->p1y)
                     <<QString("%1").arg(aLine->p2x)<<QString("%1").arg(aLine->p2y);

                    QList<QStringList> insertResult = g_sql->query(insertSql,params);

                    if(insertResult.length()>0&&insertResult.at(0).length()>0)
                    {
                        aLine->id = (insertResult.at(0).at(0).toInt());
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

    if(lastLine->line){
        lastAngle = atan2(lastLine->endY - lastLine->startY,lastLine->endX-lastLine->startX);
    }else{
        lastAngle = atan2(lastLine->endY - lastLine->p2y,lastLine->endX - lastLine->p2x);
    }

    if(nextLine->line){
        nextAngle = atan2(nextLine->endY - nextLine->startY,nextLine->endX-nextLine->startX);
    }else{
        nextAngle = atan2(nextLine->p1y - nextLine->startY,nextLine->p1x - nextLine->startX);
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
            nextAngle = atan2(nextLine->endY - nextLine->startY,nextLine->endX-nextLine->startX);
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
    mutex.lock();
    //1. 计算线路的长度
    //已经计算过了

    //2. 构建反向线 有A--B的线路，然后将B--A的线路推算出来
    QMap<int,AgvLine *> reverseLines;
    for(QMap<int,AgvLine *>::iterator itr =  g_m_lines.begin();itr!=g_m_lines.end();++itr)
    {
        AgvLine *line = itr.value();
        //构造一个反向的line。和原来的line形成有向的两个线。A-->B 。这里就构建一个B-->A
        AgvLine *rLine = new AgvLine;

        rLine->length = (line->length);
        rLine->line = (line->line);
        //起止点相反
        rLine->startX=(line->endX);
        rLine->startY=(line->endY);
        rLine->endX=(line->startX);
        rLine->endY = (line->startY);
        rLine->draw = (false);

        if(!line->line){
            //弧线
            rLine->p1x = line->p2x;
            rLine->p1y = line->p2y;
            rLine->p2x = line->p1x;
            rLine->p2y = line->p1y;


            QString insertSql = "INSERT INTO agv__line (line_startX,line_startY,line_endX,line_endY,line_line,line_length,line_draw,line_p1x,line_p1y,line_p2x,line_p2y) VALUES (?,?,?,?,?,?,?,?,?,?,?);SELECT @@Identity;";
            QStringList params;
            params<<QString("%1").arg(rLine->startX)<<QString("%1").arg(rLine->startY)
                 <<QString("%1").arg(rLine->endX)<<QString("%1").arg(rLine->endY)
                <<QString("%1").arg(rLine->line)<<QString("%1").arg(rLine->length)
               <<QString("%1").arg(rLine->draw)
              <<QString("%1").arg(rLine->p1x)<<QString("%1").arg(rLine->p1y)
             <<QString("%1").arg(rLine->p2x)<<QString("%1").arg(rLine->p2y);

            QList<QStringList> insertResult = g_sql->query(insertSql,params);

            if(insertResult.length()>0&&insertResult.at(0).length()>0)
            {
                rLine->id = (insertResult.at(0).at(0).toInt());
                g_m_lines.insert(rLine->id,rLine);
            }else{
                g_log->log(AGV_LOG_LEVEL_ERROR,"save agv line to database fail!");
                delete rLine;
                continue;
            }

            //            QString insertSql = "INSERT INTO agv_line (line_startX,line_startY,line_endX,line_endY,line_line,line_length,line_draw,line_clockwise,line_centerX,line_centerY,line_midX,line_midY,line_radius,line_angle) VALUES (?,?,?,?,?,?,?);SELECT @@Identity;";
            //            QStringList params;
            //            params<<QString("%1").arg(rLine->startX())<<QString("%1").arg(rLine->startY())
            //                 <<QString("%1").arg(rLine->endX())<<QString("%1").arg(rLine->endY())
            //                <<QString("%1").arg(rLine->line())<<QString("%1").arg(rLine->length())
            //               <<QString("%1").arg(rLine->draw())<<QString("%1").arg(rLine->clockwise())
            //              <<QString("%1").arg(rLine->centerX())<<QString("%1").arg(rLine->centerY())
            //             <<QString("%1").arg(rLine->midX())<<QString("%1").arg(rLine->midY())
            //            <<QString("%1").arg(rLine->radius())<<QString("%1").arg(rLine->angle());

            //            QList<QStringList> insertResult = g_sql->query(insertSql,params);

            //            if(insertResult.length()>0&&insertResult.at(0).length()>0)
            //            {
            //                rLine->setId(insertResult.at(0).at(0).toInt());
            //                reverseLines.insert(rLine->id(),rLine);
            //            }else{
            //                g_log->log(AGV_LOG_LEVEL_ERROR,"save agv line to database fail!");
            //                delete rLine;
            //                continue;
            //            }
        }else{
            QString insertSql = "INSERT INTO agv_line (line_startX,line_startY,line_endX,line_endY,line_line,line_length,line_draw) VALUES (?,?,?,?,?,?,?);SELECT @@Identity;";
            QStringList params;
            params<<QString("%1").arg(rLine->startX)<<QString("%1").arg(rLine->startY)
                 <<QString("%1").arg(rLine->endX)<<QString("%1").arg(rLine->endY)
                <<QString("%1").arg(rLine->line)<<QString("%1").arg(rLine->length)
               <<QString("%1").arg(rLine->draw);

            QList<QStringList> insertResult = g_sql->query(insertSql,params);

            if(insertResult.length()>0&&insertResult.at(0).length()>0)
            {
                rLine->id = (insertResult.at(0).at(0).toInt());
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

    //3. 对线路的起点终点进行赋值 对站点的经过线路进行赋值
    for(QMap<int,AgvStation *>::iterator pos = g_m_stations.begin();pos!=g_m_stations.end();++pos){
        AgvStation *station = pos.value();
        station->lineAmount = (0);
        for(QMap<int,AgvLine *>::iterator itr =  g_m_lines.begin();itr!=g_m_lines.end();++itr){
            AgvLine *line = itr.value();

            //更新线路的起止站点到数据库
            if(line->startX == station->x && line->startY == station->y)
            {
                QString updateSql = "update agv_line set line_startStation = ? where id=? ;";
                QStringList params;
                params<<QString("%1").arg( station->id)<<QString("%1").arg( line->id);
                if(!g_sql->exeSql(updateSql,params)){
                    g_log->log(AGV_LOG_LEVEL_ERROR,"update agv_line set start startion fail!");
                    continue;
                }
                line->startStation = (station->id);
                station->lineAmount++;
            }else if(line->endX == station->x && line->endY == station->y){
                QString updateSql = "update agv_line set line_endStation = ? where id=? ;";
                QStringList params;
                params<<QString("%1").arg(station->id)<<QString("%1").arg(line->id);
                if(!g_sql->exeSql(updateSql,params)){
                    g_log->log(AGV_LOG_LEVEL_ERROR,"update agv_line set end startion fail!");
                    continue;
                }
                line->endStation = (station->id);
            }
        }

        //更新站点的lineamount到数据库
        QString updateSql = "update agv_station set station_lineAmount = ? where id=? ;";
        QStringList params;
        params<<QString("%1").arg(station->lineAmount)<<QString("%1").arg( station->id);
        g_sql->exeSql(updateSql,params);
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
                QStringList params;
                params<<QString("%1").arg(p.lastLine)<<QString("%1").arg(p.nextLine)<<QString("%1").arg(g_m_lmr[p]);
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
        QStringList params;
        for(QList<AgvLine *>::iterator pos = lines.begin();pos!=lines.end();++pos)
        {
            AgvLine *l = *pos;
            params.clear();
            params<<QString("%1").arg( itr.key())<<QString("%1").arg(l->id);
            g_sql->exeSql(insertSql,params);
        }
    }

    //    ////2.保存地图信息，包括如下内容：
    //    if(!save()){
    //        g_log->log(AGV_LOG_LEVEL_ERROR,"地图保存失败");
    //    }
    //TODO
    emit mapUpdate();

    mutex.lock();
}


bool MapCenter::resetMap(QString stationStr,QString lineStr,QString arcStr)//站点、直线、弧线
{
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

    return true;
}

bool MapCenter::load()
{
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
    QString queryStationSql = "select id,station_x,station_y,station_type,station_name,station_lineAmount,station_rfid from agv_station";
    QStringList params;
    QList<QStringList> result = g_sql->query(queryStationSql,params);
    for(int i=0;i<result.length();++i){
        QStringList qsl = result.at(i);
        if(qsl.length()!=7){
            QString ss =  "select error!!!!!!" + queryStationSql;
            g_log->log(AGV_LOG_LEVEL_ERROR,ss);
            return false;
        }
        AgvStation *station = new AgvStation;
        station->id = (qsl.at(0).toInt());
        station->x = (qsl.at(1).toInt());
        station->y = (qsl.at(2).toInt());
        station->type = (qsl.at(3).toInt());
        station->name = (qsl.at(4));
        station->lineAmount = (qsl.at(5).toInt());
        station->rfid = (qsl.at(6).toInt());
        g_m_stations.insert(station->id,station);
    }

    //lines
    QString squeryLineSql = "select id,line_startX,line_startY,line_endX,line_endY,line_line,line_length,line_startStation,line_endStation,line_draw,line_rate,line_p1x,line_p1y,line_p2x,line_p2y from agv__line";
    result = g_sql->query(squeryLineSql,params);
    for(int i=0;i<result.length();++i){
        QStringList qsl = result.at(i);
        if(qsl.length()!=15){
            QString ss;
            ss = "select error!!!!!!"+squeryLineSql;
            g_log->log(AGV_LOG_LEVEL_ERROR,ss);
            return false;
        }
        AgvLine *line = new AgvLine;
        line->id=(qsl.at(0).toInt());
        line->startX=(qsl.at(1).toInt());
        line->startY=(qsl.at(2).toInt());
        line->endX=(qsl.at(3).toInt());
        line->endY=(qsl.at(4).toInt());
        line->line = (qsl.at(5)!="0");
        line->length=(qsl.at(6).toInt());
        line->startStation=(qsl.at(7).toInt());
        line->endStation=(qsl.at(8).toInt());
        line->draw=(qsl.at(9)!="0");
        line->rate=(qsl.at(10).toDouble());
        line->p1x=(qsl.at(11).toInt());
        line->p1y=(qsl.at(12).toInt());
        line->p2x=(qsl.at(13).toInt());
        line->p2y=(qsl.at(14).toInt());

        //        ///在这里对station的ID做一个修正。
        //        for(QMap<int,AgvStation *>::iterator itr = g_m_stations.begin();itr!=g_m_stations.end();++itr){
        //            if(line->startX == itr.value()->x && line->startY == itr.value()->y){
        //                line->startStation = itr.key();
        //            }
        //            if(line->endX == itr.value()->x && line->endY == itr.value()->y){
        //                line->endStation = itr.key();
        //            }
        //        }
        //        //执行更新语句
        //        QString updateStartEndStation = "update agv__line set line_startStation = ?,line_endStation=? where id=?";
        //        QStringList startEndStationParams;
        //        startEndStationParams<<QString("%1").arg(line->startStation);
        //        startEndStationParams<<QString("%1").arg(line->endStation);
        //        startEndStationParams<<QString("%1").arg(line->id);
        //        if(!g_sql->exeSql(updateStartEndStation,startEndStationParams)){
        //            qDebug() << "QYH update start end station fail! id = "<<line->id;
        //        }

        //        {
        //            //////////////////temp将原来的弧线，转换成新的弧线表
        //            ///
        //            ///
        //            if(line->line()){
        //                //插入到新的line表中
        //                QStringList params;
        //                QString insertSql = "insert into agv__line (line_startX,line_startY,line_endX,line_endY,line_length,line_startStation,line_endStation,line_draw,line_rate,line_line)values(?,?,?,?,?,?,?,?,?,?);";
        //                params<<QString("%1").arg(line->startX())
        //                     <<QString("%1").arg(line->startY())
        //                    <<QString("%1").arg(line->endX())
        //                   <<QString("%1").arg(line->endY())
        //                  <<QString("%1").arg(line->length())
        //                 <<QString("%1").arg(line->startStation())
        //                <<QString("%1").arg(line->endStation())
        //                <<QString("%1").arg(line->draw())
        //                <<QString("%1").arg(line->rate())
        //                <<QString("%1").arg(line->line());
        //                bool bb = g_sql->exeSql(insertSql,params);
        //                if(!bb){
        //                    //
        //                    qDebug() <<"line - FAIL!!!!!!!!!!!!!!!!!! insert agv__line!";
        //                }else{
        //                    qDebug() <<"line - FAIL!!!!!!!!!!!!!!!!!! insert agv__line!";
        //                }
        //            }else{
        //                //插入到新的line表中(这是一个arc)
        //                QStringList params;
        //                QString insertSql = "insert into agv__line (line_startX,line_startY,line_endX,line_endY,line_length,line_startStation,line_endStation,line_draw,line_rate,line_line,line_p1x,line_p1y,line_p2x,line_p2y)values(?,?,?,?,?,?,?,?,?,?,?,?,?,?);";
        //                params<<QString("%1").arg(line->startX())
        //                     <<QString("%1").arg(line->startY())
        //                    <<QString("%1").arg(line->endX())
        //                   <<QString("%1").arg(line->endY())
        //                  <<QString("%1").arg(line->length())
        //                 <<QString("%1").arg(line->startStation())
        //                <<QString("%1").arg(line->endStation())
        //                <<QString("%1").arg(line->draw())
        //                <<QString("%1").arg(line->rate())
        //                <<QString("%1").arg(line->line());

        //                ////////////////////////////这里是接下来的弧线部分
        //                int p1x = (line->startX()+line->midX())/2;
        //                int p1y = (line->startY()+line->midY())/2;
        //                int p2x = (line->endX()+line->midX())/2;
        //                int p2y = (line->endY()+line->midY())/2;

        //                params<<QString("%1").arg(p1x)
        //                     <<QString("%1").arg(p1y)
        //                    <<QString("%1").arg(p2x)
        //                   <<QString("%1").arg(p2y);
        //                bool bb =  g_sql->exeSql(insertSql,params);
        //                if(!bb){
        //                    //
        //                    qDebug() <<"arc - FAIL!!!!!!!!!!!!!!!!!! insert agv__line!";
        //                }else{
        //                    qDebug() <<"arc - FAIL!!!!!!!!!!!!!!!!!! insert agv__line!";
        //                }
        //            }
        //        }
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
        QStringList qsl = result.at(i);
        if(qsl.length()!=3){
            QString ss =  "select error!!!!!!"+queryLmrSql;
            g_log->log(AGV_LOG_LEVEL_ERROR,ss);
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
        QStringList qsl = result.at(i);
        if(qsl.length()!=2){
            QString ss = "select error!!!!!!"+queryAdjSql;
            g_log->log(AGV_LOG_LEVEL_ERROR,ss);
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

//    /////////////////////////////////////////////////////////////////////////
//    ///因为数据是之前的导入进来的，缺少adj和lmr数据。所以这里对这个进行一番补充
//    ///搞定后，请注释掉这段
//    ///---------------------------------------------------------start--------------------------------------------------------
//    //4.构建左中右信息 上一线路的key，下一下路的key，然后是 LMRN  L:left,M:middle,R:right,N:noway;就是不通的意思
//    //对每个站点的所有连线进行匹配
//    qDebug() << "qyh job start!";
//    for(QMap<int,AgvLine *>::iterator itr =  g_m_lines.begin();itr!=g_m_lines.end();++itr){
//        AgvLine *a = itr.value();
//        for(QMap<int,AgvLine *>::iterator pos =  g_m_lines.begin();pos!=g_m_lines.end();++pos){
//            AgvLine *b = pos.value();
//            if(a == b)continue;
//            //a-->station -->b （a线路的终点是b线路的起点。那么计算一下这三个点的左中右信息）
//            if(a->endStation == b->startStation && a->startStation!=b->endStation){
//                PATH_LEFT_MIDDLE_RIGHT p;
//                p.lastLine = a->id;
//                p.nextLine = b->id;
//                if(g_m_lmr.keys().contains(p))continue;
//                g_m_lmr[p]=getLMR(a,b);
//                //保存到数据库
//                QString insertSql = "insert into agv_lmr(lmr_lastLine,lmr_nextLine,lmr_lmr) values(?,?,?);";
//                QStringList params;
//                params<<QString("%1").arg(p.lastLine)<<QString("%1").arg(p.nextLine)<<QString("%1").arg(g_m_lmr[p]);
//                g_sql->exeSql(insertSql,params);
//            }
//        }
//    }
//    //构建adj
//    for(QMap<int,AgvLine *>::iterator itr =  g_m_lines.begin();itr!=g_m_lines.end();++itr){
//        AgvLine *a = itr.value();
//        for(QMap<int,AgvLine *>::iterator pos =  g_m_lines.begin();pos!=g_m_lines.end();++pos){
//            AgvLine *b = pos.value();
//            if(a == b)continue;

//            PATH_LEFT_MIDDLE_RIGHT p;
//            p.lastLine = a->id;
//            p.nextLine = b->id;

//            if(a->endStation == b->startStation && a->startStation != b->endStation){
//                if(g_m_l_adj.keys().contains(a->id)){
//                    if(g_m_l_adj[a->id].contains(b))continue;

//                    if(g_m_lmr.keys().contains(p) && g_m_lmr[p] != PATH_LMF_NOWAY){
//                        g_m_l_adj[a->id].append(b);
//                    }
//                }else{
//                    //插入
//                    if(g_m_lmr[p] != PATH_LMF_NOWAY){
//                        QList<AgvLine*> v;
//                        v.append(b);
//                        g_m_l_adj[a->id] = v;
//                    }
//                }
//            }
//        }
//    }
//    //将adj保存到数据库
//    for(QMap<int,QList<AgvLine*> >::iterator itr = g_m_l_adj.begin();itr!=g_m_l_adj.end();++itr){
//        QList<AgvLine*> lines = itr.value();
//        QString insertSql = "insert into agv_adj (adj_startLine,adj_endLine) values(?,?)";
//        QStringList params;
//        for(QList<AgvLine *>::iterator pos = lines.begin();pos!=lines.end();++pos)
//        {
//            AgvLine *l = *pos;
//            params.clear();
//            params<<QString("%1").arg( itr.key())<<QString("%1").arg(l->id);
//            g_sql->exeSql(insertSql,params);
//        }
//    }
//    qDebug() << "qyh job done!";
//    ///---------------------------------------------------------end--------------------------------------------------------

    return true;
}

//bool MapCenter::save()
//{
//    QString deleteStationSql = "delete from agv_station;";
//    QStringList params;

//    bool b = g_sql->exeSql(deleteStationSql,params);
//    if(!b){
//        g_log->log(AGV_LOG_LEVEL_ERROR,"can not clear table agv_station!");
//        return false;
//    }
//    QString deleteLineSql = "delete from agv_line;";
//    b = g_sql->exeSql(deleteLineSql,params);
//    if(!b){
//        g_log->log(AGV_LOG_LEVEL_ERROR,"can not clear table agv_line!");
//        return false;
//    }
//    QString deleteLmrSql = "delete from agv_lmr;";
//    b = g_sql->exeSql(deleteLmrSql,params);
//    if(!b){
//        g_log->log(AGV_LOG_LEVEL_ERROR,"can not clear table agv_lmr!");
//        return false;
//    }
//    QString deleteAdjSql = "delete from agv_adj;";
//    b = g_sql->exeSql(deleteAdjSql,params);
//    if(!b){
//        g_log->log(AGV_LOG_LEVEL_ERROR,"can not clear table agv_adj!");
//        return false;
//    }
//    //插入数据
//    QString insertStationSql = "insert into agv_station(station_x,station_y,station_type,station_name,station_lineAmount,station_rfid) values(?,?,?,?,?,?,?);";
//    for(QMap<int,AgvStation *>::iterator itr = g_m_stations.begin();itr!=g_m_stations.end();++itr){
//        AgvStation *s = itr.value();
//        params.clear();
//        params<<QString("%1").arg(s->x())<<QString("%1").arg(s->y())<<QString("%1").arg(s->type())<<s->name()<<QString("%1").arg(s->lineAmount())<<QString("%1").arg(s->rfid());
//        if(!g_sql->exeSql(insertStationSql,params))
//        {
//            g_log->log(AGV_LOG_LEVEL_ERROR," insert into agv_station failed!");
//            return false;
//        }
//    }

//    QString insertLineSql = "insert into agv_line(station_startX,startY,endX,endY,radius,clockwise,line,midX,midY,centerX,centerY,angle,length,startStation,endStation,draw) values(?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?);";
//    for(QMap<int,AgvLine *>::iterator itr =  g_m_lines.begin();itr!=g_m_lines.end();++itr){
//        AgvLine *l = itr.value();
//        params.clear();
//        params<<QString("%1").arg(l->id())<<QString("%1").arg(l->startX())<<QString("%1").arg(l->startY())<<QString("%1").arg(l->endX())<<QString("%1").arg(l->endY())<<QString("%1").arg(l->radius())<<QString("%1").arg(l->clockwise())<<QString("%1").arg(l->line())<<QString("%1").arg(l->midX())<<QString("%1").arg(l->midY())<<QString("%1").arg(l->centerX())<<QString("%1").arg(l->centerY())<<QString("%1").arg(l->angle())<<QString("%1").arg(l->length())<<QString("%1").arg(l->startStation())<<QString("%1").arg(l->endStation())<<QString("%1").arg(l->draw());
//        if(!g_sql->exeSql(insertLineSql,params))
//        {
//            g_log->log(AGV_LOG_LEVEL_ERROR," insert into agv_line failed!");
//            return false;
//        }
//    }

//    QString insertLmrSql = "insert into agv_lmr(lastLine,nextLine,lmr) values(?,?,?);";
//    for(QMap<PATH_LEFT_MIDDLE_RIGHT,int>::iterator itr = g_m_lmr.begin();itr!=g_m_lmr.end();++itr){
//        params.clear();
//        params<<QString("%1").arg(itr.key().lastLine)<<QString("%1").arg(itr.key().nextLine)<<QString("%1").arg(itr.value());
//        if(!g_sql->exeSql(insertLmrSql,params)){
//            g_log->log(AGV_LOG_LEVEL_ERROR,"insert into agv_lmr failed!");
//            return false;
//        }
//    }

//    QString insertAdjSql = "insert into agv_adj(myKey,lines) values(?,?);";
//    for(QMap<int,QList<AgvLine*> >::iterator itr = g_m_l_adj.begin();itr!=g_m_l_adj.end();++itr)
//    {
//        params.clear();
//        QString linesStr = "";
//        for(QList<AgvLine *>::iterator pos = itr.value().begin();pos!=itr.value().end();++pos){
//            AgvLine *lt = *pos;
//            linesStr.append(QString("%1,").arg(lt->id()));
//        }
//        if(linesStr.endsWith(","))
//            linesStr = linesStr.left(linesStr.length()-1);
//        params<<QString("%1").arg(itr.key())<<linesStr;
//        if(!g_sql->exeSql(insertAdjSql,params)){
//            g_log->log(AGV_LOG_LEVEL_ERROR,"insert into agv_adj failed!");
//            return false;
//        }
//    }

//    return true;
//}

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

    mutex.unlock();

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



    return result;
}

