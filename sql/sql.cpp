#include "sql.h"
#include <QDebug>
#include <QSqlError>
#include <QSqlRecord>
#include <QSqlQuery>

Sql::Sql()
{

}

Sql::~Sql()
{
    closeConnection();
}

bool Sql::checkTables()
{
    QString querySql = "SELECT COUNT(*) FROM sqlite_master where type='table' and name=?";
    QStringList args;
    //检查表如下：
    /// 1.agv_stations
    /// 2.agv_lines
    /// 3.agv_lmr
    /// 4.agv_adj
    /// 5.agv_log
    /// 6.agv_user
    /// 7.agv_agv

    args.clear();
    args<<"agv_station";
    QList<QStringList> qsl = query(querySql,args);
    if(qsl.length()==1&&qsl[0].length()==1&&qsl[0][0]=="1"){
        //存在了
    }else{
        //不存在.创建
        QString createSql = "create table agv_station ( id INTEGER PRIMARY KEY AUTOINCREMENT, mykey INTEGER unique, x INTEGER, y INTEGER, type INTEGER, name text,lineAmount INTEGER,lines text,rfid INTEGER);";
        args.clear();
        bool b = exec(createSql,args);
        if(!b)return false;
    }

    args.clear();
    args<<"agv_line";
    qsl = query(querySql,args);
    if(qsl.length()==1&&qsl[0].length()==1&&qsl[0][0]=="1"){
        //存在了
    }else{
        //不存在.创建
        QString createSql = "create table agv_line (id INTEGER PRIMARY KEY AUTOINCREMENT,mykey INTEGER unique,startX INTEGER,startY INTEGER,endX INTEGER,endY INTEGER,radius INTEGER,name text,clockwise BOOLEAN,line BOOLEAN,midX INTEGER,midY INTEGER,centerX INTEGER,centerY INTEGER,angle INTEGER,length INTEGER,startStation INTEGER,endStation INTEGER, draw BOOLEAN);";
        args.clear();
        bool b = exec(createSql,args);
        if(!b)return false;
    }

    args.clear();
    args<<"agv_lmr";
    qsl = query(querySql,args);
    if(qsl.length()==1&&qsl[0].length()==1&&qsl[0][0]=="1"){
        //存在了
    }else{
        //不存在.创建
        QString createSql = "create table agv_lmr (id INTEGER PRIMARY KEY AUTOINCREMENT,lastLine INTEGER,nextLine INTEGER,lmr INTEGER);";
        args.clear();
        bool b = exec(createSql,args);
        if(!b)return false;
    }

    args.clear();
    args<<"agv_adj";
    qsl = query(querySql,args);
    if(qsl.length()==1&&qsl[0].length()==1&&qsl[0][0]=="1"){
        //存在了
    }else{
        //不存在.创建
        QString createSql = "create table agv_adj (id INTEGER PRIMARY KEY AUTOINCREMENT,myKey INTEGER,lines text);";
        args.clear();
        bool b = exec(createSql,args);
        if(!b)return false;
    }


    args.clear();
    args<<"agv_log";
    qsl = query(querySql,args);
    if(qsl.length()==1&&qsl[0].length()==1&&qsl[0][0]=="1"){
        //存在了
    }else{
        //不存在.创建
        QString createSql = "create table agv_log (id INTEGER PRIMARY KEY AUTOINCREMENT,type text,from_id text,level text,msg text,date datetime);";
        args.clear();
        bool b = exec(createSql,args);
        if(!b)return false;
    }

    args.clear();
    args<<"agv_task";
    qsl = query(querySql,args);
    if(qsl.length()==1&&qsl[0].length()==1&&qsl[0][0]=="1"){
        //存在了
    }else{
        //不存在.创建
        QString createSql = "create table agv_task ( id INTEGER PRIMARY KEY AUTOINCREMENT, mykey INTEGER unique, aimStation INTEGER,type INTEGER,pickupStation INTEGER,produceTime datetime,doTime dateTime,doneTime datetime,excuteCar INTEGER,status INTEGER,goPickGoAim bool,waitTypePick INTEGER,waitTimePick INTEGER,waitTypeAim INTEGER,waitTimeAim INTEGER);";
        args.clear();
        bool b = exec(createSql,args);
        if(!b)return false;
    }


    args.clear();
    args<<"agv_user";
    qsl = query(querySql,args);
    if(qsl.length()==1&&qsl[0].length()==1&&qsl[0][0]=="1"){
        //存在了
    }else{
        //不存在.创建
        QString createSql = "CREATE TABLE agv_user(id INTEGER PRIMARY KEY AUTOINCREMENT,name text,pwd text,realName TEXT,lastSignTime datetime,signState integer,tickeId text,sex bool,age int,createTime DATETIME,role INTEGER);";
        args.clear();
        bool b = exec(createSql,args);
        if(!b)return false;
    }

    args.clear();
    args<<"agv_agv";
    qsl = query(querySql,args);
    if(qsl.length()==1&&qsl[0].length()==1&&qsl[0][0]=="1"){
        //存在了
    }else{
        //不存在.创建
        QString createSql = "CREATE TABLE agv_agv(id INTEGER PRIMARY KEY AUTOINCREMENT,name text,ip text);";
        args.clear();
        bool b = exec(createSql,args);
        if(!b)return false;
    }

    return true;
}



//创建数据库连接
bool Sql::createConnection()
{
    if (QSqlDatabase::contains("mysqliteconnection"))
    {
        database = QSqlDatabase::database("mysqliteconnection");
    }
    else
    {
        database = QSqlDatabase::addDatabase("QSQLITE","mysqliteconnection");
        database.setDatabaseName("database/agv_manager.db");
        database.setUserName("XingYeZhiXia");
        database.setPassword("123456");
    }

    if(!database.isValid())return false;

    if (!database.open())
    {
        qDebug() << "Error: Failed to connect database." << database.lastError();
        return false;
    }
    return checkTables();
}

//关闭数据库连接
bool Sql::closeConnection()
{
    database.close();
    return true;
}

//执行sql语句
bool Sql::exec(QString qeurysql,QStringList args)
{
    qDebug() << "qeurysql="<<qeurysql;
    QSqlQuery sql_query(database);
    sql_query.prepare(qeurysql);
    for(int i=0;i<args.length();++i){
        sql_query.addBindValue(args[i]);
        qDebug()<<args.at(i);
    }
    //qDebug() << sql_query.lastQuery();
    if(!sql_query.exec())
    {
        qDebug() << "Error: Fail to sql_query.exec()." << sql_query.lastError();
        return false;
    }

    return true;
}

//查询数据
QList<QStringList> Sql::query(QString qeurysql, QStringList args)
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


