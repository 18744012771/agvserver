#ifndef AGVLINE_H
#define AGVLINE_H

#include <QObject>

#define distance_infinity INT_MAX

enum{
    AGV_LINE_COLOR_WHITE = 0,  //未算出路径最小值
    AGV_LINE_COLOR_GRAY,       //已经计算出一定的值，在Q队列中，但是尚未计算出最小值
    AGV_LINE_COLOR_BLACK,      //已算出路径最小值
};

class AgvLine
{
public:
    AgvLine():
        line(false),
        p1x(0.0),
        p1y(0.0),
        p2x(0.0),
        p2y(0.0),
        id(0),
        draw(false),
        occuAgv(0),
        length(0),
        rate(0),
        color_r(0),
        color_g(255),
        color_b(0)
    {
    }
    AgvLine::AgvLine(const AgvLine &b)
    {
        line=b.line;
        p1x=b.p1x;
        p2x=b.p2x;
        p1y=b.p1y;
        p2y=b.p2y;
        id=b.id;
        draw=b.draw;
        occuAgv=b.occuAgv;
        length=b.length;
        startStation=b.startStation;
        endStation=b.endStation;
        rate=b.rate;
        color_r = b.color_r;
        color_g = b.color_g;
        color_b = b.color_b;
    }
    bool operator <(const AgvLine &b){
        return id<b.id;
    }

    //直线弧线共有部分
    bool line;//true为直线 false为曲线
    int id;
    bool draw;
    int occuAgv;
    double length;
    int startStation;
    int endStation;
    double rate;

    //弧线单独部分
    //弧线(贝塞尔曲线),三次贝塞尔曲线，共四个点，P0(startx,starty),P1,P2,P3(endX,endY);这里需要额外的两个点坐标P1,P2
    double p1x;
    double p1y;
    double p2x;
    double p2y;
    //这条线路的颜色RGB，显示用的
    int color_r;
    int color_g;
    int color_b;
    ////////////////////////////下面这部分是计算路径用的
public:
    //计算路径用的
    int father;
    int distance;//起点到这条线的终点 的距离
    int color;

    bool operator == (const AgvLine &b){
        return this->startStation == b.startStation&& this->endStation == b.endStation;
    }
};

#endif // AGVLINE_H
