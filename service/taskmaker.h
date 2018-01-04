#ifndef TASKMAKER_H
#define TASKMAKER_H

#include <QObject>
#include <QThread>
#include <QMap>

class TaskMaker : public QObject
{
    Q_OBJECT
public:
    TaskMaker();
    ~TaskMaker();

    bool init();
signals:
    void sigInit();
    void sigTaskAccept(int);
    void sigTaskStart(int,int);
    void sigTaskErrorEmpty(int);
    void sigTaskErrorFull(int);
    void sigTaskFinish(int);
public slots:
    void onInitResults(bool);
    void onNewTask(int,int,int,int,int);

    void onTaskStart(int taskId, int agv);
    void onTaskFinish(int taskId);
private:
    QThread workerThread;
    volatile bool hasInit;
    bool initResult;
    QMap<int,int> m_taskId_workNo;
};

#endif // TASKMAKER_H
