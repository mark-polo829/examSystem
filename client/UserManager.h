#ifndef USER_MANAGER_H
#define USER_MANAGER_H

#include <QWidget>
#include <QTableWidget>
#include <QPushButton>
#include <QLineEdit>
#include <QComboBox>
#include <QList>
#include "User.h"

class UserManager : public QWidget
{
    Q_OBJECT
public:
    explicit UserManager(int userId, QWidget *parent = nullptr);

private slots:
    void onRefresh();
    void onAdd();
    void onEdit();
    void onDelete();
    void onResetPassword();

private:
    void setupUI();
    void loadUsers();
    void showUserDialog(User *user = nullptr);

    int m_userId;

    QTableWidget *m_table;
    QPushButton *m_addBtn;
    QPushButton *m_editBtn;
    QPushButton *m_deleteBtn;
    QPushButton *m_resetPwdBtn;
    QPushButton *m_refreshBtn;

    QList<User> m_users;
};

#endif // USER_MANAGER_H
