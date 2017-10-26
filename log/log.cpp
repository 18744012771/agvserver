#include "log.h"
#include <QDateTime>

#include "util/global.h"

Log::Log()
{

}

void Log::log(LOG_LOG _log)//记录一条日志
{
    lastlog = _log;
    QString sql = "INSERT INTO agv_log(type, from_id,level,msg,date) VALUES (?,?,?,?,?)";
    QStringList qsl;
    qsl.append(QString(_log.LOG_TYPE));
    qsl.append(QString(_log.LOG_FROM_ID));
    qsl.append(QString(_log.LOG_LEVEL));
    qsl.append(_log.msg);
    qsl.append(_log.date);
    if(g_sql){
        g_sql->exec(sql,qsl);
    }
}

Log::LOG_LOG Log::getLastLog()//获取最后一条日志
{
    return lastlog;
}

QList<Log::LOG_LOG> Log::getLogByFrom(int from_id,QString date_from,QString date_to)
{
    QList<LOG_LOG> result;
    QString sql = "select ('type', 'from_id','level','msg','date') from agv_log where fromid = ? and date>= ? and date<= ?";
    QStringList qsl;
    qsl.append(QString(from_id));
    qsl.append(date_from);
    qsl.append(date_to);
    if(g_sql){
        QList<QStringList> queryQsl= g_sql->query(sql,qsl);
        for(int i=0;i<queryQsl.length();++i){
            QStringList qsll = queryQsl.at(i);
            if(qsll.length()!=5){
                continue;
            }
            LOG_LOG _log;
            _log.LOG_TYPE = qsll[0].toInt();
            _log.LOG_FROM_ID = qsll[1].toInt();
            _log.LOG_LEVEL = qsll[2].toInt();
            _log.msg = qsll[3];
            _log.date = qsll[4];
            result.append(_log);
        }
    }

    return result;
}

QList<Log::LOG_LOG> Log::getLogByType(int type, QString date_from, QString date_to)
{
    QList<LOG_LOG> result;
    QString sql = "select ('type', 'from_id','level','msg','date') from agv_log where type = ? and date>= ? and date<= ?";
    QStringList qsl;
    qsl.append(QString(type));
    qsl.append(date_from);
    qsl.append(date_to);
    if(g_sql){
        QList<QStringList> queryQsl= g_sql->query(sql,qsl);
        for(int i=0;i<queryQsl.length();++i){
            QStringList qsll = queryQsl.at(i);
            if(qsll.length()!=5){
                continue;
            }
            LOG_LOG _log;
            _log.LOG_TYPE = qsll[0].toInt();
            _log.LOG_FROM_ID = qsll[1].toInt();
            _log.LOG_LEVEL = qsll[2].toInt();
            _log.msg = qsll[3];
            _log.date = qsll[4];
            result.append(_log);
        }
    }

    return result;
}

QList<Log::LOG_LOG> Log::getLogByDate(QString date_from,QString date_to)
{
    QList<LOG_LOG> result;
    QString sql = "select ('type', 'from_id','level','msg','date') from agv_log where date>= ? and date<= ?";
    QStringList qsl;
    qsl.append(date_from);
    qsl.append(date_to);
    if(g_sql){
        QList<QStringList> queryQsl= g_sql->query(sql,qsl);
        for(int i=0;i<queryQsl.length();++i){
            QStringList qsll = queryQsl.at(i);
            if(qsll.length()!=5){
                continue;
            }
            LOG_LOG _log;
            _log.LOG_TYPE = qsll[0].toInt();
            _log.LOG_FROM_ID = qsll[1].toInt();
            _log.LOG_LEVEL = qsll[2].toInt();
            _log.msg = qsll[3];
            _log.date = qsll[4];
            result.append(_log);
        }
    }

    return result;
}


void Log::test()
{
    Log::LOG_LOG logg;
    logg.date = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss");
    logg.LOG_TYPE = 1;
    logg.LOG_FROM_ID = 1;
    logg.LOG_LEVEL = 1;
    logg.msg = QStringLiteral("hahaha");
    g_log->log(logg);
}
