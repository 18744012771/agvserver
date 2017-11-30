#ifndef AGVSTATION_H
#define AGVSTATION_H

#include <QString>

//去掉了虚站点!所有站点都是RFID点
enum AGV_STATION_TYPE{
    AGV_STATION_TYPE_RFID = 1,//RFID站点
    AGV_STATION_TYPE_REAL//真实站点，它其实也是RFID点
};



class AgvStation
{
public:
    AgvStation():
        x(0),
        y(0),
        type(0),
        name(""),
        lineAmount(0),
        id(0),
        rfid(0),
        occuAgv(0)
    {
    }

    AgvStation::AgvStation(const AgvStation &b)
    {
        x=b.x;
        y=b.y;
        type=b.type;
        name=b.name;
        lineAmount=b.lineAmount;
        id=b.id;
        rfid=b.rfid;
        occuAgv=b.occuAgv;
    }

    bool operator <(const AgvStation &b){
        return id<b.id;
    }

    int x;
    int y;
    int type;
    QString name;
    int lineAmount;
    int id;
    int rfid;
    int occuAgv;

    bool operator == (const AgvStation &b){
        return this->id == b.id;
    }
};

#endif // AGVSTATION_H
