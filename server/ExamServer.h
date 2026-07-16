#ifndef EXAM_SERVER_H
#define EXAM_SERVER_H

#include <QObject>
#include <QThreadPool>
#include <QHash>
#include "DatabaseManager.h"

class ExamTcpServer;
class HttpRequest;
class HttpResponse;

class ExamServer : public QObject
{
    Q_OBJECT
public:
    explicit ExamServer(QObject *parent = nullptr);
    ~ExamServer();

    bool start(quint16 port = 8080);
    void stop();

    QThreadPool* threadPool() { return &m_threadPool; }

private:
    ExamTcpServer *m_server;
    QThreadPool m_threadPool;
    bool m_running;
};

#endif // EXAM_SERVER_H
