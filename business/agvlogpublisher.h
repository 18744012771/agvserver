#ifndef AGVLOGPUBLISHER_H
#define AGVLOGPUBLISHER_H

#include <QObject>
#include <QThread>
#include <list>
#include <mutex>

class AgvLogPublisher : public QThread
{
    Q_OBJECT
public:
    explicit AgvLogPublisher(QObject *parent = nullptr);

    ~AgvLogPublisher();
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

#endif // AGVLOGPUBLISHER_H
