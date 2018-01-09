#ifndef QYHZMQSERVERWORKER_H
#define QYHZMQSERVERWORKER_H

#include <zmq.hpp>

class QyhZmqServerWorker
{
public:
    QyhZmqServerWorker(void *ctx);
    void work();
private:
    zmq::context_t *ctx_;
    zmq::socket_t worker_;
};

#endif // QYHZMQSERVERWORKER_H
