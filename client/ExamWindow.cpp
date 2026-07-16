#include "ExamWindow.h"
#include "QuestionWidget.h"
#include "HttpClient.h"
#include "JsonHelper.h"
#include "Constants.h"
#include "ExamResultDialog.h"
#include <QSplitter>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QMessageBox>
#include <QScrollArea>
#include <QFrame>
#include <QDebug>
#include <QApplication>
#include <QScreen>

ExamWindow::ExamWindow(const User &user, QWidget *parent)
    : QMainWindow(parent), m_user(user), m_currentIndex(0),
      m_totalSeconds(0), m_remainingSeconds(0), m_examSubmitted(false)
{
    setWindowTitle(QString::fromUtf8("在线考试 - %1").arg(user.realName()));

    // 根据屏幕尺寸自适应窗口大小
    QRect screen = QApplication::primaryScreen()->availableGeometry();
    QSize size = screen.size() * 4 / 5;
    size = size.boundedTo(QSize(1000, 700));
    size = size.expandedTo(QSize(800, 560));
    resize(size);
    setMinimumSize(760, 540);

    setupUI();
    setupConnections();
}

ExamWindow::~ExamWindow()
{
    if (m_timer->isActive()) {
        m_timer->stop();
    }
}

void ExamWindow::setupUI()
{
    // 中央部件
    m_centralWidget = new QWidget(this);
    setCentralWidget(m_centralWidget);

    QVBoxLayout *mainLayout = new QVBoxLayout(m_centralWidget);
    mainLayout->setSpacing(0);
    mainLayout->setContentsMargins(0, 0, 0, 0);

    // ===== 顶部状态栏 =====
    QFrame *topBar = new QFrame(this);
    topBar->setStyleSheet(
        "QFrame {"
        "  background-color: #2196F3;"
        "  color: white;"
        "  padding: 10px;"
        "}"
    );
    QHBoxLayout *topLayout = new QHBoxLayout(topBar);
    topLayout->setContentsMargins(15, 10, 15, 10);

    m_infoLabel = new QLabel(this);
    m_infoLabel->setStyleSheet("QLabel { color: white; font-size: 14px; }");
    topLayout->addWidget(m_infoLabel);

    topLayout->addStretch();

    m_timerLabel = new QLabel(this);
    m_timerLabel->setStyleSheet(
        "QLabel {"
        "  color: white;"
        "  font-size: 20px;"
        "  font-weight: bold;"
        "  padding: 5px 15px;"
        "  background-color: #1976D2;"
        "  border-radius: 4px;"
        "}"
    );
    topLayout->addWidget(m_timerLabel);

    QPushButton *logoutBtn = new QPushButton(QString::fromUtf8("退出登录"), this);
    logoutBtn->setStyleSheet(
        "QPushButton {"
        "  background-color: transparent;"
        "  color: white;"
        "  border: 1px solid white;"
        "  padding: 5px 15px;"
        "  border-radius: 3px;"
        "}"
        "QPushButton:hover {"
        "  background-color: rgba(255,255,255,0.2);"
        "}"
    );
    connect(logoutBtn, &QPushButton::clicked, this, &ExamWindow::onLogout);
    topLayout->addWidget(logoutBtn);

    mainLayout->addWidget(topBar);

    // ===== 进度条 =====
    m_progressBar = new QProgressBar(this);
    m_progressBar->setTextVisible(true);
    m_progressBar->setStyleSheet(
        "QProgressBar {"
        "  border: none;"
        "  height: 20px;"
        "  text-align: center;"
        "}"
        "QProgressBar::chunk {"
        "  background-color: #4CAF50;"
        "}"
    );
    mainLayout->addWidget(m_progressBar);

    // ===== 主体区域（分割器） =====
    QSplitter *splitter = new QSplitter(Qt::Horizontal, this);

    // 左侧题号列表
    QWidget *leftPanel = new QWidget(this);
    QVBoxLayout *leftLayout = new QVBoxLayout(leftPanel);
    leftLayout->setContentsMargins(10, 10, 10, 10);

    QLabel *listTitle = new QLabel(QString::fromUtf8("题目导航"), this);
    listTitle->setStyleSheet("QLabel { font-size: 14px; font-weight: bold; padding: 5px; }");
    leftLayout->addWidget(listTitle);

    m_questionList = new QListWidget(this);
    m_questionList->setMaximumWidth(200);
    m_questionList->setStyleSheet(
        "QListWidget {"
        "  border: 1px solid #ddd;"
        "  border-radius: 4px;"
        "  padding: 5px;"
        "}"
        "QListWidget::item {"
        "  padding: 10px;"
        "  border-bottom: 1px solid #eee;"
        "}"
        "QListWidget::item:selected {"
        "  background-color: #2196F3;"
        "  color: white;"
        "}"
        "QListWidget::item:hover {"
        "  background-color: #e3f2fd;"
        "}"
    );
    leftLayout->addWidget(m_questionList);

    // 右侧题目区域
    QWidget *rightPanel = new QWidget(this);
    QVBoxLayout *rightLayout = new QVBoxLayout(rightPanel);
    rightLayout->setContentsMargins(10, 10, 10, 10);

    m_questionStack = new QStackedWidget(this);

    // 用滚动区域包裹题目栈，避免题目内容过多时底部按钮被挤出可视区
    QScrollArea *questionScroll = new QScrollArea(this);
    questionScroll->setWidgetResizable(true);
    questionScroll->setFrameShape(QFrame::NoFrame);
    questionScroll->setWidget(m_questionStack);
    rightLayout->addWidget(questionScroll, 1);

    // 底部导航按钮
    QFrame *bottomBar = new QFrame(this);
    bottomBar->setStyleSheet("QFrame { background-color: #f5f5f5; border-top: 1px solid #ddd; }");
    QHBoxLayout *bottomLayout = new QHBoxLayout(bottomBar);
    bottomLayout->setContentsMargins(15, 10, 15, 10);

    m_prevBtn = new QPushButton(QString::fromUtf8("◀ 上一题"), this);
    m_prevBtn->setStyleSheet(
        "QPushButton {"
        "  padding: 10px 20px;"
        "  font-size: 14px;"
        "  background-color: #607D8B;"
        "  color: white;"
        "  border: none;"
        "  border-radius: 4px;"
        "}"
        "QPushButton:hover { background-color: #546E7A; }"
        "QPushButton:disabled { background-color: #B0BEC5; }"
    );

    m_nextBtn = new QPushButton(QString::fromUtf8("下一题 ▶"), this);
    m_nextBtn->setStyleSheet(
        "QPushButton {"
        "  padding: 10px 20px;"
        "  font-size: 14px;"
        "  background-color: #607D8B;"
        "  color: white;"
        "  border: none;"
        "  border-radius: 4px;"
        "}"
        "QPushButton:hover { background-color: #546E7A; }"
        "QPushButton:disabled { background-color: #B0BEC5; }"
    );

    m_submitBtn = new QPushButton(QString::fromUtf8("✓ 交卷"), this);
    m_submitBtn->setStyleSheet(
        "QPushButton {"
        "  padding: 10px 30px;"
        "  font-size: 14px;"
        "  background-color: #f44336;"
        "  color: white;"
        "  border: none;"
        "  border-radius: 4px;"
        "  font-weight: bold;"
        "}"
        "QPushButton:hover { background-color: #d32f2f; }"
    );

    bottomLayout->addWidget(m_prevBtn);
    bottomLayout->addStretch();
    bottomLayout->addWidget(m_nextBtn);
    bottomLayout->addSpacing(20);
    bottomLayout->addWidget(m_submitBtn);

    rightLayout->addWidget(bottomBar);

    splitter->addWidget(leftPanel);
    splitter->addWidget(rightPanel);
    splitter->setStretchFactor(0, 0);
    splitter->setStretchFactor(1, 1);
    splitter->setSizes(QList<int>() << 200 << 800);

    mainLayout->addWidget(splitter, 1);
}

void ExamWindow::setupConnections()
{
    connect(m_questionList, &QListWidget::currentRowChanged,
            this, &ExamWindow::onQuestionSelected);
    connect(m_prevBtn, &QPushButton::clicked, this, &ExamWindow::onPrevClicked);
    connect(m_nextBtn, &QPushButton::clicked, this, &ExamWindow::onNextClicked);
    connect(m_submitBtn, &QPushButton::clicked, this, &ExamWindow::onSubmitClicked);

    m_timer = new QTimer(this);
    m_timer->setInterval(1000);
    connect(m_timer, &QTimer::timeout, this, &ExamWindow::updateTimer);
}

void ExamWindow::startExam()
{
    loadQuestions();
}

void ExamWindow::loadQuestions()
{
    HttpClient client;
    // 使用当前用户ID作为token
    client.setAuthToken(QString::number(m_user.id()));

    QJsonObject response = client.syncGet(Constants::API_BASE + "/questions/random?count=25");

    if (!response["success"].toBool()) {
        QMessageBox::critical(this, QString::fromUtf8("错误"),
                              QString::fromUtf8("获取题目失败：") + response["message"].toString());
        return;
    }

    QJsonObject data = response["data"].toObject();
    QJsonArray questionsArray = data["questions"].toArray();

    m_questions.clear();
    m_answers.clear();

    for (const QJsonValue &val : questionsArray) {
        Question q = Question::fromJson(val.toObject());
        m_questions.append(q);
    }

    if (m_questions.isEmpty()) {
        QMessageBox::warning(this, QString::fromUtf8("警告"),
                               QString::fromUtf8("题库中可组成的题目数量不足，无法开始考试。\n"
                                                 "请确保至少包含 15 道单选题、5 道多选题、5 道判断题。"));
        return;
    }

    // 初始化界面
    m_questionList->clear();
    while (m_questionStack->count() > 0) {
        QWidget *w = m_questionStack->widget(0);
        m_questionStack->removeWidget(w);
        delete w;
    }

    for (int i = 0; i < m_questions.size(); ++i) {
        // 添加题号列表项
        QListWidgetItem *item = new QListWidgetItem(
            QString::fromUtf8("第%1题").arg(i + 1), m_questionList);
        item->setData(Qt::UserRole, m_questions[i].id());
        m_questionList->addItem(item);

        // 添加题目组件
        QuestionWidget *widget = new QuestionWidget(m_questions[i], this);
        connect(widget, &QuestionWidget::answerChanged,
                this, &ExamWindow::onAnswerChanged);
        m_questionStack->addWidget(widget);
    }

    // 设置考试信息
    int examDurationMinutes = data["duration"].toInt();
    if (examDurationMinutes <= 0) {
        examDurationMinutes = Constants::DEFAULT_EXAM_DURATION;
    }
    m_totalSeconds = examDurationMinutes * 60;
    m_remainingSeconds = m_totalSeconds;
    m_startTime = QDateTime::currentDateTime();
    m_examDeadline = m_startTime.addSecs(m_totalSeconds);
    m_examSubmitted = false;

    m_infoLabel->setText(QString::fromUtf8("考生：%1 | 题目数：%2 | 总分：%3分")
                         .arg(m_user.realName())
                         .arg(m_questions.size())
                         .arg(calculateTotalScore()));

    m_progressBar->setMaximum(m_questions.size());
    m_progressBar->setValue(0);
    m_progressBar->setFormat(QString::fromUtf8("已答 %v/%m 题"));

    // 显示第一题
    m_currentIndex = 0;
    m_questionList->setCurrentRow(0);
    showQuestion(0);

    // 启动计时器
    m_timer->start();
    updateTimer();
}

int ExamWindow::calculateTotalScore() const
{
    int total = 0;
    for (const Question &q : m_questions) {
        total += q.score();
    }
    return total;
}

void ExamWindow::showQuestion(int index)
{
    if (index < 0 || index >= m_questions.size()) {
        return;
    }

    m_currentIndex = index;
    m_questionStack->setCurrentIndex(index);

    // 恢复已保存的答案
    QuestionWidget *widget = qobject_cast<QuestionWidget*>(m_questionStack->widget(index));
    if (widget) {
        int qid = m_questions[index].id();
        if (m_answers.contains(qid)) {
            widget->setAnswer(m_answers[qid]);
        }
    }

    updateNavigationButtons();
    updateQuestionList();
}

void ExamWindow::updateQuestionList()
{
    for (int i = 0; i < m_questionList->count(); ++i) {
        QListWidgetItem *item = m_questionList->item(i);
        int qid = item->data(Qt::UserRole).toInt();

        if (m_answers.contains(qid) && !m_answers[qid].isEmpty()) {
            item->setText(QString::fromUtf8("第%1题 ✓").arg(i + 1));
            item->setBackground(QColor("#e8f5e9"));
        } else {
            item->setText(QString::fromUtf8("第%1题").arg(i + 1));
            item->setBackground(QColor("white"));
        }
    }

    // 更新进度条
    int answeredCount = 0;
    for (auto it = m_answers.begin(); it != m_answers.end(); ++it) {
        if (!it.value().isEmpty()) {
            answeredCount++;
        }
    }
    m_progressBar->setValue(answeredCount);
}

void ExamWindow::updateNavigationButtons()
{
    m_prevBtn->setEnabled(m_currentIndex > 0);
    m_nextBtn->setEnabled(m_currentIndex < m_questions.size() - 1);
}

void ExamWindow::onQuestionSelected(int index)
{
    showQuestion(index);
}

void ExamWindow::onPrevClicked()
{
    if (m_currentIndex > 0) {
        m_questionList->setCurrentRow(m_currentIndex - 1);
    }
}

void ExamWindow::onNextClicked()
{
    if (m_currentIndex < m_questions.size() - 1) {
        m_questionList->setCurrentRow(m_currentIndex + 1);
    }
}

void ExamWindow::onSubmitClicked()
{
    // 检查是否有未答题目
    int unanswered = 0;
    for (const Question &q : m_questions) {
        if (!m_answers.contains(q.id()) || m_answers[q.id()].isEmpty()) {
            unanswered++;
        }
    }

    QString msg = QString::fromUtf8("确定要交卷吗？\n");
    if (unanswered > 0) {
        msg += QString::fromUtf8("你还有 %1 道题目未作答。\n").arg(unanswered);
    }
    msg += QString::fromUtf8("交卷后将无法修改答案。");

    int ret = QMessageBox::question(this, QString::fromUtf8("确认交卷"), msg,
                                     QMessageBox::Yes | QMessageBox::No,
                                     QMessageBox::No);

    if (ret == QMessageBox::Yes) {
        submitExam(false);
    }
}

void ExamWindow::onTimeOut()
{
    QMessageBox::warning(this, QString::fromUtf8("考试时间到"),
                         QString::fromUtf8("考试时间已结束，系统将自动交卷。"));
    submitExam(true);
}

void ExamWindow::onAnswerChanged(int questionId, const QString &answer)
{
    m_answers[questionId] = answer;
    updateQuestionList();
}

void ExamWindow::updateTimer()
{
    // 根据当前系统时间与截止时间的真实差距计算剩余时间，避免 QTimer 累计误差
    // 同时把时间限制在 [0, m_totalSeconds] 范围内，防止显示超过考试限定时间
    int remaining = static_cast<int>(QDateTime::currentDateTime().secsTo(m_examDeadline));
    remaining = qBound(0, remaining, m_totalSeconds);
    m_remainingSeconds = remaining;

    if (m_remainingSeconds <= 0) {
        m_timer->stop();
        m_timerLabel->setText(QString::fromUtf8("00:00:00"));
        onTimeOut();
        return;
    }

    m_timerLabel->setText(formatTime(m_remainingSeconds));

    // 最后5分钟警告
    if (m_remainingSeconds <= 300) {
        m_timerLabel->setStyleSheet(
            "QLabel {"
            "  color: white;"
            "  font-size: 20px;"
            "  font-weight: bold;"
            "  padding: 5px 15px;"
            "  background-color: #f44336;"
            "  border-radius: 4px;"
            "}"
        );
    }

    // 最后1分钟闪烁
    if (m_remainingSeconds <= 60) {
        static bool blink = false;
        blink = !blink;
        if (blink) {
            m_timerLabel->setStyleSheet(
                "QLabel {"
                "  color: white;"
                "  font-size: 20px;"
                "  font-weight: bold;"
                "  padding: 5px 15px;"
                "  background-color: #d32f2f;"
                "  border-radius: 4px;"
                "}"
            );
        }
    }
}

void ExamWindow::onLogout()
{
    int ret = QMessageBox::question(this, QString::fromUtf8("确认退出"),
                                     QString::fromUtf8("确定要退出登录吗？\n退出后将返回登录界面。"),
                                     QMessageBox::Yes | QMessageBox::No,
                                     QMessageBox::No);
    if (ret == QMessageBox::Yes) {
        if (m_timer->isActive()) {
            m_timer->stop();
        }
        emit logoutRequested();
        close();
    }
}

QString ExamWindow::formatTime(int seconds) const
{
    int h = seconds / 3600;
    int m = (seconds % 3600) / 60;
    int s = seconds % 60;
    return QString("%1:%2:%3")
        .arg(h, 2, 10, QChar('0'))
        .arg(m, 2, 10, QChar('0'))
        .arg(s, 2, 10, QChar('0'));
}

void ExamWindow::submitExam(bool autoSubmit)
{
    if (m_examSubmitted) {
        return;
    }

    m_examSubmitted = true;
    m_timer->stop();

    // 构建提交数据
    QJsonArray questionArray;
    for (const Question &q : m_questions) {
        questionArray.append(q.id());
    }

    QJsonObject answerObj;
    for (auto it = m_answers.begin(); it != m_answers.end(); ++it) {
        answerObj[QString::number(it.key())] = it.value();
    }

    QJsonObject submitData;
    submitData["exam_id"] = 1;
    submitData["questions"] = questionArray;
    submitData["answers"] = answerObj;
    submitData["start_time"] = m_startTime.toString(Qt::ISODate);

    HttpClient client;
    client.setAuthToken(QString::number(m_user.id()));

    QJsonObject response = client.syncPost(Constants::API_BASE + "/exam/submit", submitData);

    if (!response["success"].toBool()) {
        QMessageBox::critical(this, QString::fromUtf8("提交失败"),
                              response["message"].toString());
        m_examSubmitted = false;
        return;
    }

    // 获取考试记录
    QJsonObject data = response["data"].toObject();
    int recordId = data["record_id"].toInt();

    QJsonObject resultResponse = client.syncGet(
        Constants::API_BASE + "/exam/result?record_id=" + QString::number(recordId));

    if (resultResponse["success"].toBool()) {
        ExamRecord record = ExamRecord::fromJson(resultResponse["data"].toObject());

        ExamResultDialog *dialog = new ExamResultDialog(record, m_questions, this);
        dialog->exec();
    }

    emit examFinished();
    close();
}
