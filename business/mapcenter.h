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
    explicit MapCenter(QObject *parent = nullptr);

    //1.创建地图
    bool resetMap(QString stationStr,QString lineStr,QString arcStr,QString imagestr);//站点、直线、弧线

    //2.从数据库中载入地图
    bool load();

    //获取最优路径
    QList<int> getBestPath(int agvId, int lastStation, int startStation, int endStation, int &distance, bool canChangeDirect = false);//最后一个参数是是否可以换个方向

    //占领一个站点
    bool setStationOccuAgv(int station,int occuAgv);
    //设置lineid的反向线路的占用agv
    void setReverseOccuAgv(int lineid,int occagv);
    //如果站点被agv占用了，那么释放
    void freeStationIfAgvOccu(int station, int occuAgv);
    //如果线路被agv占用了，那么释放[含反向线路]
    void freeLineIfAgvOccu(int line,int occuAgv);
    //释放车辆占用的线路，除了某条线路【因为车辆停在了一条线路上】
    void freeAgvLines(int agvId,int exceptLine = 0);
    //释放车辆占用的站点，除了某个站点【因为车辆站在某个站点上】
    void freeAgvStation(int agvId,int excepetStation = 0);

    int getLineId(int startStation,int endStation);

    AgvStation getAgvStation(int id);

    QMap<int,AgvStation *> getAgvStations();

    AgvLine getAgvLine(int id);
    QMap<int,AgvLine *> getAgvLines();

    int getReverseLine(int id);

    int getLMR(int startLineId,int nextLineId);

signals:
    void mapUpdate();//地图更新了,通知前端的所有显示界面，更新地图
public slots:

private:
    void clear();
    void addStation(QString s);
    void addLine(QString s);
    void addArc(QString s);
    void create();

    QList<int> getPath(int agvId,int lastPoint,int startPoint,int endPoint,int &distance,bool changeDirect);

    int getLMR(AgvLine *lastLine,AgvLine *nextLine);
};

#endif // MAPCENTER_H
