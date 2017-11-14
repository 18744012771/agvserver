#ifndef SQL_H
#define SQL_H

#include <QList>
#include <QSqlDatabase>
#include <QMutex>

class Sql
{
public:
    Sql();

    virtual ~Sql();

    //创建数据库连接
    bool createConnection();

    //检查表，如果表存在就OK，不存在创建表
    bool checkTables();

    //关闭数据库连接
    bool closeConnection();

    //执行sql语句
    //原则上，除了id 时间日期 外的其他字段统统text(vchar)
    bool exeSql(QString exeSql, QStringList args);

    //查询数据
    QList<QStringList> query(QString qeurysql, QStringList args);

private:
    QSqlDatabase database;
    QMutex mutex;
};

#endif // SQL_H
