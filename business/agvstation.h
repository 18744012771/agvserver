#ifndef AGVSTATION_H
#define AGVSTATION_H

#include <QObject>

enum AGV_STATION_TYPE{
    AGV_STATION_TYPE_FATE = 0,//虚站点
    AGV_STATION_TYPE_RFID = 1,//RFID站点
    AGV_STATION_TYPE_REAL//真实站点，它其实也是RFID点
};



class AgvStation : public QObject
{
    Q_OBJECT
public:
    explicit AgvStation(QObject *parent = nullptr);

    //getter
    int x(){return  m_x;}
    int y(){return  m_y;}
    int type(){return  m_type;}
    QString name(){return  m_name;}
    int lineAmount(){return  m_lineAmount;}
    int id(){return  m_id;}
    int rfid(){return  m_rfid;}
    int occuAgv(){return m_occuAgv;}

    //setter
    void setX(int newX){m_x=newX;emit xChanged(newX);}
    void setY(int newY){m_y=newY;emit yChanged(newY);}
    void setType(int newType){m_type=newType;emit typeChanged(newType);}
    void setName(QString newName){m_name=newName;emit nameChanged(newName);}
    void setLineAmount(int newLineAmount){m_lineAmount=newLineAmount;emit lineAmountChanged(newLineAmount);}
    void setId(int newId){m_id=newId;emit idChanged(newId);}
    void setRfid(int newRfid){m_rfid=newRfid;emit rfidChanged(newRfid);}
    void setOccuAgv(int newOccuAgv){m_occuAgv=newOccuAgv;emit occuAgvChanged(newOccuAgv);}
signals:
    void xChanged(int newX);
    void yChanged(int newY);
    void typeChanged(int newType);
    void nameChanged(QString newName);
    void lineAmountChanged(int newLineAmount);
    void idChanged(int newId);
    void rfidChanged(int newRfid);
    void occuAgvChanged(int newOccuAgv);
public slots:

private:
    int m_x;
    int m_y;
    int m_type;
    QString m_name;
    int m_lineAmount;
    int m_id;
    int m_rfid;
    int m_occuAgv;
};

#endif // AGVSTATION_H
