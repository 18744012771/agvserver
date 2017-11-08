#ifndef AGVLOG_H
#define AGVLOG_H

#include <QObject>
#include <QString>
#include <QDateTime>

typedef enum{
    AGV_LOG_LEVEL_TRACE = 0,//痕迹
    AGV_LOG_LEVEL_DEBUG = 1,//调试
    AGV_LOG_LEVEL_INFO = 2,//信息
    AGV_LOG_LEVEL_WARN = 3,//警告
    AGV_LOG_LEVEL_ERROR = 4,//错误
    AGV_LOG_LEVEL_FATAL = 5,//致命错误
}AGV_LOG_LEVEL;

struct OneLog{
    QDateTime time;
    AGV_LOG_LEVEL level;
    QString msg;
};

class AgvLog : public QObject
{
    Q_OBJECT
public:
    explicit AgvLog(QObject *parent = nullptr);

    void log(AGV_LOG_LEVEL level, QString msg);
signals:

public slots:
};

#endif // AGVLOG_H
