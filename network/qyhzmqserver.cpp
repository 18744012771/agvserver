#include "qyhzmqserver.h"
#include "qyhzmqserverworker.h"

QyhZmqServer::QyhZmqServer():
    ctx_(1),
    frontend_(ctx_, ZMQ_ROUTER),
    backend_(ctx_, ZMQ_DEALER)
{

}

void QyhZmqServer::run()
{
    frontend_.bind("tcp://*:5555");
    backend_.bind("inproc://workers");

    std::vector<QyhZmqServerWorker *> worker;
    std::vector<std::thread *> worker_thread;

    for (int i = 0; i < MAX_THREAD; ++i)
    {
        //一个新的工人
        worker.push_back(new QyhZmqServerWorker((void *)&ctx_));

        //启动一个线程，执行这个工人的 work函数
        worker_thread.push_back(new std::thread(std::bind(&QyhZmqServerWorker::work, worker[i])));
        worker_thread[i]->detach();
    }

    //执行代理操作
    try {
        zmq::proxy(frontend_, backend_, nullptr);
    }
    catch (std::exception &e) {}

    for (int i = 0; i < MAX_THREAD; ++i) {
        delete worker[i];
        delete worker_thread[i];
    }
}

