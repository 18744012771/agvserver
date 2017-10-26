#ifndef MAPCENTER_H
#define MAPCENTER_H

#include <QObject>
#include <QMutex>
#include "agvline.h"

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
    QList<int> getBestPath(int agvId, int lastStation, int startStation, int endStation, int &distance, bool canChangeDirect);//最后一个参数是是否可以换个方向

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
    bool save();//保存创建的地图

    QList<int> getPath(int agvId,int lastPoint,int startPoint,int endPoint,int &distance,bool changeDirect);

    int getLMR(AgvLine *lastLine,AgvLine *nextLine);

};

#endif // MAPCENTER_H
