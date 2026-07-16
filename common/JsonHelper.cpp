#include "JsonHelper.h"

QJsonObject JsonHelper::createResponse(bool success, const QString &message,
                                        const QJsonValue &data)
{
    QJsonObject obj;
    obj["success"] = success;
    obj["message"] = message;
    if (!data.isNull() && !data.isUndefined()) {
        obj["data"] = data;
    }
    return obj;
}

QJsonObject JsonHelper::createSuccessResponse(const QString &message,
                                               const QJsonValue &data)
{
    return createResponse(true, message, data);
}

QJsonObject JsonHelper::createErrorResponse(const QString &message)
{
    return createResponse(false, message);
}

QJsonObject JsonHelper::parseJsonObject(const QByteArray &data)
{
    QJsonDocument doc = QJsonDocument::fromJson(data);
    if (doc.isNull() || !doc.isObject()) {
        return QJsonObject();
    }
    return doc.object();
}

QJsonArray JsonHelper::parseJsonArray(const QByteArray &data)
{
    QJsonDocument doc = QJsonDocument::fromJson(data);
    if (doc.isNull() || !doc.isArray()) {
        return QJsonArray();
    }
    return doc.array();
}

QByteArray JsonHelper::toJsonBytes(const QJsonObject &obj)
{
    QJsonDocument doc(obj);
    return doc.toJson(QJsonDocument::Compact);
}

QByteArray JsonHelper::toJsonBytes(const QJsonArray &arr)
{
    QJsonDocument doc(arr);
    return doc.toJson(QJsonDocument::Compact);
}

QString JsonHelper::toJsonString(const QJsonObject &obj)
{
    return QString::fromUtf8(toJsonBytes(obj));
}

QString JsonHelper::getString(const QJsonObject &obj, const QString &key,
                               const QString &defaultValue)
{
    if (obj.contains(key) && obj[key].isString()) {
        return obj[key].toString();
    }
    return defaultValue;
}

int JsonHelper::getInt(const QJsonObject &obj, const QString &key, int defaultValue)
{
    if (obj.contains(key) && obj[key].isDouble()) {
        return obj[key].toInt();
    }
    return defaultValue;
}

double JsonHelper::getDouble(const QJsonObject &obj, const QString &key, double defaultValue)
{
    if (obj.contains(key) && obj[key].isDouble()) {
        return obj[key].toDouble();
    }
    return defaultValue;
}

bool JsonHelper::getBool(const QJsonObject &obj, const QString &key, bool defaultValue)
{
    if (obj.contains(key) && obj[key].isBool()) {
        return obj[key].toBool();
    }
    return defaultValue;
}
