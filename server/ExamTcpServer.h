#ifndef EXAM_TCP_SERVER_H
#define EXAM_TCP_SERVER_H

#include <QTcpServer>
#include <QThreadPool>

class ExamTcpServer : public QTcpServer
{
    Q_OBJECT
public:
    explicit ExamTcpServer(QThreadPool *threadPool, QObject *parent = nullptr);

protected:
    void incomingConnection(qintptr handle) override;

private:
    QThreadPool *m_threadPool;
};

#endif // EXAM_TCP_SERVER_H
