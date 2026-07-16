#ifndef ADMIN_MAIN_WINDOW_H
#define ADMIN_MAIN_WINDOW_H

#include <QMainWindow>
#include <QStackedWidget>
#include <QListWidget>
#include <QLabel>
#include "User.h"

class QuestionManager;
class ScoreStatistics;
class UserManager;

class AdminMainWindow : public QMainWindow
{
    Q_OBJECT
public:
    explicit AdminMainWindow(const User &user, QWidget *parent = nullptr);

signals:
    void logoutRequested();

private slots:
    void onMenuSelected(int index);
    void onLogout();

private:
    void setupUI();
    void setupMenu();

    User m_user;

    QWidget *m_centralWidget;
    QListWidget *m_menuList;
    QStackedWidget *m_contentStack;
    QLabel *m_statusLabel;
    QLabel *m_userInfoLabel;

    QuestionManager *m_questionManager;
    ScoreStatistics *m_scoreStatistics;
    UserManager *m_userManager;
};

#endif // ADMIN_MAIN_WINDOW_H
