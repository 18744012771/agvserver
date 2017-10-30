#ifndef USERMSGPROCESSOR_H
#define USERMSGPROCESSOR_H

#include <QThread>

class UserMsgProcessor : public QThread
{
    Q_OBJECT
public:
    explicit UserMsgProcessor(QObject *parent = nullptr);
    ~UserMsgProcessor();
    void run() override;
    void myquit();
signals:

public slots:

private:
    volatile bool isQuit;
};

#endif // USERMSGPROCESSOR_H
