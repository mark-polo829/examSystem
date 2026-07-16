#include "ExamTcpServer.h"
#include "RequestHandler.h"

ExamTcpServer::ExamTcpServer(QThreadPool *threadPool, QObject *parent)
    : QTcpServer(parent), m_threadPool(threadPool)
{
}

void ExamTcpServer::incomingConnection(qintptr handle)
{
    // 直接把原生 socket 描述符交给工作线程，由工作线程创建 QTcpSocket，
    // 彻底避免跨线程操作 Qt socket 对象。
    if (m_threadPool) {
        RequestHandler *handler = new RequestHandler(handle);
        m_threadPool->start(handler);
    }
}
