#ifndef USER_H
#define USER_H

#include <QString>
#include <QJsonObject>
#include "Constants.h"

class User
{
public:
    User();
    User(int id, const QString &username, const QString &password,
         Constants::UserRole role, const QString &realName);

    int id() const { return m_id; }
    void setId(int id) { m_id = id; }

    QString username() const { return m_username; }
    void setUsername(const QString &username) { m_username = username; }

    QString password() const { return m_password; }
    void setPassword(const QString &password) { m_password = password; }

    Constants::UserRole role() const { return m_role; }
    void setRole(Constants::UserRole role) { m_role = role; }
    QString roleString() const;

    QString realName() const { return m_realName; }
    void setRealName(const QString &realName) { m_realName = realName; }

    QJsonObject toJson() const;
    static User fromJson(const QJsonObject &obj);

private:
    int m_id;
    QString m_username;
    QString m_password;
    Constants::UserRole m_role;
    QString m_realName;
};

#endif // USER_H
