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
        startX(0),
        startY(0),
        endX(0),
        endY(0),
        line(false),
        p1x(0),
        p1y(0),
        p2x(0),
        p2y(0),
        id(0),
        draw(false),
        occuAgv(0),
        length(0),
        rate(0)
    {
    }
    AgvLine::AgvLine(const AgvLine &b)
    {
        startX=b.startX;
        startY=b.startY;
        endX=b.endX;
        endY=b.endY;
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
    }
    bool operator <(const AgvLine &b){
        return id<b.id;
    }

    //直线弧线共有部分
    int startX;
    int startY;
    int endX;
    int endY;
    bool line;//true为直线 false为曲线
    int id;
    bool draw;
    int occuAgv;
    int length;
    int startStation;
    int endStation;
    double rate;

    //弧线单独部分
    //弧线(贝塞尔曲线),三次贝塞尔曲线，共四个点，P0(startx,starty),P1,P2,P3(endX,endY);这里需要额外的两个点坐标P1,P2
    int p1x;
    int p1y;
    int p2x;
    int p2y;


public:
    //计算路径用的
    int father;
    int distance;//起点到这条线的终点 的距离
    int color;


    //TODO
    //TODO
    bool operator == (const AgvLine &b){
        return this->startStation == b.startStation&& this->endStation == b.endStation;
    }
};

#endif // AGVLINE_H
