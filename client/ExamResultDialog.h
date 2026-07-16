#ifndef EXAM_RESULT_DIALOG_H
#define EXAM_RESULT_DIALOG_H

#include <QDialog>
#include <QLabel>
#include <QTableWidget>
#include <QPushButton>
#include "ExamRecord.h"
#include "Question.h"

class ExamResultDialog : public QDialog
{
    Q_OBJECT
public:
    explicit ExamResultDialog(const ExamRecord &record,
                              const QList<Question> &questions,
                              QWidget *parent = nullptr);

private:
    void setupUI();
    void loadData();

    ExamRecord m_record;
    QList<Question> m_questions;

    QLabel *m_scoreLabel;
    QLabel *m_infoLabel;
    QTableWidget *m_detailTable;
    QPushButton *m_okBtn;
};

#endif // EXAM_RESULT_DIALOG_H
