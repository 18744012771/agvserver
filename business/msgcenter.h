#ifndef MSGCENTER_H
#define MSGCENTER_H

#include <QObject>
#include <QThread>
#include "publisher/agvpositionpublisher.h"
#include "publisher/agvstatuspublisher.h"
#include "publisher/agvtaskpublisher.h"
//这里将会启动一个CPU个数*2的线程，用于处理用户的数据
//保证并发量和响应时间

class MsgCenter : public QObject
{
    Q_OBJECT
public:
    explicit MsgCenter(QObject *parent = nullptr);
    ~MsgCenter();
    void init();
signals:

public slots:

private:
    AgvPositionPublisher *positionPublisher;
    AgvStatusPublisher *statusPublisher;
    AgvTaskPublisher *taskPublisher;
};

#endif // MSGCENTER_H
