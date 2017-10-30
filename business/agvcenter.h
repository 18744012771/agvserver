#ifndef AGVCENTER_H
#define AGVCENTER_H

#include <QObject>
#include <QList>
#include "agv.h"

class AgvCenter : public QObject
{
    Q_OBJECT
public:
    explicit AgvCenter(QObject *parent = nullptr);
    QList<Agv *> getIdleAgvs();

    bool agvStartTask(int agvId,QList<int> path);

    bool load();//从数据库载入所有的agv
    bool save();//将agv保存到数据库
signals:
    void carArriveStation(int agvId,int station);
public slots:
};

#endif // AGVCENTER_H
