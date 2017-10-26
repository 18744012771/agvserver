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
signals:

public slots:
};

#endif // AGVCENTER_H
