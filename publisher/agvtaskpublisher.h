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
    void run();
signals:

public slots:
private:
    volatile bool isQuit;
};

#endif // AGVTASKPUBLISHER_H
