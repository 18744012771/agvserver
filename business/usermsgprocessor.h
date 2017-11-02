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

    std::string makeAccessToken();
signals:

public slots:

private:
    volatile bool isQuit;

    //对接收到的一条消息进行xml解析
    void UserMsgProcessor::parseOneMsg(const QyhMsgDateItem &item, const std::string &oneMsg);

    //对解析后的请求信息，进行处理，并得到相应的回应内容
    void responseOneMsg(const QyhMsgDateItem &item, std::map<string,string> requestDatas, std::vector<std::map<std::string,std::string> > datalists);

};

#endif // USERMSGPROCESSOR_H
