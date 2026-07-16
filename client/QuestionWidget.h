#ifndef QUESTION_WIDGET_H
#define QUESTION_WIDGET_H

#include <QWidget>
#include <QButtonGroup>
#include <QRadioButton>
#include <QCheckBox>
#include <QLabel>
#include <QVBoxLayout>
#include "Question.h"

class QuestionWidget : public QWidget
{
    Q_OBJECT
public:
    explicit QuestionWidget(const Question &question, QWidget *parent = nullptr);

    QString getAnswer() const;
    void setAnswer(const QString &answer);
    void clearAnswer();

    int questionId() const { return m_question.id(); }

signals:
    void answerChanged(int questionId, const QString &answer);

private slots:
    void onSingleChoiceChanged(int id);
    void onMultipleChoiceChanged();
    void onTrueFalseChanged(int id);

private:
    void setupUI();

    Question m_question;
    QButtonGroup *m_buttonGroup;
    QList<QRadioButton*> m_radioButtons;
    QList<QCheckBox*> m_checkBoxes;
    QLabel *m_contentLabel;
    QLabel *m_typeLabel;
    QWidget *m_optionsWidget;
};

#endif // QUESTION_WIDGET_H
