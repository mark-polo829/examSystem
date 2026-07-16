#ifndef HTTP_CLIENT_H
#define HTTP_CLIENT_H

#include <QObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QJsonObject>
#include <QJsonDocument>
#include <QElapsedTimer>
#include <QThread>

class HttpClient : public QObject
{
    Q_OBJECT
public:
    explicit HttpClient(QObject *parent = nullptr);

    // 同步请求（阻塞式，带超时）
    QJsonObject syncGet(const QString &url, int timeoutMs = 10000);
    QJsonObject syncPost(const QString &url, const QJsonObject &data, int timeoutMs = 10000);
    QJsonObject syncPut(const QString &url, const QJsonObject &data, int timeoutMs = 10000);
    QJsonObject syncDelete(const QString &url, int timeoutMs = 10000);

    // 异步请求
    void asyncGet(const QString &url);
    void asyncPost(const QString &url, const QJsonObject &data);
    void asyncPut(const QString &url, const QJsonObject &data);
    void asyncDelete(const QString &url);

    // 设置认证令牌
    void setAuthToken(const QString &token) { m_authToken = token; }
    void clearAuthToken() { m_authToken.clear(); }

signals:
    void requestFinished(const QJsonObject &response);
    void requestError(const QString &error);

private slots:
    void onReplyFinished(QNetworkReply *reply);

private:
    QNetworkRequest createRequest(const QString &url);
    QJsonObject waitForReply(QNetworkReply *reply, int timeoutMs);

    QNetworkAccessManager *m_manager;
    QString m_authToken;
};

#endif // HTTP_CLIENT_H
