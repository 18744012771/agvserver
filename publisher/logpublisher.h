#ifndef LOGPUBLISHER_H
#define LOGPUBLISHER_H

#include <QObject>

class LogPublisher : public QObject
{
    Q_OBJECT
public:
    explicit LogPublisher(QObject *parent = nullptr);

signals:

public slots:
};

#endif // LOGPUBLISHER_H