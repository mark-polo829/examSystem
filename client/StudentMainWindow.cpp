#include "StudentMainWindow.h"
#include "HttpClient.h"
#include "JsonHelper.h"
#include "Constants.h"
#include "ExamResultDialog.h"
#include "ExamRecord.h"
#include "Question.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QMessageBox>
#include <QDateTime>
#include <QScrollArea>
#include <QFrame>

StudentMainWindow::StudentMainWindow(const User &user, QWidget *parent)
    : QMainWindow(parent), m_user(user)
{
    setWindowTitle(QString::fromUtf8("考生中心"));
    resize(900, 600);
    setupUI();
    setupConnections();
    refreshRecords();
}

void StudentMainWindow::setupUI()
{
    QWidget *centralWidget = new QWidget(this);
    setCentralWidget(centralWidget);

    QVBoxLayout *mainLayout = new QVBoxLayout(centralWidget);
    mainLayout->setSpacing(20);
    mainLayout->setContentsMargins(30, 30, 30, 30);

    // ===== 顶部欢迎栏 =====
    QFrame *topBar = new QFrame(this);
    topBar->setStyleSheet(
        "QFrame {"
        "  background-color: #2196F3;"
        "  border-radius: 8px;"
        "  padding: 15px;"
        "}"
    );
    QHBoxLayout *topLayout = new QHBoxLayout(topBar);
    topLayout->setContentsMargins(20, 15, 20, 15);

    m_welcomeLabel = new QLabel(this);
    m_welcomeLabel->setStyleSheet("QLabel { color: white; font-size: 18px; font-weight: bold; }");
    m_welcomeLabel->setText(QString::fromUtf8("欢迎，%1").arg(m_user.realName()));
    topLayout->addWidget(m_welcomeLabel);

    topLayout->addStretch();

    m_logoutBtn = new QPushButton(QString::fromUtf8("退出登录"), this);
    m_logoutBtn->setStyleSheet(
        "QPushButton {"
        "  background-color: transparent;"
        "  color: white;"
        "  border: 1px solid white;"
        "  padding: 8px 20px;"
        "  border-radius: 4px;"
        "  font-size: 13px;"
        "}"
        "QPushButton:hover {"
        "  background-color: rgba(255,255,255,0.2);"
        "}"
    );
    topLayout->addWidget(m_logoutBtn);

    mainLayout->addWidget(topBar);

    // ===== 考试记录区域 =====
    QLabel *recordsTitle = new QLabel(QString::fromUtf8("历史考试记录"), this);
    recordsTitle->setStyleSheet("QLabel { font-size: 16px; font-weight: bold; color: #333; }");
    mainLayout->addWidget(recordsTitle);

    m_recordsTable = new QTableWidget(this);
    m_recordsTable->setColumnCount(6);
    m_recordsTable->setHorizontalHeaderLabels(QStringList()
        << QString::fromUtf8("序号")
        << QString::fromUtf8("考试时间")
        << QString::fromUtf8("得分")
        << QString::fromUtf8("用时")
        << QString::fromUtf8("状态")
        << QString::fromUtf8("操作"));
    m_recordsTable->horizontalHeader()->setStretchLastSection(false);
    m_recordsTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    m_recordsTable->horizontalHeader()->setSectionResizeMode(5, QHeaderView::Fixed);
    m_recordsTable->setColumnWidth(5, 150);
    m_recordsTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_recordsTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_recordsTable->setAlternatingRowColors(true);
    m_recordsTable->verticalHeader()->setDefaultSectionSize(60);
    m_recordsTable->verticalHeader()->setSectionResizeMode(QHeaderView::Fixed);
    m_recordsTable->setStyleSheet(
        "QTableWidget {"
        "  border: 1px solid #ddd;"
        "  gridline-color: #eee;"
        "  font-size: 13px;"
        "}"
        "QHeaderView::section {"
        "  background-color: #f5f5f5;"
        "  padding: 10px;"
        "  border: 1px solid #ddd;"
        "  font-weight: bold;"
        "}"
        "QTableWidget::item { padding: 8px; }"
    );
    mainLayout->addWidget(m_recordsTable, 1);

    // ===== 状态提示 =====
    m_statusLabel = new QLabel(this);
    m_statusLabel->setAlignment(Qt::AlignCenter);
    m_statusLabel->setStyleSheet("QLabel { color: #666; font-size: 13px; }");
    mainLayout->addWidget(m_statusLabel);

    // ===== 底部按钮 =====
    QHBoxLayout *bottomLayout = new QHBoxLayout();
    bottomLayout->addStretch();

    m_startExamBtn = new QPushButton(QString::fromUtf8("开始考试"), this);
    m_startExamBtn->setStyleSheet(
        "QPushButton {"
        "  background-color: #4CAF50;"
        "  color: white;"
        "  border: none;"
        "  padding: 12px 50px;"
        "  font-size: 16px;"
        "  font-weight: bold;"
        "  border-radius: 6px;"
        "}"
        "QPushButton:hover {"
        "  background-color: #45a049;"
        "}"
    );
    bottomLayout->addWidget(m_startExamBtn);
    bottomLayout->addStretch();

    mainLayout->addLayout(bottomLayout);
}

void StudentMainWindow::setupConnections()
{
    connect(m_startExamBtn, &QPushButton::clicked, this, &StudentMainWindow::onStartExamClicked);
    connect(m_logoutBtn, &QPushButton::clicked, this, &StudentMainWindow::onLogoutClicked);
}

void StudentMainWindow::refreshRecords()
{
    m_statusLabel->setText(QString::fromUtf8("正在加载记录..."));
    m_statusLabel->setStyleSheet("QLabel { color: #2196F3; font-size: 13px; }");
    loadRecords();
}

void StudentMainWindow::loadRecords()
{
    HttpClient client;
    client.setAuthToken(QString::number(m_user.id()));

    QJsonObject response = client.syncGet(Constants::API_BASE + "/exam/records");

    if (!response["success"].toBool()) {
        m_statusLabel->setText(QString::fromUtf8("加载记录失败：") + response["message"].toString());
        m_statusLabel->setStyleSheet("QLabel { color: #f44336; font-size: 13px; }");
        return;
    }

    QJsonObject data = response["data"].toObject();
    m_records = data["records"].toArray();

    m_recordsTable->setRowCount(m_records.size());

    for (int i = 0; i < m_records.size(); ++i) {
        QJsonObject recordObj = m_records[i].toObject();

        QDateTime startTime = QDateTime::fromString(recordObj["start_time"].toString(), Qt::ISODate);
        QDateTime endTime = QDateTime::fromString(recordObj["end_time"].toString(), Qt::ISODate);
        int duration = recordObj["duration"].toInt();
        double score = recordObj["score"].toDouble();
        QString status = recordObj["status_string"].toString();

        // 序号
        m_recordsTable->setItem(i, 0, new QTableWidgetItem(QString::number(i + 1)));
        // 考试时间
        m_recordsTable->setItem(i, 1, new QTableWidgetItem(startTime.toString("yyyy-MM-dd hh:mm:ss")));
        // 得分
        QTableWidgetItem *scoreItem = new QTableWidgetItem(QString::number(score, 'f', 1));
        scoreItem->setTextAlignment(Qt::AlignCenter);
        m_recordsTable->setItem(i, 2, scoreItem);
        // 用时
        QTableWidgetItem *durationItem = new QTableWidgetItem(formatDuration(duration));
        durationItem->setTextAlignment(Qt::AlignCenter);
        m_recordsTable->setItem(i, 3, durationItem);
        // 状态
        QTableWidgetItem *statusItem = new QTableWidgetItem(status);
        statusItem->setTextAlignment(Qt::AlignCenter);
        m_recordsTable->setItem(i, 4, statusItem);

        // 操作按钮
        QPushButton *detailBtn = new QPushButton(QString::fromUtf8("查看详情"), this);
        detailBtn->setProperty("record_index", i);
        detailBtn->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Expanding);
        detailBtn->setStyleSheet(
            "QPushButton {"
            "  background-color: #2196F3;"
            "  color: white;"
            "  border: none;"
            "  padding: 0px 8px;"
            "  border-radius: 4px;"
            "  font-size: 12px;"
            "  min-height: 30px;"
            "}"
            "QPushButton:hover {"
            "  background-color: #1976D2;"
            "}"
        );
        connect(detailBtn, &QPushButton::clicked, this, &StudentMainWindow::onViewDetailsClicked);

        // 把按钮放入带边距的容器，避免单元格内上下被截断
        QWidget *btnContainer = new QWidget(this);
        QHBoxLayout *btnLayout = new QHBoxLayout(btnContainer);
        btnLayout->setContentsMargins(4, 8, 4, 8);
        btnLayout->addWidget(detailBtn);
        m_recordsTable->setCellWidget(i, 5, btnContainer);
    }

    if (m_records.isEmpty()) {
        m_statusLabel->setText(QString::fromUtf8("暂无考试记录，点击“开始考试”进行首次考试。"));
    } else {
        m_statusLabel->setText(QString::fromUtf8("共 %1 条考试记录").arg(m_records.size()));
        m_statusLabel->setStyleSheet("QLabel { color: #4CAF50; font-size: 13px; }");
    }
}

void StudentMainWindow::onStartExamClicked()
{
    emit startExamRequested();
}

void StudentMainWindow::onLogoutClicked()
{
    int ret = QMessageBox::question(this, QString::fromUtf8("确认退出"),
                                    QString::fromUtf8("确定要退出登录吗？"),
                                    QMessageBox::Yes | QMessageBox::No,
                                    QMessageBox::No);
    if (ret == QMessageBox::Yes) {
        emit logoutRequested();
    }
}

void StudentMainWindow::onViewDetailsClicked()
{
    QPushButton *btn = qobject_cast<QPushButton *>(sender());
    if (!btn) {
        return;
    }

    int index = btn->property("record_index").toInt();
    if (index < 0 || index >= m_records.size()) {
        return;
    }

    QJsonObject recordObj = m_records[index].toObject();
    showRecordDetails(recordObj);
}

void StudentMainWindow::showRecordDetails(const QJsonObject &recordObj)
{
    // 构建 ExamRecord
    ExamRecord record = ExamRecord::fromJson(recordObj);

    // 从 question_details 构建 Question 列表
    QList<Question> questions;
    QJsonArray detailsArray = recordObj["question_details"].toArray();
    for (const QJsonValue &val : detailsArray) {
        QJsonObject qObj = val.toObject();
        Question q = Question::fromJson(qObj);

        // 用 user_answer 覆盖 answer，以便 ExamResultDialog 显示考生答案
        // 但正确答案也需要显示，因此这里保留原始 answer，仅把 user_answer 放到 answers map 中
        questions.append(q);
    }

    // 如果 question_details 为空（进行中或未记录详情），则提示
    if (questions.isEmpty()) {
        QMessageBox::information(this, QString::fromUtf8("提示"),
                                 QString::fromUtf8("该考试记录没有详细的题目信息。"));
        return;
    }

    ExamResultDialog dialog(record, questions, this);
    dialog.exec();
}

QString StudentMainWindow::formatDuration(int seconds) const
{
    if (seconds <= 0) {
        return QString::fromUtf8("-");
    }
    int m = seconds / 60;
    int s = seconds % 60;
    return QString::fromUtf8("%1分%2秒").arg(m).arg(s, 2, 10, QChar('0'));
}
