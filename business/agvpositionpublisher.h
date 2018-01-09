#ifndef AGVPOSITIONPUBLISHER_H
#define AGVPOSITIONPUBLISHER_H

#include <QObject>
#include <QThread>
#include <QList>
#include <QMutex>


//其实可以将它定义未观察者模式
class AgvPositionPublisher : public QThread
{
    Q_OBJECT
public:
    explicit AgvPositionPublisher(QObject *parent = nullptr);
    ~AgvPositionPublisher();
    void run();
signals:

public slots:
private:
    volatile bool isQuit;
};

#endif // AGVPOSITIONPUBLISHER_H
