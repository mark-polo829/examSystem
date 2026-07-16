#include "QuestionWidget.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QButtonGroup>
#include <QGroupBox>
#include <QVariant>

QuestionWidget::QuestionWidget(const Question &question, QWidget *parent)
    : QWidget(parent), m_question(question)
{
    setupUI();
}

void QuestionWidget::setupUI()
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(15);
    mainLayout->setContentsMargins(20, 20, 20, 20);

    // 题型标签
    m_typeLabel = new QLabel(QString("[%1] %2").arg(m_question.typeString()).arg(m_question.difficultyString()), this);
    m_typeLabel->setStyleSheet(
        "QLabel {"
        "  color: #666;"
        "  font-size: 12px;"
        "  padding: 5px;"
        "  background-color: #f0f0f0;"
        "  border-radius: 3px;"
        "}"
    );
    mainLayout->addWidget(m_typeLabel);

    // 题目内容
    m_contentLabel = new QLabel(this);
    m_contentLabel->setWordWrap(true);
    m_contentLabel->setTextFormat(Qt::RichText);

    // 格式化题目内容
    QString content = QString("<h3>%1</h3>").arg(m_question.content());
    m_contentLabel->setText(content);
    m_contentLabel->setStyleSheet("QLabel { font-size: 14px; line-height: 1.6; }");
    mainLayout->addWidget(m_contentLabel);

    mainLayout->addSpacing(10);

    // 选项区域
    m_optionsWidget = new QWidget(this);
    QVBoxLayout *optionsLayout = new QVBoxLayout(m_optionsWidget);
    optionsLayout->setSpacing(10);

    m_buttonGroup = new QButtonGroup(this);

    if (m_question.type() == Constants::TypeMultipleChoice) {
        // 多选题：使用 CheckBox
        for (int i = 0; i < m_question.options().size(); ++i) {
            QCheckBox *checkBox = new QCheckBox(m_question.options()[i], this);
            checkBox->setProperty("option_value", QVariant(QString(QChar('A' + i))));
            checkBox->setStyleSheet(
                "QCheckBox {"
                "  font-size: 14px;"
                "  padding: 8px;"
                "  spacing: 10px;"
                "}"
                "QCheckBox::indicator {"
                "  width: 20px;"
                "  height: 20px;"
                "}"
            );
            m_checkBoxes.append(checkBox);
            optionsLayout->addWidget(checkBox);
            connect(checkBox, &QCheckBox::stateChanged, this, &QuestionWidget::onMultipleChoiceChanged);
        }
    } else {
        // 单选题/判断题：使用 RadioButton
        for (int i = 0; i < m_question.options().size(); ++i) {
            QRadioButton *radioBtn = new QRadioButton(m_question.options()[i], this);
            radioBtn->setProperty("option_value", QVariant(QString(QChar('A' + i))));
            radioBtn->setStyleSheet(
                "QRadioButton {"
                "  font-size: 14px;"
                "  padding: 8px;"
                "  spacing: 10px;"
                "}"
                "QRadioButton::indicator {"
                "  width: 20px;"
                "  height: 20px;"
                "}"
            );
            m_radioButtons.append(radioBtn);
            m_buttonGroup->addButton(radioBtn, i);
            optionsLayout->addWidget(radioBtn);
        }

        if (m_question.type() == Constants::TypeTrueFalse) {
            connect(m_buttonGroup, QOverload<int>::of(&QButtonGroup::buttonClicked),
                    this, &QuestionWidget::onTrueFalseChanged);
        } else {
            connect(m_buttonGroup, QOverload<int>::of(&QButtonGroup::buttonClicked),
                    this, &QuestionWidget::onSingleChoiceChanged);
        }
    }

    mainLayout->addWidget(m_optionsWidget);
    mainLayout->addStretch();

    // 分值提示
    QLabel *scoreLabel = new QLabel(QString::fromUtf8("本题分值：%1分").arg(m_question.score()), this);
    scoreLabel->setStyleSheet("QLabel { color: #999; font-size: 12px; }");
    mainLayout->addWidget(scoreLabel);
}

QString QuestionWidget::getAnswer() const
{
    if (m_question.type() == Constants::TypeMultipleChoice) {
        QStringList selected;
        for (QCheckBox *checkBox : m_checkBoxes) {
            if (checkBox->isChecked()) {
                selected.append(checkBox->property("option_value").toString());
            }
        }
        std::sort(selected.begin(), selected.end());
        return selected.join("");
    } else {
        for (QRadioButton *radioBtn : m_radioButtons) {
            if (radioBtn->isChecked()) {
                return radioBtn->property("option_value").toString();
            }
        }
    }
    return QString();
}

void QuestionWidget::setAnswer(const QString &answer)
{
    if (m_question.type() == Constants::TypeMultipleChoice) {
        for (QCheckBox *checkBox : m_checkBoxes) {
            QString opt = checkBox->property("option_value").toString();
            checkBox->setChecked(answer.contains(opt));
        }
    } else {
        for (QRadioButton *radioBtn : m_radioButtons) {
            QString opt = radioBtn->property("option_value").toString();
            if (opt == answer) {
                radioBtn->setChecked(true);
                break;
            }
        }
    }
}

void QuestionWidget::clearAnswer()
{
    if (m_question.type() == Constants::TypeMultipleChoice) {
        for (QCheckBox *checkBox : m_checkBoxes) {
            checkBox->setChecked(false);
        }
    } else {
        for (QRadioButton *radioBtn : m_radioButtons) {
            radioBtn->setAutoExclusive(false);
            radioBtn->setChecked(false);
            radioBtn->setAutoExclusive(true);
        }
    }
}

void QuestionWidget::onSingleChoiceChanged(int id)
{
    Q_UNUSED(id)
    emit answerChanged(m_question.id(), getAnswer());
}

void QuestionWidget::onMultipleChoiceChanged()
{
    emit answerChanged(m_question.id(), getAnswer());
}

void QuestionWidget::onTrueFalseChanged(int id)
{
    Q_UNUSED(id)
    emit answerChanged(m_question.id(), getAnswer());
}
