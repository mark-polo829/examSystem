#ifndef EXAM_RECORD_H
#define EXAM_RECORD_H

#include <QString>
#include <QDateTime>
#include <QMap>
#include <QJsonObject>
#include <QJsonArray>
#include "Constants.h"

class ExamRecord
{
public:
    ExamRecord();
    ExamRecord(int id, int userId, int examId, const QList<int> &questionIds,
               const QMap<int, QString> &answers, double score,
               const QDateTime &startTime, const QDateTime &endTime,
               Constants::ExamStatus status);

    int id() const { return m_id; }
    void setId(int id) { m_id = id; }

    int userId() const { return m_userId; }
    void setUserId(int userId) { m_userId = userId; }

    int examId() const { return m_examId; }
    void setExamId(int examId) { m_examId = examId; }

    QList<int> questionIds() const { return m_questionIds; }
    void setQuestionIds(const QList<int> &questionIds) { m_questionIds = questionIds; }

    QMap<int, QString> answers() const { return m_answers; }
    void setAnswers(const QMap<int, QString> &answers) { m_answers = answers; }

    double score() const { return m_score; }
    void setScore(double score) { m_score = score; }

    QDateTime startTime() const { return m_startTime; }
    void setStartTime(const QDateTime &startTime) { m_startTime = startTime; }

    QDateTime endTime() const { return m_endTime; }
    void setEndTime(const QDateTime &endTime) { m_endTime = endTime; }

    Constants::ExamStatus status() const { return m_status; }
    void setStatus(Constants::ExamStatus status) { m_status = status; }
    QString statusString() const;

    // 计算用时（秒）
    int duration() const;

    QJsonObject toJson() const;
    static ExamRecord fromJson(const QJsonObject &obj);

private:
    int m_id;
    int m_userId;
    int m_examId;
    QList<int> m_questionIds;
    QMap<int, QString> m_answers;
    double m_score;
    QDateTime m_startTime;
    QDateTime m_endTime;
    Constants::ExamStatus m_status;
};

#endif // EXAM_RECORD_H
