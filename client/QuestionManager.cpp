#include "QuestionManager.h"
#include "HttpClient.h"
#include "JsonHelper.h"
#include "Constants.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QMessageBox>
#include <QHeaderView>
#include <QDialog>
#include <QLabel>
#include <QTextEdit>
#include <QGroupBox>
#include <QScrollArea>
#include <QJsonArray>

QuestionManager::QuestionManager(int userId, QWidget *parent)
    : QWidget(parent), m_userId(userId)
{
    setupUI();
    loadCategories();
    loadQuestions();
}

void QuestionManager::setupUI()
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(15);
    mainLayout->setContentsMargins(20, 20, 20, 20);

    // 标题
    QLabel *titleLabel = new QLabel(QString::fromUtf8("题库管理"), this);
    QFont titleFont = titleLabel->font();
    titleFont.setPointSize(16);
    titleFont.setBold(true);
    titleLabel->setFont(titleFont);
    mainLayout->addWidget(titleLabel);

    // 工具栏
    QHBoxLayout *toolbarLayout = new QHBoxLayout();

    m_refreshBtn = new QPushButton(QString::fromUtf8("🔄 刷新"), this);
    m_refreshBtn->setStyleSheet(
        "QPushButton { padding: 8px 15px; background-color: #2196F3; color: white; border: none; border-radius: 4px; }"
        "QPushButton:hover { background-color: #1976D2; }"
    );
    connect(m_refreshBtn, &QPushButton::clicked, this, &QuestionManager::onRefresh);
    toolbarLayout->addWidget(m_refreshBtn);

    m_addBtn = new QPushButton(QString::fromUtf8("➕ 新增题目"), this);
    m_addBtn->setStyleSheet(
        "QPushButton { padding: 8px 15px; background-color: #4CAF50; color: white; border: none; border-radius: 4px; }"
        "QPushButton:hover { background-color: #45a049; }"
    );
    connect(m_addBtn, &QPushButton::clicked, this, &QuestionManager::onAdd);
    toolbarLayout->addWidget(m_addBtn);

    m_editBtn = new QPushButton(QString::fromUtf8("✏️ 编辑"), this);
    m_editBtn->setStyleSheet(
        "QPushButton { padding: 8px 15px; background-color: #FF9800; color: white; border: none; border-radius: 4px; }"
        "QPushButton:hover { background-color: #F57C00; }"
    );
    connect(m_editBtn, &QPushButton::clicked, this, &QuestionManager::onEdit);
    toolbarLayout->addWidget(m_editBtn);

    m_deleteBtn = new QPushButton(QString::fromUtf8("🗑️ 删除"), this);
    m_deleteBtn->setStyleSheet(
        "QPushButton { padding: 8px 15px; background-color: #f44336; color: white; border: none; border-radius: 4px; }"
        "QPushButton:hover { background-color: #d32f2f; }"
    );
    connect(m_deleteBtn, &QPushButton::clicked, this, &QuestionManager::onDelete);
    toolbarLayout->addWidget(m_deleteBtn);

    toolbarLayout->addStretch();

    // 分类筛选
    QLabel *categoryLabel = new QLabel(QString::fromUtf8("分类："), this);
    toolbarLayout->addWidget(categoryLabel);

    m_categoryCombo = new QComboBox(this);
    m_categoryCombo->setMinimumWidth(150);
    connect(m_categoryCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &QuestionManager::onCategoryChanged);
    toolbarLayout->addWidget(m_categoryCombo);

    // 搜索
    m_searchEdit = new QLineEdit(this);
    m_searchEdit->setPlaceholderText(QString::fromUtf8("搜索题目内容..."));
    m_searchEdit->setMinimumWidth(200);
    toolbarLayout->addWidget(m_searchEdit);

    m_searchBtn = new QPushButton(QString::fromUtf8("🔍 搜索"), this);
    m_searchBtn->setStyleSheet(
        "QPushButton { padding: 8px 15px; background-color: #9C27B0; color: white; border: none; border-radius: 4px; }"
        "QPushButton:hover { background-color: #7B1FA2; }"
    );
    connect(m_searchBtn, &QPushButton::clicked, this, &QuestionManager::onSearch);
    toolbarLayout->addWidget(m_searchBtn);

    mainLayout->addLayout(toolbarLayout);

    // 表格
    m_table = new QTableWidget(this);
    m_table->setColumnCount(7);
    m_table->setHorizontalHeaderLabels(QStringList()
        << QString::fromUtf8("ID")
        << QString::fromUtf8("题型")
        << QString::fromUtf8("题目内容")
        << QString::fromUtf8("正确答案")
        << QString::fromUtf8("分类")
        << QString::fromUtf8("难度")
        << QString::fromUtf8("分值"));
    m_table->horizontalHeader()->setStretchLastSection(false);
    m_table->horizontalHeader()->setSectionResizeMode(2, QHeaderView::Stretch);
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->setSelectionMode(QAbstractItemView::SingleSelection);
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_table->setAlternatingRowColors(true);
    m_table->setStyleSheet(
        "QTableWidget {"
        "  border: 1px solid #ddd;"
        "  gridline-color: #eee;"
        "}"
        "QHeaderView::section {"
        "  background-color: #f5f5f5;"
        "  padding: 10px;"
        "  border: 1px solid #ddd;"
        "  font-weight: bold;"
        "}"
    );
    mainLayout->addWidget(m_table, 1);
}

void QuestionManager::loadQuestions()
{
    HttpClient client;
    client.setAuthToken(QString::number(m_userId));

    QString url = Constants::API_BASE + "/questions";
    if (m_categoryCombo->currentIndex() > 0) {
        url += "?category=" + m_categoryCombo->currentText();
    }

    QJsonObject response = client.syncGet(url);

    if (!response["success"].toBool()) {
        QMessageBox::warning(this, QString::fromUtf8("错误"),
                             response["message"].toString());
        return;
    }

    QJsonObject data = response["data"].toObject();
    QJsonArray questionsArray = data["questions"].toArray();

    m_questions.clear();
    m_table->setRowCount(0);

    for (const QJsonValue &val : questionsArray) {
        Question q = Question::fromJson(val.toObject());
        m_questions.append(q);
    }

    // 搜索过滤
    QString searchText = m_searchEdit->text().trimmed();
    if (!searchText.isEmpty()) {
        QList<Question> filtered;
        for (const Question &q : m_questions) {
            if (q.content().contains(searchText, Qt::CaseInsensitive)) {
                filtered.append(q);
            }
        }
        m_questions = filtered;
    }

    m_table->setRowCount(m_questions.size());
    for (int i = 0; i < m_questions.size(); ++i) {
        const Question &q = m_questions[i];
        m_table->setItem(i, 0, new QTableWidgetItem(QString::number(q.id())));
        m_table->setItem(i, 1, new QTableWidgetItem(q.typeString()));
        m_table->setItem(i, 2, new QTableWidgetItem(q.content()));
        m_table->setItem(i, 3, new QTableWidgetItem(q.answer()));
        m_table->setItem(i, 4, new QTableWidgetItem(q.category()));
        m_table->setItem(i, 5, new QTableWidgetItem(q.difficultyString()));
        m_table->setItem(i, 6, new QTableWidgetItem(QString::number(q.score())));
    }
}

void QuestionManager::loadCategories()
{
    HttpClient client;
    client.setAuthToken(QString::number(m_userId));

    QJsonObject response = client.syncGet(Constants::API_BASE + "/categories");

    m_categoryCombo->clear();
    m_categoryCombo->addItem(QString::fromUtf8("全部"));

    if (response["success"].toBool()) {
        QJsonObject data = response["data"].toObject();
        QJsonArray categoriesArray = data["categories"].toArray();
        for (const QJsonValue &val : categoriesArray) {
            m_categoryCombo->addItem(val.toString());
        }
    }
}

void QuestionManager::showQuestionDialog(Question *question)
{
    bool isEdit = (question != nullptr);
    QDialog dialog(this);
    dialog.setWindowTitle(isEdit ? QString::fromUtf8("编辑题目") : QString::fromUtf8("新增题目"));
    dialog.resize(500, 600);

    QVBoxLayout *layout = new QVBoxLayout(&dialog);
    layout->setSpacing(15);
    layout->setContentsMargins(20, 20, 20, 20);

    // 题型
    QHBoxLayout *typeLayout = new QHBoxLayout();
    typeLayout->addWidget(new QLabel(QString::fromUtf8("题型："), &dialog));
    QComboBox *typeCombo = new QComboBox(&dialog);
    typeCombo->addItem(QString::fromUtf8("单选题"), static_cast<int>(Constants::TypeSingleChoice));
    typeCombo->addItem(QString::fromUtf8("多选题"), static_cast<int>(Constants::TypeMultipleChoice));
    typeCombo->addItem(QString::fromUtf8("判断题"), static_cast<int>(Constants::TypeTrueFalse));
    typeLayout->addWidget(typeCombo);
    typeLayout->addStretch();
    layout->addLayout(typeLayout);

    // 分类
    QHBoxLayout *catLayout = new QHBoxLayout();
    catLayout->addWidget(new QLabel(QString::fromUtf8("分类："), &dialog));
    QLineEdit *categoryEdit = new QLineEdit(&dialog);
    categoryEdit->setPlaceholderText(QString::fromUtf8("请输入分类"));
    catLayout->addWidget(categoryEdit);
    layout->addLayout(catLayout);

    // 难度
    QHBoxLayout *diffLayout = new QHBoxLayout();
    diffLayout->addWidget(new QLabel(QString::fromUtf8("难度："), &dialog));
    QComboBox *diffCombo = new QComboBox(&dialog);
    diffCombo->addItem(QString::fromUtf8("易"), static_cast<int>(Constants::DiffEasy));
    diffCombo->addItem(QString::fromUtf8("中"), static_cast<int>(Constants::DiffMedium));
    diffCombo->addItem(QString::fromUtf8("难"), static_cast<int>(Constants::DiffHard));
    diffLayout->addWidget(diffCombo);
    diffLayout->addStretch();
    layout->addLayout(diffLayout);

    // 题目内容
    layout->addWidget(new QLabel(QString::fromUtf8("题目内容："), &dialog));
    QTextEdit *contentEdit = new QTextEdit(&dialog);
    contentEdit->setPlaceholderText(QString::fromUtf8("请输入题目内容"));
    contentEdit->setMinimumHeight(80);
    layout->addWidget(contentEdit);

    // 选项
    layout->addWidget(new QLabel(QString::fromUtf8("选项（每行一个，格式如：A.选项内容）："), &dialog));
    QTextEdit *optionsEdit = new QTextEdit(&dialog);
    optionsEdit->setPlaceholderText(QString::fromUtf8("A.选项1\nB.选项2\nC.选项3\nD.选项4"));
    optionsEdit->setMinimumHeight(100);
    layout->addWidget(optionsEdit);

    // 正确答案
    layout->addWidget(new QLabel(QString::fromUtf8("正确答案："), &dialog));
    QLineEdit *answerEdit = new QLineEdit(&dialog);
    answerEdit->setPlaceholderText(QString::fromUtf8("单选/判断填A/B，多选填ABC"));
    layout->addWidget(answerEdit);

    // 按钮
    QHBoxLayout *btnLayout = new QHBoxLayout();
    btnLayout->addStretch();
    QPushButton *okBtn = new QPushButton(QString::fromUtf8("确定"), &dialog);
    QPushButton *cancelBtn = new QPushButton(QString::fromUtf8("取消"), &dialog);
    btnLayout->addWidget(okBtn);
    btnLayout->addWidget(cancelBtn);
    btnLayout->addStretch();
    layout->addLayout(btnLayout);

    connect(okBtn, &QPushButton::clicked, &dialog, &QDialog::accept);
    connect(cancelBtn, &QPushButton::clicked, &dialog, &QDialog::reject);

    // 如果是编辑，填充数据
    if (isEdit) {
        typeCombo->setCurrentIndex(static_cast<int>(question->type()));
        categoryEdit->setText(question->category());
        diffCombo->setCurrentIndex(static_cast<int>(question->difficulty()));
        contentEdit->setPlainText(question->content());

        QStringList options = question->options();
        optionsEdit->setPlainText(options.join("\n"));
        answerEdit->setText(question->answer());
    }

    if (dialog.exec() != QDialog::Accepted) {
        return;
    }

    // 保存数据
    Question q;
    if (isEdit) {
        q.setId(question->id());
    }
    q.setType(static_cast<Constants::QuestionType>(typeCombo->currentIndex()));
    q.setCategory(categoryEdit->text().trimmed());
    q.setDifficulty(static_cast<Constants::Difficulty>(diffCombo->currentIndex()));
    q.setContent(contentEdit->toPlainText().trimmed());

    QStringList options = optionsEdit->toPlainText().split("\n", QString::SkipEmptyParts);
    q.setOptions(options);
    q.setAnswer(answerEdit->text().trimmed().toUpper());

    HttpClient client;
    client.setAuthToken(QString::number(m_userId));

    QJsonObject response;
    if (isEdit) {
        response = client.syncPut(Constants::API_BASE + "/questions/" + QString::number(q.id()), q.toJson());
    } else {
        response = client.syncPost(Constants::API_BASE + "/questions", q.toJson());
    }

    if (response["success"].toBool()) {
        QMessageBox::information(this, QString::fromUtf8("成功"),
                                 isEdit ? QString::fromUtf8("题目更新成功") : QString::fromUtf8("题目添加成功"));
        loadQuestions();
        loadCategories();
    } else {
        QMessageBox::warning(this, QString::fromUtf8("失败"),
                             response["message"].toString());
    }
}

void QuestionManager::onRefresh()
{
    loadQuestions();
}

void QuestionManager::onAdd()
{
    showQuestionDialog();
}

void QuestionManager::onEdit()
{
    int row = m_table->currentRow();
    if (row < 0) {
        QMessageBox::warning(this, QString::fromUtf8("提示"),
                             QString::fromUtf8("请先选择一道题目"));
        return;
    }
    showQuestionDialog(&m_questions[row]);
}

void QuestionManager::onDelete()
{
    int row = m_table->currentRow();
    if (row < 0) {
        QMessageBox::warning(this, QString::fromUtf8("提示"),
                             QString::fromUtf8("请先选择一道题目"));
        return;
    }

    int id = m_questions[row].id();
    QString content = m_questions[row].content();

    int ret = QMessageBox::question(this, QString::fromUtf8("确认删除"),
                                   QString::fromUtf8("确定要删除题目「%1」吗？").arg(content),
                                   QMessageBox::Yes | QMessageBox::No,
                                   QMessageBox::No);
    if (ret != QMessageBox::Yes) {
        return;
    }

    HttpClient client;
    client.setAuthToken(QString::number(m_userId));

    QJsonObject response = client.syncDelete(Constants::API_BASE + "/questions/" + QString::number(id));

    if (response["success"].toBool()) {
        QMessageBox::information(this, QString::fromUtf8("成功"),
                                 QString::fromUtf8("题目删除成功"));
        loadQuestions();
    } else {
        QMessageBox::warning(this, QString::fromUtf8("失败"),
                             response["message"].toString());
    }
}

void QuestionManager::onSearch()
{
    loadQuestions();
}

void QuestionManager::onCategoryChanged(int index)
{
    Q_UNUSED(index)
    loadQuestions();
}
