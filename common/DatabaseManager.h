#ifndef DATABASE_MANAGER_H
#define DATABASE_MANAGER_H

#include <QObject>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QMutex>
#include <QMutexLocker>
#include <QList>
#include "User.h"
#include "Question.h"
#include "ExamRecord.h"

class DatabaseManager : public QObject
{
    Q_OBJECT
public:
    static DatabaseManager* instance();

    bool initialize(const QString &dbPath);
    bool createTables();
    bool insertDefaultData();

    // 用户相关
    User getUserByUsername(const QString &username);
    User getUserById(int id);
    QList<User> getAllUsers();
    bool addUser(const User &user);
    bool updateUser(const User &user);
    bool deleteUser(int id);

    // 题目相关
    Question getQuestionById(int id);
    QList<Question> getAllQuestions();
    QList<Question> getQuestionsByCategory(const QString &category);
    QList<Question> getRandomQuestions(int count, const QString &category = QString());
    // 按固定题型组成抽取考试题目（保证不重复且总分固定）
    QList<Question> getExamQuestions(const QString &category = QString());
    int getExamDuration(int examId = 1);
    bool addQuestion(const Question &question);
    bool updateQuestion(const Question &question);
    bool deleteQuestion(int id);
    QList<QString> getAllCategories();

    // 考试记录相关
    ExamRecord getExamRecordById(int id);
    QList<ExamRecord> getExamRecordsByUserId(int userId);
    QList<ExamRecord> getAllExamRecords();
    bool addExamRecord(ExamRecord &record);  // 传入引用，会设置id
    bool updateExamRecord(const ExamRecord &record);

    // 成绩统计
    double getAverageScore(int examId = -1);
    int getExamCount(int examId = -1);
    QMap<QString, int> getScoreDistribution(int examId = -1);  // 优秀/良好/及格/不及格
    QMap<QString, double> getAverageScoreByCategory();

private:
    explicit DatabaseManager(QObject *parent = nullptr);
    ~DatabaseManager();

    QSqlDatabase m_db;
    QMutex m_mutex{QMutex::Recursive};

    static DatabaseManager *m_instance;

    // 从 QSqlQuery 构造实体，减少重复代码
    Question questionFromQuery(const QSqlQuery &query) const;
    ExamRecord examRecordFromQuery(const QSqlQuery &query) const;
};

#endif // DATABASE_MANAGER_H
