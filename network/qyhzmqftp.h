#ifndef QYHZMQFTP_H
#define QYHZMQFTP_H

#include <vector>
#include <thread>
#include <memory>
#include <functional>
#include <zmq.hpp>

#include <QObject>
#include <QMutex>

class QyhZmqFtp
{
public:
    QyhZmqFtp();
    //分成两个部分
    //1.上传文件接口:5566[采用request - reply模式]
    //2.下载文件接口:5567[采用publisher - subscriber模式]
    void init();

    void upload_process();
    void download_process();
private:
    zmq::context_t ctx_upload;
    zmq::socket_t s_upload;

    zmq::context_t cxt_download;
    zmq::socket_t s_download;

    //文件的数据
    QByteArray fileData;
    QMutex fileDataMutex;
    //文件的名称
    QString fileName;
    QMutex fileNameMutex;
};

#endif // QYHZMQFTP_H
