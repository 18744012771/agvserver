#include "agvlog.h"
#include <QDateTime>
#include <QDebug>
#include "util/global.h"

AgvLog::AgvLog(QObject *parent):QObject(parent)
{

}

void AgvLog::log(AGV_LOG_LEVEL level, std::string msg)
{
    //本身控制台是全部消息都要打印的
    QDateTime now = QDateTime::currentDateTime();

    QString strLevel = "";
    switch (level) {
    case AGV_LOG_LEVEL_TRACE:
        strLevel = "[TRACE]";
        break;
    case AGV_LOG_LEVEL_DEBUG:
        strLevel = "[DEBUG]";
        break;
    case AGV_LOG_LEVEL_INFO:
        strLevel = "[INFO]";
        break;
    case AGV_LOG_LEVEL_WARN:
        strLevel = "[WARN]";
        break;
    case AGV_LOG_LEVEL_ERROR:
        strLevel = "[ERROR]";
        break;
    case AGV_LOG_LEVEL_FATAL:
        strLevel = "[FATAL]";
        break;
    default:
        strLevel = "[UNKNOWN]";
        break;
    }
    //1.打印
    qDebug() <<now.toString(DATE_TIME_FORMAT)<<strLevel<<QString::fromLocal8Bit(msg.c_str());

    //2.入队一个消息
    OneLog onelog;
    onelog.level = level;
    onelog.time=now;
    onelog.msg = msg;

    //g_log_queue.enqueue(onelog);
}

