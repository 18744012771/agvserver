#ifndef QYHZMQSERVER_H
#define QYHZMQSERVER_H

#include <vector>
#include <thread>
#include <memory>
#include <functional>

#include <zmq.hpp>


class QyhZmqServer
{
public:
    QyhZmqServer();

    //线程数量
    enum{ MAX_THREAD = 20 };

    //线程中初始化的入口
    void run();
private:
    zmq::context_t ctx_;
    zmq::socket_t frontend_;
    zmq::socket_t backend_;
};

#endif // QYHZMQSERVER_H
