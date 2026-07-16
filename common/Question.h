#ifndef QUESTION_H
#define QUESTION_H

#include <QString>
#include <QStringList>
#include <QJsonObject>
#include <QJsonArray>
#include "Constants.h"

class Question
{
public:
    Question();
    Question(int id, Constants::QuestionType type, const QString &content,
             const QStringList &options, const QString &answer,
             const QString &category, Constants::Difficulty difficulty);

    int id() const { return m_id; }
    void setId(int id) { m_id = id; }

    Constants::QuestionType type() const { return m_type; }
    void setType(Constants::QuestionType type) { m_type = type; }
    QString typeString() const;

    QString content() const { return m_content; }
    void setContent(const QString &content) { m_content = content; }

    QStringList options() const { return m_options; }
    void setOptions(const QStringList &options) { m_options = options; }

    QString answer() const { return m_answer; }
    void setAnswer(const QString &answer) { m_answer = answer; }

    QString category() const { return m_category; }
    void setCategory(const QString &category) { m_category = category; }

    Constants::Difficulty difficulty() const { return m_difficulty; }
    void setDifficulty(Constants::Difficulty difficulty) { m_difficulty = difficulty; }
    QString difficultyString() const;

    QJsonObject toJson() const;
    static Question fromJson(const QJsonObject &obj);

    // 检查答案是否正确
    bool checkAnswer(const QString &userAnswer) const;
    // 获取分值（单选2分，多选3分，判断1分）
    int score() const;

private:
    int m_id;
    Constants::QuestionType m_type;
    QString m_content;
    QStringList m_options;
    QString m_answer;
    QString m_category;
    Constants::Difficulty m_difficulty;
};

#endif // QUESTION_H
