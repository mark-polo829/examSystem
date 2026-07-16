#include "LoginDialog.h"
#include "HttpClient.h"
#include "JsonHelper.h"
#include "Constants.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QMessageBox>
#include <QCryptographicHash>
#include <QFile>
#include <QTextStream>
#include <QDateTime>

LoginDialog::LoginDialog(QWidget *parent)
    : QDialog(parent), m_user()
{
    setWindowTitle(QString::fromUtf8("在线考试系统 - 登录"));
    setFixedSize(480, 420);
    setupUI();
    setupConnections();
}

void LoginDialog::setupUI()
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(15);
    mainLayout->setContentsMargins(40, 30, 40, 30);

    // 标题
    QLabel *titleLabel = new QLabel(QString::fromUtf8("在线考试系统"), this);
    QFont titleFont = titleLabel->font();
    titleFont.setPointSize(20);
    titleFont.setBold(true);
    titleLabel->setFont(titleFont);
    titleLabel->setAlignment(Qt::AlignCenter);
    mainLayout->addWidget(titleLabel);

    // 副标题
    QLabel *subTitleLabel = new QLabel(QString::fromUtf8("用户登录"), this);
    QFont subFont = subTitleLabel->font();
    subFont.setPointSize(12);
    subTitleLabel->setFont(subFont);
    subTitleLabel->setAlignment(Qt::AlignCenter);
    mainLayout->addWidget(subTitleLabel);

    mainLayout->addSpacing(25);

    // 表单区域
    QGridLayout *formLayout = new QGridLayout();
    formLayout->setColumnStretch(0, 0);
    formLayout->setColumnStretch(1, 1);
    formLayout->setHorizontalSpacing(15);
    formLayout->setVerticalSpacing(20);
    formLayout->setContentsMargins(10, 10, 10, 10);

    // 统一标签样式
    QFont labelFont;
    labelFont.setPointSize(12);
    labelFont.setBold(true);

    QString labelStyle = "QLabel { font-size: 12px; font-weight: bold; color: #333; }";

    // QLineEdit 样式
    QString lineEditStyle =
        "QLineEdit {"
        "  font-size: 13px;"
        "  padding: 6px 10px;"
        "  border: 1px solid #bbb;"
        "  border-radius: 6px;"
        "  background-color: white;"
        "}"
        "QLineEdit:focus {"
        "  border: 2px solid #2196F3;"
        "}"
        "QLineEdit:hover {"
        "  border: 1px solid #888;"
        "}";

    // QComboBox 样式（含下拉列表 item 悬浮/选中样式，避免白条遮挡文字）
    QString comboStyle =
        "QComboBox {"
        "  font-size: 13px;"
        "  padding: 6px 10px;"
        "  border: 1px solid #bbb;"
        "  border-radius: 6px;"
        "  background-color: white;"
        "}"
        "QComboBox:focus {"
        "  border: 2px solid #2196F3;"
        "}"
        "QComboBox:hover {"
        "  border: 1px solid #888;"
        "}"
        "QComboBox::drop-down {"
        "  subcontrol-origin: padding;"
        "  subcontrol-position: top right;"
        "  width: 24px;"
        "  border-left: 1px solid #ccc;"
        "  border-top-right-radius: 6px;"
        "  border-bottom-right-radius: 6px;"
        "}"
        "QComboBox::item {"
        "  height: 28px;"
        "  padding: 4px 10px;"
        "  color: #333;"
        "  background-color: transparent;"
        "}"
        "QComboBox::item:hover {"
        "  background-color: #2196F3;"
        "  color: white;"
        "}"
        "QComboBox::item:selected {"
        "  background-color: #2196F3;"
        "  color: white;"
        "}";

    // 用户名
    QLabel *userLabel = new QLabel(QString::fromUtf8("用户名："), this);
    userLabel->setFont(labelFont);
    userLabel->setStyleSheet(labelStyle);
    userLabel->setFixedSize(80, 32);
    userLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    m_usernameEdit = new QLineEdit(this);
    m_usernameEdit->setPlaceholderText(QString::fromUtf8("请输入用户名"));
    m_usernameEdit->setText("student");
    m_usernameEdit->setStyleSheet(lineEditStyle);
    m_usernameEdit->setFixedHeight(32);
    formLayout->addWidget(userLabel, 0, 0);
    formLayout->addWidget(m_usernameEdit, 0, 1);

    // 密码
    QLabel *pwdLabel = new QLabel(QString::fromUtf8("密  码："), this);
    pwdLabel->setFont(labelFont);
    pwdLabel->setStyleSheet(labelStyle);
    pwdLabel->setFixedSize(80, 32);
    pwdLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    m_passwordEdit = new QLineEdit(this);
    m_passwordEdit->setPlaceholderText(QString::fromUtf8("请输入密码"));
    m_passwordEdit->setEchoMode(QLineEdit::Password);
    m_passwordEdit->setText("student123");
    m_passwordEdit->setStyleSheet(lineEditStyle);
    m_passwordEdit->setFixedHeight(32);
    formLayout->addWidget(pwdLabel, 1, 0);
    formLayout->addWidget(m_passwordEdit, 1, 1);

    // 角色选择
    QLabel *roleLabel = new QLabel(QString::fromUtf8("角  色："), this);
    roleLabel->setFont(labelFont);
    roleLabel->setStyleSheet(labelStyle);
    roleLabel->setFixedSize(80, 32);
    roleLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    m_roleCombo = new QComboBox(this);
    m_roleCombo->addItem(QString::fromUtf8("考生"), static_cast<int>(Constants::RoleStudent));
    m_roleCombo->addItem(QString::fromUtf8("教师"), static_cast<int>(Constants::RoleTeacher));
    m_roleCombo->addItem(QString::fromUtf8("管理员"), static_cast<int>(Constants::RoleAdmin));
    m_roleCombo->setCurrentIndex(0);
    m_roleCombo->setStyleSheet(comboStyle);
    m_roleCombo->setFixedHeight(32);
    formLayout->addWidget(roleLabel, 2, 0);
    formLayout->addWidget(m_roleCombo, 2, 1);

    mainLayout->addLayout(formLayout);

    mainLayout->addSpacing(15);

    // 记住密码
    m_rememberCheck = new QCheckBox(QString::fromUtf8("记住密码"), this);
    QFont checkFont = m_rememberCheck->font();
    checkFont.setPointSize(11);
    m_rememberCheck->setFont(checkFont);
    mainLayout->addWidget(m_rememberCheck, 0, Qt::AlignCenter);

    // 状态标签
    m_statusLabel = new QLabel(this);
    m_statusLabel->setAlignment(Qt::AlignCenter);
    m_statusLabel->setStyleSheet("color: red; font-size: 12px;");
    m_statusLabel->setMinimumHeight(20);
    mainLayout->addWidget(m_statusLabel);

    mainLayout->addStretch();

    // 按钮区域
    QHBoxLayout *btnLayout = new QHBoxLayout();
    btnLayout->setSpacing(25);
    m_loginBtn = new QPushButton(QString::fromUtf8("登 录"), this);
    m_loginBtn->setDefault(true);
    m_loginBtn->setFixedSize(120, 40);
    m_loginBtn->setStyleSheet(
        "QPushButton {"
        "  background-color: #4CAF50;"
        "  color: white;"
        "  border: none;"
        "  padding: 8px 24px;"
        "  font-size: 15px;"
        "  font-weight: bold;"
        "  border-radius: 6px;"
        "}"
        "QPushButton:hover {"
        "  background-color: #45a049;"
        "}"
    );

    m_registerBtn = new QPushButton(QString::fromUtf8("注 册"), this);
    m_registerBtn->setFixedSize(120, 40);
    m_registerBtn->setStyleSheet(
        "QPushButton {"
        "  background-color: #2196F3;"
        "  color: white;"
        "  border: none;"
        "  padding: 8px 24px;"
        "  font-size: 15px;"
        "  font-weight: bold;"
        "  border-radius: 6px;"
        "}"
        "QPushButton:hover {"
        "  background-color: #0b7dda;"
        "}"
    );

    btnLayout->addStretch();
    btnLayout->addWidget(m_loginBtn);
    btnLayout->addWidget(m_registerBtn);
    btnLayout->addStretch();

    mainLayout->addLayout(btnLayout);
}

void LoginDialog::setupConnections()
{
    connect(m_loginBtn, &QPushButton::clicked, this, &LoginDialog::onLoginClicked);
    connect(m_registerBtn, &QPushButton::clicked, this, &LoginDialog::onRegisterClicked);
    connect(m_passwordEdit, &QLineEdit::returnPressed, this, &LoginDialog::onLoginClicked);
}

Constants::UserRole LoginDialog::selectedRole() const
{
    // 直接使用当前选中项的索引，避免 QVariant 转换异常导致恒为 0
    return static_cast<Constants::UserRole>(m_roleCombo->currentIndex());
}

void LoginDialog::onLoginClicked()
{
    QString username = m_usernameEdit->text().trimmed();
    QString password = m_passwordEdit->text().trimmed();

    if (username.isEmpty() || password.isEmpty()) {
        m_statusLabel->setText(QString::fromUtf8("请输入用户名和密码"));
        return;
    }

    m_loginBtn->setEnabled(false);
    m_statusLabel->setText(QString::fromUtf8("登录中..."));
    m_statusLabel->setStyleSheet("color: blue;");

    // 发送登录请求
    HttpClient client;
    QJsonObject loginData;
    loginData["username"] = username;
    loginData["password"] = password;

    QJsonObject response = client.syncPost(Constants::API_BASE + "/login", loginData);

    m_loginBtn->setEnabled(true);

    if (!response["success"].toBool()) {
        m_statusLabel->setText(response["message"].toString());
        m_statusLabel->setStyleSheet("color: red;");
        return;
    }

    QJsonObject data = response["data"].toObject();
    m_user = User::fromJson(data);

    // 调试输出，帮助定位角色不匹配问题
    QString logMsg = QString("[%1] username=%2 selectedRole=%3 serverRole=%4 response=%5\n")
                         .arg(QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss"))
                         .arg(username)
                         .arg(selectedRole())
                         .arg(m_user.role())
                         .arg(QString::fromUtf8(QJsonDocument(response).toJson(QJsonDocument::Compact)));
    qDebug().noquote() << logMsg;
    QFile logFile("login_debug.log");
    if (logFile.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text)) {
        QTextStream out(&logFile);
        out << logMsg;
        logFile.close();
    }

    // 验证角色
    if (m_user.role() != selectedRole()) {
        m_statusLabel->setText(QString::fromUtf8("角色不匹配，请选择正确的角色"));
        m_statusLabel->setStyleSheet("color: red;");
        m_user = User();  // 清空
        return;
    }

    m_statusLabel->setText(QString::fromUtf8("登录成功！"));
    m_statusLabel->setStyleSheet("color: green;");

    emit loginSuccess(m_user);
    accept();
}

void LoginDialog::onRegisterClicked()
{
    QMessageBox::information(this, QString::fromUtf8("提示"),
                             QString::fromUtf8("请联系管理员添加用户。\n默认账号：\n考生：student/student123\n教师：teacher/teacher123\n管理员：admin/admin123"));
}
