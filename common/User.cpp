#include "User.h"

User::User()
    : m_id(0), m_role(Constants::RoleStudent)
{
}

User::User(int id, const QString &username, const QString &password,
           Constants::UserRole role, const QString &realName)
    : m_id(id), m_username(username), m_password(password),
      m_role(role), m_realName(realName)
{
}

QString User::roleString() const
{
    switch (m_role) {
    case Constants::RoleStudent: return QString::fromUtf8("考生");
    case Constants::RoleTeacher: return QString::fromUtf8("教师");
    case Constants::RoleAdmin: return QString::fromUtf8("管理员");
    default: return QString::fromUtf8("未知");
    }
}

QJsonObject User::toJson() const
{
    QJsonObject obj;
    obj["id"] = m_id;
    obj["username"] = m_username;
    obj["role"] = static_cast<int>(m_role);
    obj["role_string"] = roleString();
    obj["real_name"] = m_realName;
    return obj;
}

User User::fromJson(const QJsonObject &obj)
{
    User user;
    user.m_id = obj["id"].toInt();
    user.m_username = obj["username"].toString();
    user.m_role = static_cast<Constants::UserRole>(obj["role"].toInt());
    user.m_realName = obj["real_name"].toString();
    return user;
}
