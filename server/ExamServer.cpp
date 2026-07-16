#include "ExamServer.h"
#include "ExamTcpServer.h"
#include "RequestHandler.h"
#include <QTcpSocket>
#include <QDir>
#include <QDebug>

// 原生 socket 头，用于在 bind 之前设置 SO_REUSEADDR 实现端口复用
#ifdef Q_OS_WIN
    #include <winsock2.h>
    #include <ws2tcpip.h>
#else
    #include <sys/socket.h>
    #include <netinet/in.h>
    #include <arpa/inet.h>
    #include <unistd.h>
    #include <errno.h>
    #include <string.h>
#endif

// 创建一个已设置 SO_REUSEADDR 并完成 bind/listen 的原生 socket 描述符。
// 失败返回 -1，并通过 errorString 输出错误信息。
static qintptr createReusableServerSocket(quint16 port, QString *errorString = nullptr)
{
#ifdef Q_OS_WIN
    SOCKET sockfd = ::socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == INVALID_SOCKET) {
        if (errorString) {
            *errorString = QString("Failed to create socket, error: %1")
                           .arg(WSAGetLastError());
        }
        return -1;
    }

    int reuse = 1;
    if (::setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR,
                     (const char *)&reuse, sizeof(reuse)) != 0) {
        if (errorString) {
            *errorString = QString("Failed to set SO_REUSEADDR, error: %1")
                           .arg(WSAGetLastError());
        }
        ::closesocket(sockfd);
        return -1;
    }
#else
    int sockfd = ::socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        if (errorString) {
            *errorString = QString("Failed to create socket: %1")
                           .arg(strerror(errno));
        }
        return -1;
    }

    int reuse = 1;
    if (::setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR,
                     &reuse, sizeof(reuse)) != 0) {
        if (errorString) {
            *errorString = QString("Failed to set SO_REUSEADDR: %1")
                           .arg(strerror(errno));
        }
        ::close(sockfd);
        return -1;
    }

    // Linux/BSD 可额外设置 SO_REUSEPORT，允许同一端口多个 socket 负载均衡
#ifdef SO_REUSEPORT
    int reuseport = 1;
    ::setsockopt(sockfd, SOL_SOCKET, SO_REUSEPORT,
                 &reuseport, sizeof(reuseport));
#endif
#endif

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = INADDR_ANY;

    if (::bind(sockfd, (struct sockaddr *)&addr, sizeof(addr)) != 0) {
#ifdef Q_OS_WIN
        if (errorString) {
            *errorString = QString("Failed to bind to port %1, error: %2")
                           .arg(port).arg(WSAGetLastError());
        }
        ::closesocket(sockfd);
#else
        if (errorString) {
            *errorString = QString("Failed to bind to port %1: %2")
                           .arg(port).arg(strerror(errno));
        }
        ::close(sockfd);
#endif
        return -1;
    }

    if (::listen(sockfd, 128) != 0) {
#ifdef Q_OS_WIN
        if (errorString) {
            *errorString = QString("Failed to listen, error: %1")
                           .arg(WSAGetLastError());
        }
        ::closesocket(sockfd);
#else
        if (errorString) {
            *errorString = QString("Failed to listen: %1")
                           .arg(strerror(errno));
        }
        ::close(sockfd);
#endif
        return -1;
    }

    return static_cast<qintptr>(sockfd);
}

ExamServer::ExamServer(QObject *parent)
    : QObject(parent), m_server(new ExamTcpServer(&m_threadPool, this)), m_running(false)
{
    // 设置线程池最大线程数
    m_threadPool.setMaxThreadCount(QThread::idealThreadCount() * 2);
}

ExamServer::~ExamServer()
{
    stop();
}

bool ExamServer::start(quint16 port)
{
    if (m_running) {
        return true;
    }

    // 初始化数据库
    QString dbPath = QDir::currentPath() + "/exam.db";
    if (!DatabaseManager::instance()->initialize(dbPath)) {
        qDebug() << "Failed to initialize database";
        return false;
    }

    // 关闭可能残留的旧 socket
    m_server->close();

    // Windows 下使用原生 Winsock API 前必须先初始化
#ifdef Q_OS_WIN
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        qDebug() << "Failed to initialize Winsock";
        return false;
    }
#endif

    // 使用原生 socket 并在 bind 前设置 SO_REUSEADDR，
    // 解决程序异常退出后端口仍处于 TIME_WAIT 导致无法启动的问题
    QString errorString;
    qintptr sockfd = createReusableServerSocket(port, &errorString);
    if (sockfd < 0) {
        qDebug() << errorString;
#ifdef Q_OS_WIN
        WSACleanup();
#endif
        return false;
    }

    // 将已绑定/监听的 socket 交给 QTcpServer 管理
    if (!m_server->setSocketDescriptor(sockfd)) {
        qDebug() << "Failed to set socket descriptor:"
                 << m_server->errorString();
#ifdef Q_OS_WIN
        ::closesocket(static_cast<SOCKET>(sockfd));
        WSACleanup();
#else
        ::close(static_cast<int>(sockfd));
#endif
        return false;
    }

    m_running = true;
    qDebug() << "Server started on port" << port;

    return true;
}

void ExamServer::stop()
{
    if (!m_running) {
        return;
    }

    m_running = false;
    m_server->close();
    m_threadPool.waitForDone();
    qDebug() << "Server stopped";

#ifdef Q_OS_WIN
    WSACleanup();
#endif
}
