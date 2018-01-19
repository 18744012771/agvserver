#ifndef MAPCENTER_H
#define MAPCENTER_H

#include <QObject>
#include <QMap>
#include <QMutex>
#include "bean/agvline.h"
#include "bean/agvstation.h"

//地图由四个信息描述
//基本的绘图信息是
//agv_line  (线路)
//agv_station (站点)
//为了计算最优路径，还定义了两个辅助类  分别是
//agv_lmr(左中右信息)
//和agv_adj (这条线路 与 这条线路能到到的其他线路 的对应关系)

class MapCenter : public QObject
{
    Q_OBJECT
public:

    struct PATH_LEFT_MIDDLE_RIGHT{
        int lastLine;
        int nextLine;

        bool operator == (const PATH_LEFT_MIDDLE_RIGHT &r){
            return lastLine == r.lastLine && nextLine == r.nextLine;
        }

        bool operator < (const PATH_LEFT_MIDDLE_RIGHT &r) const
        {
            if(lastLine!=r.lastLine){
                return lastLine<r.lastLine;
            }

            return nextLine<r.nextLine;
        }
    };

    enum{
        PATH_LMF_NOWAY = -2,//代表可能要掉头行驶
        PATH_LMR_LEFT=-1,
        PATH_LMR_MIDDLE=0,
        PATH_LMR_RIGHT=1,
    };

    explicit MapCenter(QObject *parent = nullptr);

    //1.创建地图
    bool resetMap(QString stationStr,QString lineStr,QString arcStr,QString imagestr);//站点、直线、弧线

    //2.从数据库中载入地图
    bool load();

    //获取最优路径
    QList<int> getBestPath(int agvId, int lastStation, int startStation, int endStation, int &distance, bool canChangeDirect = false);//最后一个参数是是否可以换个方向

    //设置lineid的反向线路的占用agv
    void setReverseOccuAgv(int lineid,int occagv);

    AgvStation getAgvStation(int id){
        AgvStation s;
        mutex.lock();
        if(g_m_stations.contains(id)){
            s = *(g_m_stations[id]);
        }
        mutex.unlock();
        return s;
    }

    QMap<int,AgvStation *> getAgvStations(){
        QMap<int,AgvStation *> stations;
        mutex.lock();
        stations = g_m_stations;
        mutex.unlock();
        return stations;
    }

    AgvLine getAgvLine(int id)
    {
        AgvLine l;
        mutex.lock();
        if(g_m_lines.contains(id)){
            l = *(g_m_lines[id]);
        }
        mutex.unlock();
        return l;
    }

    QMap<int,AgvLine *> getAgvLines()
    {
        QMap<int,AgvLine *> lines;
        mutex.lock();
        lines = g_m_lines;
        mutex.unlock();
        return lines;
    }

    int getReverseLine(int id)
    {
        if(g_reverseLines.contains(id))return g_reverseLines[id];
        return 0;
    }

    int getLMR(int startLineId,int nextLineId)
    {
        int result = PATH_LMF_NOWAY;
        mutex.lock();
        PATH_LEFT_MIDDLE_RIGHT p;
        p.lastLine = startLineId;
        p.nextLine = nextLineId;
        if(g_m_lmr.contains(p))
            result = g_m_lmr[p];
        mutex.unlock();
        return result;
    }

signals:
    void mapUpdate();//地图更新了,通知前端的所有显示界面，更新地图
public slots:

private:
    QMutex mutex;//获取最优路径时，加锁

    void clear();
    void addStation(QString s);
    void addLine(QString s);
    void addArc(QString s);
    void create();

    QList<int> getPath(int agvId,int lastPoint,int startPoint,int endPoint,int &distance,bool changeDirect);

    int getLMR(AgvLine *lastLine,AgvLine *nextLine);

    QMap<PATH_LEFT_MIDDLE_RIGHT,int> g_m_lmr; //左中右
    QMap<int,QList<AgvLine*> > g_m_l_adj;  //从一条线路到另一条线路的关联表

    QMap<int,int> g_reverseLines;//线路和它的反方向线路的集合。

    QMap<int,AgvStation *> g_m_stations;  //所有的站点(站点+线路 = 地图)
    QMap<int,AgvLine *> g_m_lines;        //所有的线路(站点+线路 = 地图)
};

#endif // MAPCENTER_H
