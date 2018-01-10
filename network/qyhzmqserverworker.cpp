#include "qyhzmqserverworker.h"
#include "util/global.h"

QyhZmqServerWorker::QyhZmqServerWorker(void *ctx):
    ctx_((zmq::context_t *)ctx),
    worker_(*ctx_, ZMQ_REP)
{

}

void QyhZmqServerWorker::work()
{
    worker_.connect("inproc://workers");

    while (true) {
        zmq::message_t msg;
        zmq::message_t copied_msg;
        try {
            worker_.recv(&msg);

            //处理结尾没有\0的问题
            std::string s = std::string((char *)msg.data(),msg.size());
            printf("recv:\n%s\n",s.c_str());

            std::string rep = userMsgProcessor->parseOneMsg(ctx_,s);

            copied_msg = zmq::message_t(rep.data(),rep.length());
            worker_.send(copied_msg);
        }
        catch (std::exception &e) {}
    }
}
