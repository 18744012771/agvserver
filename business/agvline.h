#ifndef AGVLINE_H
#define AGVLINE_H

#include <QObject>

#define distance_infinity INT_MAX

enum{
    AGV_LINE_COLOR_WHITE = 0,  //未算出路径最小值
    AGV_LINE_COLOR_GRAY,       //已经计算出一定的值，在Q队列中，但是尚未计算出最小值
    AGV_LINE_COLOR_BLACK,      //已算出路径最小值
};

class AgvLine : public QObject
{
    Q_OBJECT
public:
    explicit AgvLine(QObject *parent = nullptr);

    //getter
    int startX() const {return m_startX;}
    int startY() const {return m_startY;}
    int endX() const {return m_endX;}
    int endY() const {return m_endY;}
    int radius() const {return m_radius;}
    bool clockwise() const {return m_clockwise;}
    bool line() const {return m_line;}
    int midX() const {return m_midX;}
    int midY() const {return m_midY;}
    int centerX() const {return m_centerX;}
    int centerY() const {return m_centerY;}
    int angle() const {return m_angle;}
    int id() const {return m_id;}
    bool draw() const {return m_draw;}
    int occuAgv() const {return m_occuAgv;}//占用这个线路的小车
    int length() const {return m_length;}
    int startStation() const {return m_startStation;}
    int endStation() const {return m_endStation;}


    //setter
    void setStartX(int newStartX){m_startX=newStartX;emit startXChanged(newStartX);}
    void setStartY(int newStartY){m_startY=newStartY;emit startYChanged(newStartY);}
    void setEndX(int newEndX){m_endX=newEndX;emit endXChanged(newEndX);}
    void setEndY(int newEndY){m_endY=newEndY;emit endYChanged(newEndY);}
    void setRadius(int newRadius){m_radius=newRadius;emit radiusChanged(newRadius);}
    void setClockwise(bool newClockwise){m_clockwise=newClockwise;emit clockwiseChanged(newClockwise);}
    void setLine(bool newLine){m_line=newLine;emit lineChanged(newLine);}
    void setMidX(int newMidX){m_midX=newMidX;emit midXChanged(newMidX);}
    void setMidY(int newMidY){m_midY=newMidY;emit midYChanged(newMidY);}
    void setCenterX(int newCenterX){m_centerX=newCenterX;emit centerXChanged(newCenterX);}
    void setCenterY(int newCenterY){m_centerY=newCenterY;emit centerYChanged(newCenterY);}
    void setAngle(int newAngle){m_angle=newAngle;emit angleChanged(newAngle);}
    void setId(int newId){m_id=newId;emit idChanged(newId);}
    void setDraw(bool newDraw){m_draw=newDraw;emit drawChanged(newDraw);}
    void setOccuAgv(int newOccuAgv){m_occuAgv = newOccuAgv;emit occuAgvChanged(newOccuAgv);}
    void setLength(int newLength){m_length=newLength;emit lengthChanged(newLength);}
    void setStartStation(int newStartStation){m_startStation=newStartStation;emit startStationChanged(newStartStation);}
    void setEndStation(int newEndStation){m_endStation=newEndStation;emit endStationChanged(newEndStation);}

signals:
    void startXChanged(int newStartX);
    void startYChanged(int newStartY);
    void endXChanged(int newEndX);
    void endYChanged(int newEndY);
    void radiusChanged(int newRadius);
    void clockwiseChanged(bool newClockwise);
    void lineChanged(bool newLine);
    void midXChanged(int newMidX);
    void midYChanged(int newMidY);
    void centerXChanged(int newCenterX);
    void centerYChanged(int newCenterY);
    void angleChanged(int newAngle);
    void idChanged(int newId);
    void drawChanged(bool newDraw);
    void occuAgvChanged(int newOccuAgv);
    void lengthChanged(int newLength);
    void startStationChanged(int newKUrtStation);
    void endStationChanged(int newKUndStation);
public slots:

private:
    int m_startX;
    int m_startY;
    int m_endX;
    int m_endY;
    int m_radius;
    bool m_clockwise;
    bool m_line;
    int m_midX;
    int m_midY;
    int m_centerX;
    int m_centerY;
    int m_angle;
    int m_id;
    bool m_draw;
    int m_occuAgv;//占用这个线路的小车
    int m_length;
    int m_startStation;
    int m_endStation;

public:
    //计算路径用的
    int father;
    int distance;//起点到这条线的终点 的距离
    int color;

    bool operator == (const AgvLine &b){
        return this->startStation() == b.startStation();// && this->endStation() == b.endStation() && this->length() == b.length();
    }
};

#endif // AGVLINE_H
