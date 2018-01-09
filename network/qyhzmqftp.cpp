#include "qyhzmqftp.h"
#include "util/global.h"

QyhZmqFtp::QyhZmqFtp():
    ctx_upload(1),
    s_upload(ctx_upload,ZMQ_REP),
    cxt_download(1),
    s_download(cxt_download, ZMQ_REP),
    fileName("")
{

}

void QyhZmqFtp::init()
{
    //从数据库载入ID最大的文件的 数据
    QString querySql = "select a.bkg_name,a.bkg_data from agv_bkg a where a.id in(select max(b.id) from agv_bkg b where b.id=a.id);";
    QList<QVariant> params;
    QList<QList<QVariant> > result = g_sql->query(querySql,params);
    if(result.length()>0){
        fileName = result.at(0).at(0).toString();
        fileData = result.at(0).at(1).toByteArray();
    }

    std::thread(std::bind(&QyhZmqFtp::upload_process, this)).detach();
    std::thread(std::bind(&QyhZmqFtp::download_process, this)).detach();
}

//文件上载
//请求： filename "" data
//回应:  OK
void QyhZmqFtp::upload_process()
{
    s_upload.bind("tcp://*:5567");
    while(true)
    {
        zmq::message_t msg_filename;
        s_upload.recv(&msg_filename);
        zmq::message_t msg_null;
        s_upload.recv(&msg_null);
        zmq::message_t msg_data;
        s_upload.recv(&msg_data);

        zmq::message_t msg_reply(2);
        if(msg_filename.size()>0 && msg_null.size()==0 &&msg_data.size()>0)
        {

            QString insertSql = "insert into agv_bkg(bkg_name,bkg_data,bkg_upload_time) values(?,?,?)";
            QList<QVariant> params;
            QByteArray qba((char *)msg_data.data(),msg_data.size());
            QString name = QString::fromStdString(std::string((char *)msg_filename.data(),msg_filename.size()));
            params<<QString::fromStdString(std::string((char *)msg_filename.data(),msg_filename.size()))
                 <<qba
                <<QDateTime::currentDateTime();

            if(g_sql->exeSql(insertSql,params)){
                //执行保存数据库
                fileNameMutex.lock();
                fileName = name;
                fileNameMutex.unlock();
                fileDataMutex.lock();
                fileData = qba;
                fileDataMutex.unlock();
            }

            memcpy(msg_reply.data(),"OK",2);
            s_upload.send(msg_reply);
        }else{
            memcpy(msg_reply.data(),"NO",2);
            s_upload.send(msg_reply);
        }
    }
}

//文件下载
//请求： download
//回应:  filename "" data
void QyhZmqFtp::download_process()
{
    s_download.bind ("tcp://*:5566");
    while(true)
    {
        zmq::message_t msg_request;
        s_download.recv(&msg_request);

        if(std::string((char *)msg_request.data(),msg_request.size()) == "download")
        {
            fileNameMutex.lock();
            QString filename_ = fileName;
            fileNameMutex.unlock();
            fileDataMutex.lock();
            QByteArray qba = fileData;
            fileDataMutex.unlock();
            s_download.send(filename_.data(),filename_.size(),ZMQ_SNDMORE);
            s_download.send("",0,ZMQ_SNDMORE);
            s_download.send(qba.data(),qba.length());
        }
    }
}


