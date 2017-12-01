#ifndef AGVTASKPUBLISHER_H
#define AGVTASKPUBLISHER_H

#include <QObject>
#include <QThread>
#include <QList>
#include <QMutex>


class AgvTaskPublisher : public QThread
{
    Q_OBJECT
public:
    explicit AgvTaskPublisher(QObject *parent = nullptr);
    ~AgvTaskPublisher();
    void addSubscribe(int subscribe);
    void removeSubscribe(int subscribe);
    void run();
signals:

public slots:
private:
    QList<int> subscribers;
    QMutex mutex;
    volatile bool isQuit;
};

#endif // AGVTASKPUBLISHER_H
