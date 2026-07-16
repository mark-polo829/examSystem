#include "HttpClient.h"
#include "JsonHelper.h"
#include <QUrl>
#include <QElapsedTimer>
#include <QThread>
#include <QCoreApplication>
#include <QEventLoop>

HttpClient::HttpClient(QObject *parent)
    : QObject(parent)
    // QNetworkAccessManager 生命周期需覆盖其 QNetworkReply
    // 使用 this 作为 parent 即可，HttpClient 对象存在期间 manager 有效
    , m_manager(new QNetworkAccessManager(this))
{
    connect(m_manager, &QNetworkAccessManager::finished,
            this, &HttpClient::onReplyFinished);
}

QJsonObject HttpClient::syncGet(const QString &url, int timeoutMs)
{
    QNetworkRequest request = createRequest(url);
    QNetworkReply *reply = m_manager->get(request);
    return waitForReply(reply, timeoutMs);
}

QJsonObject HttpClient::syncPost(const QString &url, const QJsonObject &data, int timeoutMs)
{
    QNetworkRequest request = createRequest(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    QNetworkReply *reply = m_manager->post(request, JsonHelper::toJsonBytes(data));
    return waitForReply(reply, timeoutMs);
}

QJsonObject HttpClient::syncPut(const QString &url, const QJsonObject &data, int timeoutMs)
{
    QNetworkRequest request = createRequest(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    QNetworkReply *reply = m_manager->put(request, JsonHelper::toJsonBytes(data));
    return waitForReply(reply, timeoutMs);
}

QJsonObject HttpClient::syncDelete(const QString &url, int timeoutMs)
{
    QNetworkRequest request = createRequest(url);
    QNetworkReply *reply = m_manager->deleteResource(request);
    return waitForReply(reply, timeoutMs);
}

void HttpClient::asyncGet(const QString &url)
{
    QNetworkRequest request = createRequest(url);
    m_manager->get(request);
}

void HttpClient::asyncPost(const QString &url, const QJsonObject &data)
{
    QNetworkRequest request = createRequest(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    m_manager->post(request, JsonHelper::toJsonBytes(data));
}

void HttpClient::asyncPut(const QString &url, const QJsonObject &data)
{
    QNetworkRequest request = createRequest(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    m_manager->put(request, JsonHelper::toJsonBytes(data));
}

void HttpClient::asyncDelete(const QString &url)
{
    QNetworkRequest request = createRequest(url);
    m_manager->deleteResource(request);
}

QNetworkRequest HttpClient::createRequest(const QString &url)
{
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    if (!m_authToken.isEmpty()) {
        request.setRawHeader("Authorization", "Bearer " + m_authToken.toUtf8());
    }
    return request;
}

QJsonObject HttpClient::waitForReply(QNetworkReply *reply, int timeoutMs)
{
    // 同步请求时临时断开异步 finished 槽，防止 onReplyFinished 先 readAll
    // 导致 waitForReply 这里读到空数据
    disconnect(m_manager, &QNetworkAccessManager::finished,
               this, &HttpClient::onReplyFinished);

    // 使用 QElapsedTimer + msleep 轮询，避免 QEventLoop 嵌套导致的问题
    QElapsedTimer timer;
    timer.start();

    while (!reply->isFinished() && timer.elapsed() < timeoutMs) {
        QCoreApplication::processEvents(QEventLoop::ExcludeUserInputEvents, 50);
        QThread::msleep(50);
    }

    QJsonObject response;
    if (!reply->isFinished()) {
        reply->abort();
        response = JsonHelper::createErrorResponse(QString::fromUtf8("请求超时"));
    } else {
        if (reply->error() == QNetworkReply::NoError) {
            QByteArray data = reply->readAll();
            response = JsonHelper::parseJsonObject(data);
            if (response.isEmpty()) {
                response = JsonHelper::createSuccessResponse(QString::fromUtf8("请求成功"));
                response["raw_data"] = QString::fromUtf8(data);
            }
        } else {
            response = JsonHelper::createErrorResponse(reply->errorString());
        }
    }

    reply->deleteLater();

    // 恢复异步 finished 槽连接
    connect(m_manager, &QNetworkAccessManager::finished,
            this, &HttpClient::onReplyFinished);

    return response;
}

void HttpClient::onReplyFinished(QNetworkReply *reply)
{
    if (reply->error() == QNetworkReply::NoError) {
        QByteArray data = reply->readAll();
        QJsonObject response = JsonHelper::parseJsonObject(data);
        if (response.isEmpty()) {
            response = JsonHelper::createSuccessResponse();
            response["raw_data"] = QString::fromUtf8(data);
        }
        emit requestFinished(response);
    } else {
        emit requestError(reply->errorString());
    }
    // 注意：同步请求中 waitForReply 已调用 deleteLater，这里不再重复调用
    // 仅当 reply 尚未被 deleteLater 时才调用（通过检查 reply 是否还在）
    // 但 deleteLater 不会立即销毁对象，所以不调用更安全
}
