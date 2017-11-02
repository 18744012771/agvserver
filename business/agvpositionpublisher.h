#ifndef AGVPOSITIONPUBLISHER_H
#define AGVPOSITIONPUBLISHER_H

#include <QObject>
#include <QThread>
#include <list>
#include <mutex>

//其实可以将它定义未观察者模式
class AgvPositionPublisher : public QThread
{
    Q_OBJECT
public:
    explicit AgvPositionPublisher(QObject *parent = nullptr);
    ~AgvPositionPublisher();
    void addSubscribe(int subscribe);
    void removeSubscribe(int subscribe);
    void run();
signals:

public slots:
private:
    std::list<int> subscribers;
    std::mutex mutex;
    volatile bool isQuit;
};

#endif // AGVPOSITIONPUBLISHER_H
