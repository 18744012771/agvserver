#ifndef MAPCENTER_H
#define MAPCENTER_H

#include <QObject>
#include <QMutex>
#include "agvline.h"

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
    //对外接口

    //1.创建地图
    bool resetMap(QString stationStr,QString lineStr,QString arcStr);//站点、直线、弧线

    //2.从数据库中载入地图
    bool load();

    //获取最优路径
    QList<int> getBestPath(int agvId, int lastStation, int startStation, int endStation, int &distance, bool canChangeDirect = false);//最后一个参数是是否可以换个方向

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
    //bool save();//保存创建的地图

    QList<int> getPath(int agvId,int lastPoint,int startPoint,int endPoint,int &distance,bool changeDirect);

    int getLMR(AgvLine *lastLine,AgvLine *nextLine);

};

#endif // MAPCENTER_H
