#ifndef JSON_HELPER_H
#define JSON_HELPER_H

#include <QJsonObject>
#include <QJsonArray>
#include <QJsonDocument>
#include <QString>
#include <QVariant>

class JsonHelper
{
public:
    // 创建标准响应
    static QJsonObject createResponse(bool success, const QString &message,
                                      const QJsonValue &data = QJsonValue());
    static QJsonObject createSuccessResponse(const QString &message = "",
                                              const QJsonValue &data = QJsonValue());
    static QJsonObject createErrorResponse(const QString &message);

    // 解析请求
    static QJsonObject parseJsonObject(const QByteArray &data);
    static QJsonArray parseJsonArray(const QByteArray &data);

    // 序列化
    static QByteArray toJsonBytes(const QJsonObject &obj);
    static QByteArray toJsonBytes(const QJsonArray &arr);
    static QString toJsonString(const QJsonObject &obj);

    // 安全取值
    static QString getString(const QJsonObject &obj, const QString &key,
                              const QString &defaultValue = QString());
    static int getInt(const QJsonObject &obj, const QString &key, int defaultValue = 0);
    static double getDouble(const QJsonObject &obj, const QString &key, double defaultValue = 0.0);
    static bool getBool(const QJsonObject &obj, const QString &key, bool defaultValue = false);
};

#endif // JSON_HELPER_H
