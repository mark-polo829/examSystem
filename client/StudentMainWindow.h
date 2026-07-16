#ifndef STUDENT_MAIN_WINDOW_H
#define STUDENT_MAIN_WINDOW_H

#include <QMainWindow>
#include <QTableWidget>
#include <QLabel>
#include <QPushButton>
#include <QJsonObject>
#include <QJsonArray>
#include "User.h"

class StudentMainWindow : public QMainWindow
{
    Q_OBJECT
public:
    explicit StudentMainWindow(const User &user, QWidget *parent = nullptr);

    void refreshRecords();

signals:
    void startExamRequested();
    void logoutRequested();

private slots:
    void onStartExamClicked();
    void onLogoutClicked();
    void onViewDetailsClicked();

private:
    void setupUI();
    void setupConnections();
    void loadRecords();
    void showRecordDetails(const QJsonObject &recordObj);
    QString formatDuration(int seconds) const;

    User m_user;

    QLabel *m_welcomeLabel;
    QLabel *m_statusLabel;
    QTableWidget *m_recordsTable;
    QPushButton *m_startExamBtn;
    QPushButton *m_logoutBtn;

    QJsonArray m_records;
};

#endif // STUDENT_MAIN_WINDOW_H
