#ifndef AGVLOGPROCESS_H
#define AGVLOGPROCESS_H

#include <QObject>
#include <QThread>
#include <QMap>
#include <QMutex>


struct SubNode{
    bool trace = false;
    bool debug = false;
    bool info = false;
    bool warn = false;
    bool error = false;
    bool fatal = false;
};

class AgvLogProcess : public QThread
{
    Q_OBJECT
public:
    explicit AgvLogProcess(QObject *parent = nullptr);

    ~AgvLogProcess();
    void addSubscribe(int sock,SubNode node);
    void removeSubscribe(int sock);
    void run() override;

signals:

public slots:

private:
    QMap<int,SubNode> subscribers;
    QMutex mutex;
    volatile bool isQuit;
};

#endif // AGVLOGPROCESS_H
