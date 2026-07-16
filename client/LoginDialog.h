#ifndef LOGIN_DIALOG_H
#define LOGIN_DIALOG_H

#include <QDialog>
#include <QLineEdit>
#include <QComboBox>
#include <QPushButton>
#include <QLabel>
#include <QCheckBox>
#include "Constants.h"
#include "User.h"

class LoginDialog : public QDialog
{
    Q_OBJECT
public:
    explicit LoginDialog(QWidget *parent = nullptr);

    User loggedInUser() const { return m_user; }
    Constants::UserRole selectedRole() const;

signals:
    void loginSuccess(const User &user);

private slots:
    void onLoginClicked();
    void onRegisterClicked();

private:
    void setupUI();
    void setupConnections();

    QLineEdit *m_usernameEdit;
    QLineEdit *m_passwordEdit;
    QComboBox *m_roleCombo;
    QPushButton *m_loginBtn;
    QPushButton *m_registerBtn;
    QCheckBox *m_rememberCheck;
    QLabel *m_statusLabel;

    User m_user;
};

#endif // LOGIN_DIALOG_H
