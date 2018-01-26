#include "sql.h"
#include <QSqlError>
#include <QSqlRecord>
#include <QSqlQuery>
#include "util/global.h"

Sql::Sql()
{

}

Sql::~Sql()
{
    closeConnection();
}

bool Sql::checkTables()
{
    //mysql
    QString querySql = "select count(*) from INFORMATION_SCHEMA.TABLES where TABLE_NAME=? ;";
    //sqlite
    //QString querySql = "SELECT COUNT(*) FROM sqlite_master where type='table' and name=?";

    QList<QVariant> args;
    //检查表如下：
    /// 1.agv_stations
    /// 2.agv_line
    /// 3.agv_lmr
    /// 4.agv_adj
    /// 5.agv_log
    /// 6.agv_user
    /// 7.agv_agv
    /// 8.agv_task
    /// 9.agv_bkg

    args.clear();
    args<<"agv_station";
    QList<QList<QVariant> >  qsl = query(querySql,args);
    if(qsl.length()==1&&qsl[0].length()==1&&qsl[0][0].toString()=="1"){
        //存在了
    }else{
        //不存在.创建
        //MYSQL:

        QString createSql = "create table agv_station (id INTEGER PRIMARY KEY AUTO_INCREMENT, station_x INTEGER, station_y INTEGER, station_name text,station_rfid INTEGER,station_color_r INTEGER,station_color_g INTEGER,station_color_b INTEGER);";
        args.clear();
        bool b = exeSql(createSql,args);
        if(!b)return false;
    }

    args.clear();
    args<<"agv_line";
    qsl = query(querySql,args);
    if(qsl.length()==1&&qsl[0].length()==1&&qsl[0][0]=="1"){
        //存在了
    }else{
        //不存在.创建
        QString createSql = "create table agv_line (id INTEGER PRIMARY KEY AUTO_INCREMENT,line_startStation INTEGER,line_endStation INTEGER,line_line BOOLEAN,line_length DOUBLE, line_draw BOOLEAN,line_rate DOUBLE,line_color_r INTEGER,line_color_g INTEGER,line_color_b INTEGER,line_p1x DOUBLE,line_p1y DOUBLE,line_p2x DOUBLE,line_p2y DOUBLE);";
        args.clear();
        bool b = exeSql(createSql,args);
        if(!b)return false;
    }

    args.clear();
    args<<"agv_lmr";
    qsl = query(querySql,args);
    if(qsl.length()==1&&qsl[0].length()==1&&qsl[0][0]=="1"){
        //存在了
    }else{
        //不存在.创建
        QString createSql = "create table agv_lmr (id INTEGER PRIMARY KEY AUTO_INCREMENT,lmr_lastLine INTEGER,lmr_nextLine INTEGER,lmr_lmr INTEGER);";
        args.clear();
        bool b = exeSql(createSql,args);
        if(!b)return false;
    }

    args.clear();
    args<<"agv_adj";
    qsl = query(querySql,args);
    if(qsl.length()==1&&qsl[0].length()==1&&qsl[0][0]=="1"){
        //存在了
    }else{
        //不存在.创建
        QString createSql = "create table agv_adj (id INTEGER PRIMARY KEY AUTO_INCREMENT,adj_startLine INTEGER,adj_endLine INTEGER);";
        args.clear();
        bool b = exeSql(createSql,args);
        if(!b)return false;
    }


    args.clear();
    args<<"agv_log";
    qsl = query(querySql,args);
    if(qsl.length()==1&&qsl[0].length()==1&&qsl[0][0]=="1"){
        //存在了
    }else{
        //不存在.创建
        QString createSql = "create table agv_log (id INTEGER PRIMARY KEY AUTO_INCREMENT,log_level INTEGER,log_msg text,log_time datetime);";
        args.clear();
        bool b = exeSql(createSql,args);
        if(!b)return false;
    }

    args.clear();
    args<<"agv_user";
    qsl = query(querySql,args);
    if(qsl.length()==1&&qsl[0].length()==1&&qsl[0][0]=="1"){
        //存在了
    }else{
        //不存在.创建
        QString createSql = "CREATE TABLE agv_user(id INTEGER PRIMARY KEY AUTO_INCREMENT,user_username text,user_password text,user_realname TEXT,user_lastSignTime datetime,user_signState integer,user_sex bool,user_age int,user_createTime datetime,user_role INTEGER);";
        args.clear();
        bool b = exeSql(createSql,args);
        if(!b)return false;
    }

    args.clear();
    args<<"agv_agv";
    qsl = query(querySql,args);
    if(qsl.length()==1&&qsl[0].length()==1&&qsl[0][0]=="1"){
        //存在了
    }else{
        //不存在.创建
        QString createSql = "CREATE TABLE agv_agv(id INTEGER PRIMARY KEY AUTO_INCREMENT,agv_name text,agv_ip text,agv_port int);";
        args.clear();
        bool b = exeSql(createSql,args);
        if(!b)return false;
    }

    args.clear();
    args<<"agv_task";
    qsl = query(querySql,args);
    if(qsl.length()==1&&qsl[0].length()==1&&qsl[0][0].toInt()>0){
        //存在了
    }else{
        //不存在.创建
        QString createSql = "create table agv_task ( id INTEGER PRIMARY KEY AUTO_INCREMENT, task_produceTime datetime,task_doTime datetime,task_doneTime datetime,task_excuteCar integer,task_status integer,task_circle bool,task_priority integer,task_currentDoIndex integer,"
                            "task_getGoodStation integer,task_getGoodDirect integer,task_getGoodDistance integer,task_getStartTime datetime,task_getFinishTime datetime,"
                            "task_putGoodStation integer,task_putGoodDirect integer,task_putGoodDistance integer,task_putStartTime datetime,task_putFinishTime datetime,"
                            "task_standByStation integer,task_standByStartTime datetime,task_standByFinishTime datetime);";
        args.clear();
        bool b = exeSql(createSql,args);
        if(!b)return false;
    }

    args.clear();
    args<<"agv_bkg";
    qsl = query(querySql,args);
    if(qsl.length()==1&&qsl[0].length()==1&&qsl[0][0]=="1"){
        //存在了
    }else{
        //不存在.创建
        QString createSql = "CREATE TABLE agv_bkg (id INTEGER PRIMARY KEY auto_increment,bkg_name text,bkg_upload_time datetime,bkg_upload_user int,bkg_data mediumblob);";
        args.clear();
        bool b = exeSql(createSql,args);
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
        database = QSqlDatabase::addDatabase("QMYSQL","mysqliteconnection");
        database.setHostName("localhost");
        database.setDatabaseName("agv");
        database.setPort(3306);

        database.setUserName("root");
        database.setPassword("123456");
    }

    if(!database.isValid())return false;

    if (!database.open())
    {
        g_log->log(AGV_LOG_LEVEL_ERROR,QString("Error: Failed to connect database.") +database.lastError().text());
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
bool Sql::exeSql(QString esql, QList<QVariant> args)
{
    if(!esql.contains("agv_log") && !esql.contains("insert into"))//防止形成自循环
        g_log->log(AGV_LOG_LEVEL_DEBUG,"exeSql="+esql);
    mutex.lock();
    QSqlQuery sql_query(database);
    sql_query.prepare(esql);
    for(int i=0;i<args.length();++i){
        sql_query.addBindValue(args[i]);
    }

    if(!sql_query.exec())
    {
        qDebug() << "Error: Fail to sql_query.exec()."<<sql_query.lastError();
        mutex.unlock();
        return false;
    }
    mutex.unlock();

    return true;
}

//查询数据
QList<QList<QVariant> > Sql::query(QString qeurysql, QList<QVariant> args)
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


