#include "mapcenter.h"

#define _USE_MATH_DEFINES
#include <math.h>

#include "util/global.h"

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
                    aStation->setType((pp.at(0).toInt()));
                    aStation->setLineAmount((pp.at(1).toInt()));
                    aStation->setX(pp.at(2).toInt());
                    aStation->setY(pp.at(3).toInt());
                    aStation->setName(pp.at(4));
                    aStation->setRfid(pp.at(5).toInt());
                    aStation->setId(g_m_stations.keys().length()+1);
                    g_m_stations.insert(aStation->id(),aStation);
                }
            }
        }
    }
    qDebug() << "g_m_agvstations.length="<<g_m_stations.keys().length();
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
                    aLine->setLine(true);
                    aLine->setStartX(pp.at(0).toInt());
                    aLine->setStartY(pp.at(1).toInt());
                    aLine->setEndX(pp.at(2).toInt());
                    aLine->setEndY(pp.at(3).toInt());
                    aLine->setDraw(true);
                    int length = sqrt((aLine->startX()-aLine->endX())*(aLine->startX()-aLine->endX())+(aLine->startY()-aLine->endY())*(aLine->startY()-aLine->endY()));
                    aLine->setLength(length);
                    aLine->setId(g_m_lines.keys().size()+1);
                    g_m_lines.insert(aLine->id(),aLine);
                }
            }
        }
    }
    qDebug() << "g_m_agvlines.length="<<g_m_lines.size();
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
                    aLine->setLine(false);
                    aLine->setClockwise((pp.at(0)=="true"));
                    aLine->setCenterX((pp.at(1).toInt()));
                    aLine->setCenterY((pp.at(2).toInt()));
                    aLine->setMidX((pp.at(3).toInt()));
                    aLine->setMidY((pp.at(4).toInt()));
                    aLine->setRadius((pp.at(5).toInt()));
                    aLine->setStartX((pp.at(6).toInt()));
                    aLine->setStartY((pp.at(7).toInt()));
                    aLine->setEndX((pp.at(8).toInt()));
                    aLine->setEndY((pp.at(9).toInt()));
                    aLine->setAngle((pp.at(10).toInt()));
                    aLine->setDraw((true));
                    //设置长度
                    int radius = pp.at(5).toInt();
                    int angle = pp.at(10).toInt();
                    int length = 2*M_PI*radius*angle/360;
                    aLine->setLength(length);
                    aLine->setId(g_m_lines.keys().length()+1);
                    g_m_lines.insert(aLine->id(),aLine);
                    //TODO:
                    //插入反方向的连线
                }
            }
        }
    }
    qDebug() << "g_m_agvlines.length="<<g_m_lines.keys().size();
}

int MapCenter::getLMR(AgvLine *lastLine,AgvLine *nextLine)
{
    if(lastLine->endStation() != nextLine->startStation())return PATH_LMF_NOWAY;

    double lastAngle,nextAngle;

    if(lastLine->line()){
        lastAngle = atan2(lastLine->endY() - lastLine->startY(),lastLine->endX()-lastLine->startX());
    }else{
        lastAngle = atan2(lastLine->endY() - lastLine->midY(),lastLine->endX() - lastLine->midX());
    }

    if(nextLine->line()){
        nextAngle = atan2(nextLine->endY() - nextLine->startY(),nextLine->endX()-nextLine->startX());
    }else{
        nextAngle = atan2(nextLine->midY() - nextLine->startY(),nextLine->midX() - nextLine->startX());
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
        if(!nextLine->line()&& g_m_stations[nextLine->startStation()]->lineAmount()>4)
        {
            //这种情况比较多见，怎么优化呢？？回头再说吧
            nextAngle = atan2(nextLine->endY() - nextLine->startY(),nextLine->endX()-nextLine->startX());
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
    if(abs(changeAngle)>=80*M_PI/180){
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
    int maxKey = 0;
    for(QMap<int,AgvLine *>::iterator itr =  g_m_lines.begin();itr!=g_m_lines.end();++itr){
        AgvLine *line =itr.value();
        if(line->id()>maxKey)maxKey=line->id();
        if(line->line()){
            line->setLength(sqrt((line->startX()-line->endX())*(line->startX()-line->endX())+(line->startY()-line->endY())*(line->startY()-line->endY())));
        }else{
            line->setLength(2*M_PI*line->radius()*line->angle()/360);
        }
    }

    //2. 构建反向线 有A--B的线路，然后将B--A的线路推算出来
    QMap<int,AgvLine *> reverseLines;
    for(QMap<int,AgvLine *>::iterator itr =  g_m_lines.begin();itr!=g_m_lines.end();++itr)
    {
        AgvLine *line = itr.value();
        //构造一个反向的line。和原来的line形成有向的两个线。A-->B 。这里就构建一个B-->A
        AgvLine *rLine = new AgvLine;
        rLine->setId(++maxKey);
        rLine->setLength(line->length());
        rLine->setLine(line->line());
        //起止点相反
        rLine->setStartX(line->endX());
        rLine->setStartY(line->endY());
        rLine->setEndX(line->startX());
        rLine->setEndY(line->startY());
        rLine->setDraw(false);

        if(!line->line()){
            //弧线
            //顺逆时针相反
            rLine->setClockwise(!line->clockwise());
            rLine->setAngle(line->angle());
            rLine->setCenterX(line->centerX());
            rLine->setCenterY(line->centerY());
            rLine->setMidX(line->midX());
            rLine->setMidY(line->midY());
            rLine->setRadius(line->radius());
        }
        g_reverseLines[line->id()] = rLine->id();
        g_reverseLines[rLine->id()] = line->id();
        reverseLines.insert(rLine->id(),rLine);
    }
    for(QMap<int,AgvLine *>::iterator itr = reverseLines.begin();itr!=reverseLines.end();++itr){
        g_m_lines.insert(itr.key(),itr.value());
    }

    //3. 对线路的起点终点进行赋值 对站点的经过线路进行赋值
    for(QMap<int,AgvStation *>::iterator pos = g_m_stations.begin();pos!=g_m_stations.end();++pos){
        AgvStation *station = pos.value();
        station->setLineAmount(0);
        for(QMap<int,AgvLine *>::iterator itr =  g_m_lines.begin();itr!=g_m_lines.end();++itr){
            AgvLine *line = itr.value();
            if(line->startX() == station->x() && line->startY() == station->y())
            {
                line->setStartStation(station->id());
                station->setLineAmount(station->lineAmount()+1);
            }else if(line->endX() == station->x() && line->endY() == station->y()){
                line->setEndStation(station->id());
            }
        }
    }

    //4.构建左中右信息 上一线路的key，下一下路的key，然后是 LMRN  L:left,M:middle,R:right,N:noway;就是不通的意思
    //对每个站点的所有连线进行匹配
    for(QMap<int,AgvLine *>::iterator itr =  g_m_lines.begin();itr!=g_m_lines.end();++itr){
        AgvLine *a = itr.value();
        for(QMap<int,AgvLine *>::iterator pos =  g_m_lines.begin();pos!=g_m_lines.end();++pos){
            AgvLine *b = pos.value();
            if(a == b)continue;
            //a-->station -->b （a线路的终点是b线路的起点。那么计算一下这三个点的左中右信息）
            if(a->endStation() == b->startStation() && a->startStation()!=b->endStation()){
                PATH_LEFT_MIDDLE_RIGHT p;
                p.lastLine = a->id();
                p.nextLine = b->id();
                if(g_m_lmr.keys().contains(p))continue;
                g_m_lmr[p]=getLMR(a,b);
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
            p.lastLine = a->id();
            p.nextLine = b->id();

            if(a->endStation() == b->startStation() && a->startStation() != b->endStation()){
                if(g_m_l_adj.keys().contains(a->id())){
                    if(g_m_l_adj[a->id()].contains(b))continue;

                    if(g_m_lmr.keys().contains(p) && g_m_lmr[p] != PATH_LMF_NOWAY){
                        g_m_l_adj[a->id()].append(b);
                    }
                }else{
                    //插入
                    if(g_m_lmr[p] != PATH_LMF_NOWAY){
                        QVector<AgvLine*> v;
                        v.append(b);
                        g_m_l_adj[a->id()] = v;
                    }
                }
            }
        }
    }

    ////2.保存地图信息，包括如下内容：
    if(!save()){
        qDebug() << QStringLiteral("地图保存失败");
    }
    //TODO
    emit mapUpdate();
}


bool MapCenter::resetMap(QString stationStr,QString lineStr,QString arcStr)//站点、直线、弧线
{
    clear();
    addStation(stationStr);
    addLine(lineStr);
    addArc(arcStr);
    create();
    return true;
}

bool MapCenter::load()
{
    clear();
    /// 算法 线路 QMap<int,AgvLine *> g_m_agvlines;
    /// 算法 站点 QMap<int,AgvStation *> g_m_agvstations
    /// 左中右信息 QMap<PATH_LEFT_MIDDLE_RIGHT,int> g_m_leftRightMiddle;
    /// adj信息 QMap<int,QVector<AgvLine *> > g_m_adj;

    //stations
    QString queryStationSql = "select mykey,x,y,type,name,lineAmount,lines,rfid from agv_station";
    QStringList params;
    QList<QStringList> result = g_sql->query(queryStationSql,params);
    for(int i=0;i<result.length();++i){
        QStringList qsl = result.at(i);
        if(qsl.length()!=8){
            qDebug() << "select error!!!!!!"<<queryStationSql;
            return false;
        }
        AgvStation *station = new AgvStation;
        station->setId(qsl.at(0).toInt());
        station->setX(qsl.at(1).toInt());
        station->setY(qsl.at(2).toInt());
        station->setType(qsl.at(3).toInt());
        station->setName(qsl.at(4));
        station->setLineAmount(qsl.at(5).toInt());
        station->setRfid(qsl.at(7).toInt());
        g_m_stations.insert(station->id(),station);
    }

    //lines
    QString squeryLineSql = "select mykey,startX,startY,endX,endY,radius,clockwise,line,midX,midY,centerX,centerY,angle,length,startStation,endStation,draw from agv_line";
    result = g_sql->query(squeryLineSql,params);
    for(int i=0;i<result.length();++i){
        QStringList qsl = result.at(i);
        if(qsl.length()!=17){
            qDebug() << "select error!!!!!!"<<squeryLineSql;
            return false;
        }
        AgvLine *line = new AgvLine;
        line->setId(qsl.at(0).toInt());
        line->setStartX(qsl.at(1).toInt());
        line->setStartY(qsl.at(2).toInt());
        line->setEndX(qsl.at(3).toInt());
        line->setEndY(qsl.at(4).toInt());
        line->setRadius(qsl.at(5).toInt());
        line->setClockwise(qsl.at(6)!="0");
        line->setLine(qsl.at(7)!="0");
        line->setMidX(qsl.at(8).toInt());
        line->setMidY(qsl.at(9).toInt());
        line->setCenterX(qsl.at(10).toInt());
        line->setCenterY(qsl.at(11).toInt());
        line->setAngle(qsl.at(12).toInt());
        line->setLength(qsl.at(13).toInt());
        line->setStartStation(qsl.at(14).toInt());
        line->setEndStation(qsl.at(15).toInt());
        line->setDraw(qsl.at(16)!="0");
        g_m_lines.insert(line->id(),line);
    }
    //反方向线路
    for(QMap<int,AgvLine *>::iterator itr = g_m_lines.begin();itr!=g_m_lines.end();++itr){
        for(QMap<int,AgvLine *>::iterator pos = g_m_lines.begin();pos!=g_m_lines.end();++pos){
            if(itr.key()!=pos.key()
                    &&itr.value()->startStation() == pos.value()->endStation()
                    &&itr.value()->endStation() == pos.value()->startStation() ){
                g_reverseLines[itr.key()] = pos.key();
                g_reverseLines[pos.key()] = itr.key();
            }
        }
    }

    //lmr
    QString queryLmrSql = "select lastLine,nextLine,lmr from agv_lmr";
    result = g_sql->query(queryLmrSql,params);
    for(int i=0;i<result.length();++i){
        QStringList qsl = result.at(i);
        if(qsl.length()!=3){
            qDebug() << "select error!!!!!!"<<queryLmrSql;
            return false;
        }
        PATH_LEFT_MIDDLE_RIGHT ll;
        ll.lastLine = qsl.at(0).toInt();
        ll.nextLine = qsl.at(1).toInt();
        g_m_lmr.insert(ll,qsl.at(2).toInt());
    }

    //adj
    QString queryAdjSql = "select mykey,lines from agv_adj";
    result = g_sql->query(queryAdjSql,params);
    for(int i=0;i<result.length();++i){
        QStringList qsl = result.at(i);
        if(qsl.length()!=2){
            qDebug() << "select error!!!!!!"<<queryAdjSql;
            return false;
        }
        QVector<AgvLine*> lines;
        QStringList linesSStr = qsl.at(1).split(",");
        for(int j=0;j<linesSStr.length();++j){
            int kk = linesSStr.at(j).toInt();
            lines.append(g_m_lines[kk]);
        }
        g_m_l_adj[qsl.at(0).toInt()] = lines;
    }

    return true;
}

bool MapCenter::save()
{
    QString deleteStationSql = "delete from agv_station;";
    QStringList params;

    bool b = g_sql->exec(deleteStationSql,params);
    if(!b){
        qDebug()<<"can not clear table agv_station!";
        return false;
    }
    QString deleteLineSql = "delete from agv_line;";
    b = g_sql->exec(deleteLineSql,params);
    if(!b){
        qDebug()<<"can not clear table agv_line!";
        return false;
    }
    QString deleteLmrSql = "delete from agv_lmr;";
    b = g_sql->exec(deleteLmrSql,params);
    if(!b){
        qDebug()<<"can not clear table agv_lmr!";
        return false;
    }
    QString deleteAdjSql = "delete from agv_adj;";
    b = g_sql->exec(deleteAdjSql,params);
    if(!b){
        qDebug()<<"can not clear table agv_adj!";
        return false;
    }
    //插入数据
    QString insertStationSql = "insert into agv_station(mykey,x,y,type,name,lineAmount,rfid) values(?,?,?,?,?,?,?,?);";
    for(QMap<int,AgvStation *>::iterator itr = g_m_stations.begin();itr!=g_m_stations.end();++itr){
        AgvStation *s = itr.value();
        params.clear();
        params<<QString("%1").arg(s->id())<<QString("%1").arg(s->x())<<QString("%1").arg(s->y())<<QString("%1").arg(s->type())<<s->name()<<QString("%1").arg(s->lineAmount())<<QString("%1").arg(s->rfid());
        if(!g_sql->exec(insertStationSql,params))
        {
            qDebug() <<" insert into agv_station failed!";
            return false;
        }
    }

    QString insertLineSql = "insert into agv_line(mykey,startX,startY,endX,endY,radius,clockwise,line,midX,midY,centerX,centerY,angle,length,startStation,endStation,draw) values(?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?);";
    for(QMap<int,AgvLine *>::iterator itr =  g_m_lines.begin();itr!=g_m_lines.end();++itr){
        AgvLine *l = itr.value();
        params.clear();
        params<<QString("%1").arg(l->id())<<QString("%1").arg(l->startX())<<QString("%1").arg(l->startY())<<QString("%1").arg(l->endX())<<QString("%1").arg(l->endY())<<QString("%1").arg(l->radius())<<QString("%1").arg(l->clockwise())<<QString("%1").arg(l->line())<<QString("%1").arg(l->midX())<<QString("%1").arg(l->midY())<<QString("%1").arg(l->centerX())<<QString("%1").arg(l->centerY())<<QString("%1").arg(l->angle())<<QString("%1").arg(l->length())<<QString("%1").arg(l->startStation())<<QString("%1").arg(l->endStation())<<QString("%1").arg(l->draw());
        if(!g_sql->exec(insertLineSql,params))
        {
            qDebug() <<" insert into agv_line failed!";
            return false;
        }
    }

    QString insertLmrSql = "insert into agv_lmr(lastLine,nextLine,lmr) values(?,?,?);";
    for(QMap<PATH_LEFT_MIDDLE_RIGHT,int>::iterator itr = g_m_lmr.begin();itr!=g_m_lmr.end();++itr){
        params.clear();
        params<<QString("%1").arg(itr.key().lastLine)<<QString("%1").arg(itr.key().nextLine)<<QString("%1").arg(itr.value());
        if(!g_sql->exec(insertLmrSql,params)){
            qDebug() << "insert into agv_lmr failed!";
            return false;
        }
    }

    QString insertAdjSql = "insert into agv_adj(myKey,lines) values(?,?);";
    for(QMap<int,QVector<AgvLine*> >::iterator itr = g_m_l_adj.begin();itr!=g_m_l_adj.end();++itr)
    {
        params.clear();
        QString linesStr = "";
        for(QVector<AgvLine *>::iterator pos = itr.value().begin();pos!=itr.value().end();++pos){
            AgvLine *lt = *pos;
            linesStr.append(QString("%1,").arg(lt->id()));
        }
        if(linesStr.endsWith(","))
            linesStr = linesStr.left(linesStr.length()-1);
        params<<QString("%1").arg(itr.key())<<linesStr;
        if(!g_sql->exec(insertAdjSql,params)){
            qDebug() << "insert into agv_adj failed!";
            return false;
        }
    }

    qDebug() << "...........................................................";

    return true;
}

QList<int> MapCenter::getBestPath(int agvId, int lastStation, int startStation, int endStation, int &distance, bool canChangeDirect)//最后一个参数是是否可以换个方向
{
    qDebug() << "getbestlinepath==>   lastStation="<<lastStation<<"startStation="<<startStation<<"endStation="<<endStation;
    distance = distance_infinity;
    int disA=distance_infinity;
    int disB=distance_infinity;
    //先找到小车不掉头的线路
    QList<int> a = getPath(agvId,lastStation,startStation,endStation,disA,false);
    qDebug() << "disA ="<<disA;
    QList<int> b;
    if(canChangeDirect){//如果可以掉向，那么计算一下掉向的
        b = getPath(agvId,startStation,lastStation,endStation,disB,true);
        qDebug() << "disB ="<<disB;
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
            if(g_m_stations[endPoint]->occuAgv()!=0 && g_m_stations[endPoint]->occuAgv()!=agvId)
            {
                mutex.unlock();
                return result;
            }
            //那么返回一个结果
            for(QMap<int,AgvLine *>::iterator itr =  g_m_lines.begin();itr!=g_m_lines.end();++itr){
                //所有线路为白色，距离为未知
                AgvLine *line =itr.value();
                if(line->startStation() == lastPoint  && line->endStation() == startPoint ){
                    result.push_back(line->id());
                    distance = line->length();
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
    AgvStation *lastStation = g_m_stations[lastPoint];
    if(lastPoint == startPoint){
        //如果lastPoint和startPoint相同，那么说明每个方向都可以
        for(QMap<int,AgvLine *>::iterator itr =  g_m_lines.begin();itr!=g_m_lines.end();++itr){
            //所有线路为白色，距离为未知
            AgvLine *line =itr.value();
            if(line->startStation() == startPoint && (line->occuAgv()==0 || line->occuAgv() == agvId)){//以改点未起点，并且未被占用(或者被当前车辆占用的)
                line->distance = line->length();
                line->color= AGV_LINE_COLOR_GRAY;
                Q.insert(line->length(),line->id());
            }
        }
    }else{
        //找到那个对应的线路
        for(QMap<int,AgvLine *>::iterator itr =  g_m_lines.begin();itr!=g_m_lines.end();++itr){
            //所有线路为白色，距离为未知
            AgvLine *line =itr.value();
            if(line->occuAgv()!=0 && line->occuAgv()!=agvId)continue;//逆向占用了（而且是被不是当前车辆的车占用了）
            if(g_m_stations[line->endStation()]->occuAgv()!=0 && g_m_stations[line->endStation()]->occuAgv()!=agvId)continue;//这条线路的终点被占用了
            if(line->endStation() == startPoint){
                line->distance = 0;
                line->color = AGV_LINE_COLOR_GRAY;
                Q.insert(line->length(),line->id());
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
            if(l->occuAgv()!=0 && l->occuAgv()!=agvId)continue;//反向被占用，不考虑他了
            if(g_m_stations[l->endStation()]->occuAgv()!=0 && g_m_stations[l->endStation()]->occuAgv()!=agvId)continue;//这条线路的终点被占用了
            if(l->color == AGV_LINE_COLOR_BLACK)
            {
                continue;
            }else if(l->color == AGV_LINE_COLOR_WHITE){
                //白色直接赋值，并加入Q中
                l->distance = g_m_lines[tLine]->distance + l->length();
                Q.insert(l->distance,l->id());
                l->color = AGV_LINE_COLOR_GRAY;
                l->father = tLine;
            }else if(l->color == AGV_LINE_COLOR_GRAY){
                if(l->distance > g_m_lines[tLine]->distance + l->length())
                {
                    l->distance = g_m_lines[tLine]->distance + l->length();
                    l->father = tLine;
                    //更新Q中的节点的distance
                    //这里要执行一次删除和插入的操作

                    //删除
                    for(QMultiMap<int,int>::iterator iitr = Q.begin();iitr!=Q.end();++iitr){
                        if(iitr.value() == l->id()){
                            Q.erase(iitr);
                            break;
                        }
                    }
                    //插入
                    Q.insert(l->distance,l->id());
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
        if(t->endStation() == endPoint){
            if(t->distance<minDis){
                minDis = t->distance;
                index = t->id();
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
            if(g_m_lines[result.at(0)]->startStation()==lastPoint && g_m_lines[result.at(0)]->endStation()==startPoint){
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

