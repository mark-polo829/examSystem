#include "Question.h"
#include <QJsonArray>
#include <QSet>

Question::Question()
    : m_id(0), m_type(Constants::TypeSingleChoice),
      m_difficulty(Constants::DiffEasy)
{
}

Question::Question(int id, Constants::QuestionType type, const QString &content,
                   const QStringList &options, const QString &answer,
                   const QString &category, Constants::Difficulty difficulty)
    : m_id(id), m_type(type), m_content(content), m_options(options),
      m_answer(answer), m_category(category), m_difficulty(difficulty)
{
}

QString Question::typeString() const
{
    switch (m_type) {
    case Constants::TypeSingleChoice: return QString::fromUtf8("单选题");
    case Constants::TypeMultipleChoice: return QString::fromUtf8("多选题");
    case Constants::TypeTrueFalse: return QString::fromUtf8("判断题");
    default: return QString::fromUtf8("未知");
    }
}

QString Question::difficultyString() const
{
    switch (m_difficulty) {
    case Constants::DiffEasy: return QString::fromUtf8("易");
    case Constants::DiffMedium: return QString::fromUtf8("中");
    case Constants::DiffHard: return QString::fromUtf8("难");
    default: return QString::fromUtf8("未知");
    }
}

QJsonObject Question::toJson() const
{
    QJsonObject obj;
    obj["id"] = m_id;
    obj["type"] = static_cast<int>(m_type);
    obj["type_string"] = typeString();
    obj["content"] = m_content;

    QJsonArray optionsArray;
    for (const QString &opt : m_options) {
        optionsArray.append(opt);
    }
    obj["options"] = optionsArray;

    obj["answer"] = m_answer;
    obj["category"] = m_category;
    obj["difficulty"] = static_cast<int>(m_difficulty);
    obj["difficulty_string"] = difficultyString();
    obj["score"] = score();
    return obj;
}

Question Question::fromJson(const QJsonObject &obj)
{
    Question q;
    q.m_id = obj["id"].toInt();
    q.m_type = static_cast<Constants::QuestionType>(obj["type"].toInt());
    q.m_content = obj["content"].toString();

    QJsonArray optionsArray = obj["options"].toArray();
    for (const QJsonValue &val : optionsArray) {
        q.m_options.append(val.toString());
    }

    q.m_answer = obj["answer"].toString();
    q.m_category = obj["category"].toString();
    q.m_difficulty = static_cast<Constants::Difficulty>(obj["difficulty"].toInt());
    return q;
}

bool Question::checkAnswer(const QString &userAnswer) const
{
    // 标准化答案比较（去除空格，统一大小写）
    QString correct = m_answer.trimmed().toUpper();
    QString user = userAnswer.trimmed().toUpper();

    if (m_type == Constants::TypeMultipleChoice) {
        // 多选题：比较字符集合
        QSet<QChar> correctSet;
        for (const QChar &c : correct) {
            if (c.isLetter()) correctSet.insert(c);
        }
        QSet<QChar> userSet;
        for (const QChar &c : user) {
            if (c.isLetter()) userSet.insert(c);
        }
        return correctSet == userSet;
    }

    return correct == user;
}

int Question::score() const
{
    switch (m_type) {
    case Constants::TypeSingleChoice: return 2;
    case Constants::TypeMultipleChoice: return 3;
    case Constants::TypeTrueFalse: return 1;
    default: return 0;
    }
}
