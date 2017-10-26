#include "sqlserver.h"
#include <QDebug>
#include <QSqlError>
#include <QSqlRecord>
#include <QSqlQuery>

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
        qDebug() << "Error: Failed to connect database." << database.lastError();
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
bool SqlServer::exec(QString qeurysql,QStringList args)
{
    qDebug() << "qeurysql="<<qeurysql;
    QSqlQuery sql_query(database);
    sql_query.prepare(qeurysql);
    for(int i=0;i<args.length();++i){
        sql_query.addBindValue(args[i]);
        qDebug()<<args.at(i);
    }
    qDebug() << sql_query.lastQuery();
    if(!sql_query.exec())
    {
        qDebug() << "Error: Fail to sql_query.exec()." << sql_query.lastError();
        return false;
    }

    return true;
}

//查询数据
QList<QStringList> SqlServer::query(QString qeurysql, QStringList args)
{
    qDebug() << "qeurysql="<<qeurysql;
    QList<QStringList> xx;
    QSqlQuery sql_query(database);
    sql_query.prepare(qeurysql);
    for(int i=0;i<args.length();++i){
        //qDebug() <<args[i];
        sql_query.addBindValue(args[i]);
    }

    if(!sql_query.exec())
    {
        qDebug() << "Error: Fail to sql_query.exec()." << sql_query.lastError();
        return xx;
    }
    while(sql_query.next()){
        int columnNum=sql_query.record().count();
        QStringList qsl;
        for(int i=0;i<columnNum;++i)
            qsl.append(sql_query.value(i).toString());
        xx.append(qsl);
    }

    return xx;
}


