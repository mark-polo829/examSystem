#ifndef QUESTION_MANAGER_H
#define QUESTION_MANAGER_H

#include <QWidget>
#include <QTableWidget>
#include <QPushButton>
#include <QLineEdit>
#include <QComboBox>
#include <QList>
#include "Question.h"

class QuestionManager : public QWidget
{
    Q_OBJECT
public:
    explicit QuestionManager(int userId, QWidget *parent = nullptr);

private slots:
    void onRefresh();
    void onAdd();
    void onEdit();
    void onDelete();
    void onSearch();
    void onCategoryChanged(int index);

private:
    void setupUI();
    void loadQuestions();
    void loadCategories();
    void showQuestionDialog(Question *question = nullptr);

    int m_userId;

    QTableWidget *m_table;
    QPushButton *m_addBtn;
    QPushButton *m_editBtn;
    QPushButton *m_deleteBtn;
    QPushButton *m_refreshBtn;
    QLineEdit *m_searchEdit;
    QPushButton *m_searchBtn;
    QComboBox *m_categoryCombo;

    QList<Question> m_questions;
};

#endif // QUESTION_MANAGER_H
