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
    void run();
signals:

public slots:
private:
    volatile bool isQuit;
signals:

public slots:
};

#endif // AGVSTATUSPUBLISHER_H
