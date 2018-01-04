#ifndef SQLSERVER_H
#define SQLSERVER_H

#include <QList>
#include <QSqlDatabase>
#include <QMutex>
#include <QVariant>

class SqlServer
{
public:
    SqlServer();

    virtual ~SqlServer();
    //创建数据库连接
    bool createConnection();

    //检查表，如果表存在就OK，不存在创建表
    bool checkTables();

    //关闭数据库连接
    bool closeConnection();

    //执行sql语句
    //原则上，除了id 时间日期 外的其他字段统统text(vchar)
    bool exeSql(QString qeurysql,QList<QVariant> args);

    //查询数据
    QList<QList<QVariant>> query(QString qeurysql, QList<QVariant> args);

private:
    QSqlDatabase database;
    QMutex mutex;
};

#endif // SQLSERVER_H
