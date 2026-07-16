#ifndef REQUEST_HANDLER_H
#define REQUEST_HANDLER_H

#include <QRunnable>
#include <QTcpSocket>
#include <QJsonObject>
#include <QHash>
#include "Constants.h"

// HTTP请求结构
struct HttpRequest {
    QString method;
    QString path;
    QString version;
    QHash<QString, QString> headers;
    QByteArray body;
    QHash<QString, QString> queryParams;
    QString bodyString;
};

// HTTP响应结构
struct HttpResponse {
    int statusCode;
    QString statusText;
    QHash<QString, QString> headers;
    QByteArray body;

    HttpResponse(int code = 200, const QString &text = "OK")
        : statusCode(code), statusText(text) {}
};

class RequestHandler : public QRunnable
{
public:
    // 传入原生 socket 描述符，由工作线程创建 QTcpSocket，避免跨线程对象问题
    explicit RequestHandler(qintptr socketDescriptor);
    ~RequestHandler();

    void run() override;

private:
    qintptr m_socketDescriptor;
    QTcpSocket *m_socket;

    HttpRequest parseRequest(const QByteArray &data);
    void sendResponse(const HttpResponse &response);

    // 路由处理
    HttpResponse handleRequest(const HttpRequest &request);

    // 各API处理
    HttpResponse handleLogin(const HttpRequest &request);
    HttpResponse handleGetRandomQuestions(const HttpRequest &request);
    HttpResponse handleSubmitExam(const HttpRequest &request);
    HttpResponse handleGetExamResult(const HttpRequest &request);
    HttpResponse handleGetUserExamRecords(const HttpRequest &request);
    HttpResponse handleGetQuestions(const HttpRequest &request);
    HttpResponse handleAddQuestion(const HttpRequest &request);
    HttpResponse handleUpdateQuestion(const HttpRequest &request, int id);
    HttpResponse handleDeleteQuestion(const HttpRequest &request, int id);
    HttpResponse handleGetScoreStatistics(const HttpRequest &request);
    HttpResponse handleGetUsers(const HttpRequest &request);
    HttpResponse handleAddUser(const HttpRequest &request);
    HttpResponse handleUpdateUser(const HttpRequest &request, int id);
    HttpResponse handleDeleteUser(const HttpRequest &request, int id);
    HttpResponse handleGetCategories(const HttpRequest &request);

    // 辅助方法
    QJsonObject parseBody(const HttpRequest &request);
    QString getQueryParam(const HttpRequest &request, const QString &key);
    bool checkAuth(const HttpRequest &request, Constants::UserRole minRole = Constants::RoleStudent);
    int getUserIdFromToken(const HttpRequest &request);
};

#endif // REQUEST_HANDLER_H
