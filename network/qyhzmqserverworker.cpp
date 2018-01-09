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

    try {
        while (true) {
            zmq::message_t msg;
            zmq::message_t copied_msg;
            worker_.recv(&msg);
            std::string s = std::string((char *)msg.data(),msg.size());
            userMsgProcessor->parseOneMsg(ctx_,s);

            //TODO:
            //处理结尾没有\0的问题
            printf("recv:%s\n",std::string((char*)msg.data(),msg.size()).c_str());
            copied_msg.copy(&msg);
            worker_.send(copied_msg);
        }
    }
    catch (std::exception &e) {}
}
