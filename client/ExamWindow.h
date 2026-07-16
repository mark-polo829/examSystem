#ifndef EXAM_WINDOW_H
#define EXAM_WINDOW_H

#include <QMainWindow>
#include <QListWidget>
#include <QLabel>
#include <QPushButton>
#include <QTimer>
#include <QStackedWidget>
#include <QProgressBar>
#include <QList>
#include <QMap>
#include "Question.h"
#include "ExamRecord.h"
#include "User.h"

class QuestionWidget;

class ExamWindow : public QMainWindow
{
    Q_OBJECT
public:
    explicit ExamWindow(const User &user, QWidget *parent = nullptr);
    ~ExamWindow();

    void startExam();

signals:
    void examFinished();
    void logoutRequested();

private slots:
    void onQuestionSelected(int index);
    void onPrevClicked();
    void onNextClicked();
    void onSubmitClicked();
    void onTimeOut();
    void onAnswerChanged(int questionId, const QString &answer);
    void updateTimer();
    void onLogout();

private:
    void setupUI();
    void setupConnections();
    void loadQuestions();
    void showQuestion(int index);
    void updateQuestionList();
    void updateNavigationButtons();
    void submitExam(bool autoSubmit = false);
    int calculateRemainingSeconds() const;
    int calculateTotalScore() const;
    QString formatTime(int seconds) const;

    // 界面组件
    QWidget *m_centralWidget;
    QListWidget *m_questionList;
    QStackedWidget *m_questionStack;
    QLabel *m_timerLabel;
    QLabel *m_infoLabel;
    QProgressBar *m_progressBar;
    QPushButton *m_prevBtn;
    QPushButton *m_nextBtn;
    QPushButton *m_submitBtn;

    // 数据
    User m_user;
    QList<Question> m_questions;
    QMap<int, QString> m_answers;  // questionId -> answer
    int m_currentIndex;
    int m_totalSeconds;
    int m_remainingSeconds;

    // 计时器
    QTimer *m_timer;
    QDateTime m_startTime;
    QDateTime m_examDeadline;   // 考试截止时间，防止显示超过限定时间
    bool m_examSubmitted;
};

#endif // EXAM_WINDOW_H
