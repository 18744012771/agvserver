#ifndef LOG_H
#define LOG_H

#include <QList>
#include <QString>





class Log
{
public:
    typedef struct log_log{
        int LOG_TYPE;           //日子类型
        int LOG_FROM_ID;        //日志产生者id
        int LOG_LEVEL;          //INFO/WARNING/ERROR
        QString msg;            //具体内容
        QString date;           //具体时间
    }LOG_LOG;
	
    Log();

    void log(LOG_LOG _log);//记录一条日志
    LOG_LOG getLastLog();//获取最后一条日志
    QList<LOG_LOG> getLogByFrom(int from_id,QString date_from,QString date_to);
    QList<LOG_LOG> getLogByType(int type,QString date_from,QString date_to);
    QList<LOG_LOG> getLogByDate(QString date_from,QString date_to);
private:
    LOG_LOG lastlog;

    void test();
};

#endif // LOG_H
