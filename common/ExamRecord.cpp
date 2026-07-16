#include "ExamRecord.h"
#include <QJsonArray>

ExamRecord::ExamRecord()
    : m_id(0), m_userId(0), m_examId(0), m_score(0.0),
      m_status(Constants::StatusInProgress)
{
}

ExamRecord::ExamRecord(int id, int userId, int examId, const QList<int> &questionIds,
                       const QMap<int, QString> &answers, double score,
                       const QDateTime &startTime, const QDateTime &endTime,
                       Constants::ExamStatus status)
    : m_id(id), m_userId(userId), m_examId(examId), m_questionIds(questionIds),
      m_answers(answers), m_score(score), m_startTime(startTime),
      m_endTime(endTime), m_status(status)
{
}

QString ExamRecord::statusString() const
{
    switch (m_status) {
    case Constants::StatusInProgress: return QString::fromUtf8("进行中");
    case Constants::StatusSubmitted: return QString::fromUtf8("已提交");
    case Constants::StatusAutoSubmit: return QString::fromUtf8("自动交卷");
    default: return QString::fromUtf8("未知");
    }
}

int ExamRecord::duration() const
{
    if (!m_startTime.isValid() || !m_endTime.isValid())
        return 0;
    return m_startTime.secsTo(m_endTime);
}

QJsonObject ExamRecord::toJson() const
{
    QJsonObject obj;
    obj["id"] = m_id;
    obj["user_id"] = m_userId;
    obj["exam_id"] = m_examId;

    QJsonArray questionArray;
    for (int qid : m_questionIds) {
        questionArray.append(qid);
    }
    obj["questions"] = questionArray;

    QJsonObject answerObj;
    for (auto it = m_answers.begin(); it != m_answers.end(); ++it) {
        answerObj[QString::number(it.key())] = it.value();
    }
    obj["answers"] = answerObj;

    obj["score"] = m_score;
    obj["start_time"] = m_startTime.toString(Qt::ISODate);
    obj["end_time"] = m_endTime.toString(Qt::ISODate);
    obj["status"] = static_cast<int>(m_status);
    obj["status_string"] = statusString();
    obj["duration"] = duration();
    return obj;
}

ExamRecord ExamRecord::fromJson(const QJsonObject &obj)
{
    ExamRecord record;
    record.m_id = obj["id"].toInt();
    record.m_userId = obj["user_id"].toInt();
    record.m_examId = obj["exam_id"].toInt();

    QJsonArray questionArray = obj["questions"].toArray();
    for (const QJsonValue &val : questionArray) {
        record.m_questionIds.append(val.toInt());
    }

    QJsonObject answerObj = obj["answers"].toObject();
    for (const QString &key : answerObj.keys()) {
        record.m_answers[key.toInt()] = answerObj[key].toString();
    }

    record.m_score = obj["score"].toDouble();
    record.m_startTime = QDateTime::fromString(obj["start_time"].toString(), Qt::ISODate);
    record.m_endTime = QDateTime::fromString(obj["end_time"].toString(), Qt::ISODate);
    record.m_status = static_cast<Constants::ExamStatus>(obj["status"].toInt());
    return record;
}
