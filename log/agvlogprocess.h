#ifndef AGVLOGPROCESS_H
#define AGVLOGPROCESS_H

#include <QObject>
#include <QThread>
#include <list>
#include <mutex>
#include "util/global.h"

class AgvLogProcess : public QThread
{
    Q_OBJECT
public:
    explicit AgvLogProcess(QObject *parent = nullptr);

    ~AgvLogProcess();
    void addSubscribe(int sock,int role = 0);
    void removeSubscribe(int sock);
    void run() override;

signals:

public slots:

private:
    std::list<std::pair<int,int> > subscribers;
    std::mutex mutex;
    volatile bool isQuit;
};

#endif // AGVLOGPROCESS_H
