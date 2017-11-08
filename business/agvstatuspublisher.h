#ifndef AGVSTATUSPUBLISHER_H
#define AGVSTATUSPUBLISHER_H

#include <QObject>
#include <QThread>
#include <QMap>
#include <QMutex>

class AgvStatusPublisher : public QThread
{
    Q_OBJECT
public:
    explicit AgvStatusPublisher(QObject *parent = nullptr);
    ~AgvStatusPublisher();
    void addSubscribe(int subscribe, int agvid);
    void removeSubscribe(int subscribe, int agvid = 0);
    void run();
signals:

public slots:
private:
    QMap<int,int> subscribers;
    QMutex mutex;
    volatile bool isQuit;
signals:

public slots:
};

#endif // AGVSTATUSPUBLISHER_H
