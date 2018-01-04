#include "sqlserver.h"
#include <QSqlError>
#include <QSqlRecord>
#include <QSqlQuery>
#include "util/global.h"

SqlServer::SqlServer()
{

}

SqlServer::~SqlServer()
{
    closeConnection();
}

//创建数据库连接
bool SqlServer::createConnection()
{
    if (QSqlDatabase::contains("mysqlserverconnection"))
    {
        database = QSqlDatabase::database("mysqlserverconnection");
    }
    else
    {
        database=QSqlDatabase::addDatabase("QODBC","mysqlserverconnection");
        database.setDatabaseName("QyhTestDb");
        database.setUserName("sa");
        database.setPassword("6980103");
    }
    if(!database.isValid())return false;
    if (!database.open())
    {
        //g_log->log(AGV_LOG_LEVEL_ERROR,"Error: Failed to connect database."+database.lastError().text());
        return false;
    }

    return true;
}

//关闭数据库连接
bool SqlServer::closeConnection()
{
    database.close();
    return true;
}

//执行sql语句
bool SqlServer::exeSql(QString qeurysql, QList<QVariant> args)
{
    mutex.lock();
    QSqlQuery sql_query(database);
    sql_query.prepare(qeurysql);
    for(int i=0;i<args.length();++i){
        sql_query.addBindValue(args[i]);
    }

    if(!sql_query.exec())
    {
        //g_log->log(AGV_LOG_LEVEL_ERROR,"Error: Fail to sql_query.exec()."+sql_query.lastError().text());
        mutex.unlock();
        return false;
    }
    mutex.unlock();
    return true;
}

//查询数据
QList<QList<QVariant>> SqlServer::query(QString qeurysql, QList<QVariant> args)
{
    QList<QList<QVariant> > xx;
    if(qeurysql.contains("@@Identity")){
        QString insertSql = qeurysql.split(";").at(0);
        QString querySqlNew = qeurysql.split(";").at(1);

        mutex.lock();
        QSqlQuery sql_insert(database);
        sql_insert.prepare(insertSql);
        for(int i=0;i<args.length();++i){
            sql_insert.addBindValue(args[i]);
        }

        if(!sql_insert.exec())
        {
            qDebug() << "Error: Fail to sql_query.exec()."<<sql_insert.lastError();
            mutex.unlock();
            return xx;
        }

        QSqlQuery sql_query(database);
        sql_query.prepare(querySqlNew);

        if(!sql_query.exec())
        {
            qDebug() << "Error: Fail to sql_query.exec()."<<sql_query.lastError();
            mutex.unlock();
            return xx;
        }
        while(sql_query.next()){
            int columnNum=sql_query.record().count();
            QList<QVariant> qsl;
            for(int i=0;i<columnNum;++i)
                qsl.append(sql_query.value(i));
            xx.append(qsl);
        }
        mutex.unlock();
    }else{
        mutex.lock();
        QSqlQuery sql_query(database);
        sql_query.prepare(qeurysql);
        for(int i=0;i<args.length();++i){
            sql_query.addBindValue(args[i]);
        }
        if(!sql_query.exec())
        {
            qDebug() << "Error: Fail to sql_query.exec()."<<sql_query.lastError();
            mutex.unlock();
            return xx;
        }
        while(sql_query.next()){
            int columnNum=sql_query.record().count();
            QList<QVariant> qsl;
            for(int i=0;i<columnNum;++i)
                qsl.append(sql_query.value(i));
            xx.append(qsl);
        }
        mutex.unlock();
    }
    return xx;
}


