#ifndef USERMSGPROCESSOR_H
#define USERMSGPROCESSOR_H

#include <QThread>
#include "util/global.h"

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
    void UserMsgProcessor::parseOneMsg(const QyhMsgDateItem &item, const std::string &oneMsg);
    void responseOneMsg(const QyhMsgDateItem &item, std::map<string,string> requestDatas, std::vector<std::map<std::string,std::string> > datalists);
};

#endif // USERMSGPROCESSOR_H
