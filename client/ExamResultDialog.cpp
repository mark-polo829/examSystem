#include "ExamResultDialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QFont>

ExamResultDialog::ExamResultDialog(const ExamRecord &record,
                                   const QList<Question> &questions,
                                   QWidget *parent)
    : QDialog(parent), m_record(record), m_questions(questions)
{
    setWindowTitle(QString::fromUtf8("考试结果"));
    resize(700, 500);
    setupUI();
    loadData();
}

void ExamResultDialog::setupUI()
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(15);
    mainLayout->setContentsMargins(20, 20, 20, 20);

    // 标题
    QLabel *titleLabel = new QLabel(QString::fromUtf8("🎓 考试结果"), this);
    QFont titleFont = titleLabel->font();
    titleFont.setPointSize(20);
    titleFont.setBold(true);
    titleLabel->setFont(titleFont);
    titleLabel->setAlignment(Qt::AlignCenter);
    mainLayout->addWidget(titleLabel);

    // 分数显示
    QFrame *scoreFrame = new QFrame(this);
    scoreFrame->setStyleSheet(
        "QFrame {"
        "  background-color: #e3f2fd;"
        "  border-radius: 10px;"
        "  padding: 20px;"
        "}"
    );
    QHBoxLayout *scoreLayout = new QHBoxLayout(scoreFrame);

    m_scoreLabel = new QLabel(this);
    QFont scoreFont = m_scoreLabel->font();
    scoreFont.setPointSize(36);
    scoreFont.setBold(true);
    m_scoreLabel->setFont(scoreFont);
    m_scoreLabel->setAlignment(Qt::AlignCenter);
    scoreLayout->addWidget(m_scoreLabel);

    m_infoLabel = new QLabel(this);
    m_infoLabel->setAlignment(Qt::AlignCenter);
    m_infoLabel->setStyleSheet("QLabel { font-size: 14px; color: #666; }");
    scoreLayout->addWidget(m_infoLabel);

    mainLayout->addWidget(scoreFrame);

    // 详情表格
    QLabel *detailTitle = new QLabel(QString::fromUtf8("答题详情"), this);
    detailTitle->setStyleSheet("QLabel { font-size: 14px; font-weight: bold; }");
    mainLayout->addWidget(detailTitle);

    m_detailTable = new QTableWidget(this);
    m_detailTable->setColumnCount(5);
    m_detailTable->setHorizontalHeaderLabels(QStringList()
        << QString::fromUtf8("题号")
        << QString::fromUtf8("题型")
        << QString::fromUtf8("你的答案")
        << QString::fromUtf8("正确答案")
        << QString::fromUtf8("结果"));
    m_detailTable->horizontalHeader()->setStretchLastSection(true);
    m_detailTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    m_detailTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_detailTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_detailTable->setAlternatingRowColors(true);
    m_detailTable->setStyleSheet(
        "QTableWidget {"
        "  border: 1px solid #ddd;"
        "  gridline-color: #eee;"
        "}"
        "QHeaderView::section {"
        "  background-color: #f5f5f5;"
        "  padding: 8px;"
        "  border: 1px solid #ddd;"
        "  font-weight: bold;"
        "}"
    );
    mainLayout->addWidget(m_detailTable, 1);

    // 确定按钮
    m_okBtn = new QPushButton(QString::fromUtf8("确定"), this);
    m_okBtn->setStyleSheet(
        "QPushButton {"
        "  padding: 10px 40px;"
        "  font-size: 14px;"
        "  background-color: #4CAF50;"
        "  color: white;"
        "  border: none;"
        "  border-radius: 4px;"
        "}"
        "QPushButton:hover { background-color: #45a049; }"
    );
    connect(m_okBtn, &QPushButton::clicked, this, &QDialog::accept);

    QHBoxLayout *btnLayout = new QHBoxLayout();
    btnLayout->addStretch();
    btnLayout->addWidget(m_okBtn);
    btnLayout->addStretch();
    mainLayout->addLayout(btnLayout);
}

void ExamResultDialog::loadData()
{
    // 设置分数
    m_scoreLabel->setText(QString::number(m_record.score(), 'f', 1));

    // 根据分数设置颜色
    if (m_record.score() >= 90) {
        m_scoreLabel->setStyleSheet("QLabel { color: #4CAF50; }");
    } else if (m_record.score() >= 60) {
        m_scoreLabel->setStyleSheet("QLabel { color: #FF9800; }");
    } else {
        m_scoreLabel->setStyleSheet("QLabel { color: #f44336; }");
    }

    // 设置信息
    int duration = m_record.duration();
    int minutes = duration / 60;
    int seconds = duration % 60;
    m_infoLabel->setText(QString::fromUtf8("用时：%1分%2秒 | %3")
                         .arg(minutes).arg(seconds).arg(m_record.statusString()));

    // 填充详情表格
    QMap<int, QString> answers = m_record.answers();
    m_detailTable->setRowCount(m_questions.size());

    for (int i = 0; i < m_questions.size(); ++i) {
        const Question &q = m_questions[i];
        QString userAnswer = answers.value(q.id(), QString::fromUtf8("未作答"));
        bool isCorrect = q.checkAnswer(userAnswer);

        m_detailTable->setItem(i, 0, new QTableWidgetItem(QString::number(i + 1)));
        m_detailTable->setItem(i, 1, new QTableWidgetItem(q.typeString()));
        m_detailTable->setItem(i, 2, new QTableWidgetItem(userAnswer));
        m_detailTable->setItem(i, 3, new QTableWidgetItem(q.answer()));

        QTableWidgetItem *resultItem = new QTableWidgetItem(isCorrect ?
            QString::fromUtf8("✓ 正确") : QString::fromUtf8("✗ 错误"));
        resultItem->setForeground(isCorrect ? QColor("#4CAF50") : QColor("#f44336"));
        m_detailTable->setItem(i, 4, resultItem);
    }
}
