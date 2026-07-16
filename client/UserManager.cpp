#include "UserManager.h"
#include "HttpClient.h"
#include "JsonHelper.h"
#include "Constants.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QMessageBox>
#include <QDialog>
#include <QGridLayout>
#include <QJsonArray>
#include <QLabel>

UserManager::UserManager(int userId, QWidget *parent)
    : QWidget(parent), m_userId(userId)
{
    setupUI();
    loadUsers();
}

void UserManager::setupUI()
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(15);
    mainLayout->setContentsMargins(20, 20, 20, 20);

    // 标题
    QLabel *titleLabel = new QLabel(QString::fromUtf8("用户管理"), this);
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
    connect(m_refreshBtn, &QPushButton::clicked, this, &UserManager::onRefresh);
    toolbarLayout->addWidget(m_refreshBtn);

    m_addBtn = new QPushButton(QString::fromUtf8("➕ 新增用户"), this);
    m_addBtn->setStyleSheet(
        "QPushButton { padding: 8px 15px; background-color: #4CAF50; color: white; border: none; border-radius: 4px; }"
        "QPushButton:hover { background-color: #45a049; }"
    );
    connect(m_addBtn, &QPushButton::clicked, this, &UserManager::onAdd);
    toolbarLayout->addWidget(m_addBtn);

    m_editBtn = new QPushButton(QString::fromUtf8("✏️ 编辑"), this);
    m_editBtn->setStyleSheet(
        "QPushButton { padding: 8px 15px; background-color: #FF9800; color: white; border: none; border-radius: 4px; }"
        "QPushButton:hover { background-color: #F57C00; }"
    );
    connect(m_editBtn, &QPushButton::clicked, this, &UserManager::onEdit);
    toolbarLayout->addWidget(m_editBtn);

    m_deleteBtn = new QPushButton(QString::fromUtf8("🗑️ 删除"), this);
    m_deleteBtn->setStyleSheet(
        "QPushButton { padding: 8px 15px; background-color: #f44336; color: white; border: none; border-radius: 4px; }"
        "QPushButton:hover { background-color: #d32f2f; }"
    );
    connect(m_deleteBtn, &QPushButton::clicked, this, &UserManager::onDelete);
    toolbarLayout->addWidget(m_deleteBtn);

    m_resetPwdBtn = new QPushButton(QString::fromUtf8("🔑 重置密码"), this);
    m_resetPwdBtn->setStyleSheet(
        "QPushButton { padding: 8px 15px; background-color: #9C27B0; color: white; border: none; border-radius: 4px; }"
        "QPushButton:hover { background-color: #7B1FA2; }"
    );
    connect(m_resetPwdBtn, &QPushButton::clicked, this, &UserManager::onResetPassword);
    toolbarLayout->addWidget(m_resetPwdBtn);

    toolbarLayout->addStretch();
    mainLayout->addLayout(toolbarLayout);

    // 表格
    m_table = new QTableWidget(this);
    m_table->setColumnCount(5);
    m_table->setHorizontalHeaderLabels(QStringList()
        << QString::fromUtf8("ID")
        << QString::fromUtf8("用户名")
        << QString::fromUtf8("真实姓名")
        << QString::fromUtf8("角色")
        << QString::fromUtf8("创建时间"));
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

void UserManager::loadUsers()
{
    HttpClient client;
    client.setAuthToken(QString::number(m_userId));

    QJsonObject response = client.syncGet(Constants::API_BASE + "/users");

    if (!response["success"].toBool()) {
        QMessageBox::warning(this, QString::fromUtf8("错误"),
                             response["message"].toString());
        return;
    }

    QJsonObject data = response["data"].toObject();
    QJsonArray usersArray = data["users"].toArray();

    m_users.clear();
    m_table->setRowCount(0);

    for (const QJsonValue &val : usersArray) {
        User u = User::fromJson(val.toObject());
        m_users.append(u);
    }

    m_table->setRowCount(m_users.size());
    for (int i = 0; i < m_users.size(); ++i) {
        const User &u = m_users[i];
        m_table->setItem(i, 0, new QTableWidgetItem(QString::number(u.id())));
        m_table->setItem(i, 1, new QTableWidgetItem(u.username()));
        m_table->setItem(i, 2, new QTableWidgetItem(u.realName()));
        m_table->setItem(i, 3, new QTableWidgetItem(u.roleString()));
        m_table->setItem(i, 4, new QTableWidgetItem(QString::fromUtf8("-")));
    }
}

void UserManager::showUserDialog(User *user)
{
    bool isEdit = (user != nullptr);
    QDialog dialog(this);
    dialog.setWindowTitle(isEdit ? QString::fromUtf8("编辑用户") : QString::fromUtf8("新增用户"));
    dialog.resize(400, 300);

    QVBoxLayout *layout = new QVBoxLayout(&dialog);
    layout->setSpacing(15);
    layout->setContentsMargins(20, 20, 20, 20);

    QGridLayout *formLayout = new QGridLayout();
    formLayout->setSpacing(10);

    // 用户名
    formLayout->addWidget(new QLabel(QString::fromUtf8("用户名："), &dialog), 0, 0);
    QLineEdit *usernameEdit = new QLineEdit(&dialog);
    formLayout->addWidget(usernameEdit, 0, 1);

    // 真实姓名
    formLayout->addWidget(new QLabel(QString::fromUtf8("真实姓名："), &dialog), 1, 0);
    QLineEdit *realNameEdit = new QLineEdit(&dialog);
    formLayout->addWidget(realNameEdit, 1, 1);

    // 密码
    formLayout->addWidget(new QLabel(QString::fromUtf8("密码："), &dialog), 2, 0);
    QLineEdit *passwordEdit = new QLineEdit(&dialog);
    passwordEdit->setEchoMode(QLineEdit::Password);
    passwordEdit->setPlaceholderText(isEdit ? QString::fromUtf8("留空则不修改") : QString::fromUtf8("默认123456"));
    formLayout->addWidget(passwordEdit, 2, 1);

    // 角色
    formLayout->addWidget(new QLabel(QString::fromUtf8("角色："), &dialog), 3, 0);
    QComboBox *roleCombo = new QComboBox(&dialog);
    roleCombo->addItem(QString::fromUtf8("考生"), static_cast<int>(Constants::RoleStudent));
    roleCombo->addItem(QString::fromUtf8("教师"), static_cast<int>(Constants::RoleTeacher));
    roleCombo->addItem(QString::fromUtf8("管理员"), static_cast<int>(Constants::RoleAdmin));
    formLayout->addWidget(roleCombo, 3, 1);

    layout->addLayout(formLayout);
    layout->addStretch();

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

    // 编辑模式填充数据
    if (isEdit) {
        usernameEdit->setText(user->username());
        realNameEdit->setText(user->realName());
        roleCombo->setCurrentIndex(static_cast<int>(user->role()));
    }

    if (dialog.exec() != QDialog::Accepted) {
        return;
    }

    // 保存
    QJsonObject userData;
    if (isEdit) {
        userData["id"] = user->id();
    }
    userData["username"] = usernameEdit->text().trimmed();
    userData["real_name"] = realNameEdit->text().trimmed();
    userData["role"] = roleCombo->currentIndex();
    if (!passwordEdit->text().isEmpty()) {
        userData["password"] = passwordEdit->text();
    }

    HttpClient client;
    client.setAuthToken(QString::number(m_userId));

    QJsonObject response;
    if (isEdit) {
        response = client.syncPut(Constants::API_BASE + "/users/" + QString::number(user->id()), userData);
    } else {
        response = client.syncPost(Constants::API_BASE + "/users", userData);
    }

    if (response["success"].toBool()) {
        QMessageBox::information(this, QString::fromUtf8("成功"),
                                 isEdit ? QString::fromUtf8("用户更新成功") : QString::fromUtf8("用户添加成功"));
        loadUsers();
    } else {
        QMessageBox::warning(this, QString::fromUtf8("失败"),
                             response["message"].toString());
    }
}

void UserManager::onRefresh()
{
    loadUsers();
}

void UserManager::onAdd()
{
    showUserDialog();
}

void UserManager::onEdit()
{
    int row = m_table->currentRow();
    if (row < 0) {
        QMessageBox::warning(this, QString::fromUtf8("提示"),
                             QString::fromUtf8("请先选择一个用户"));
        return;
    }
    showUserDialog(&m_users[row]);
}

void UserManager::onDelete()
{
    int row = m_table->currentRow();
    if (row < 0) {
        QMessageBox::warning(this, QString::fromUtf8("提示"),
                             QString::fromUtf8("请先选择一个用户"));
        return;
    }

    int id = m_users[row].id();
    QString name = m_users[row].realName();

    int ret = QMessageBox::question(this, QString::fromUtf8("确认删除"),
                                   QString::fromUtf8("确定要删除用户「%1」吗？").arg(name),
                                   QMessageBox::Yes | QMessageBox::No,
                                   QMessageBox::No);
    if (ret != QMessageBox::Yes) {
        return;
    }

    HttpClient client;
    client.setAuthToken(QString::number(m_userId));

    QJsonObject response = client.syncDelete(Constants::API_BASE + "/users/" + QString::number(id));

    if (response["success"].toBool()) {
        QMessageBox::information(this, QString::fromUtf8("成功"),
                                 QString::fromUtf8("用户删除成功"));
        loadUsers();
    } else {
        QMessageBox::warning(this, QString::fromUtf8("失败"),
                             response["message"].toString());
    }
}

void UserManager::onResetPassword()
{
    int row = m_table->currentRow();
    if (row < 0) {
        QMessageBox::warning(this, QString::fromUtf8("提示"),
                             QString::fromUtf8("请先选择一个用户"));
        return;
    }

    int id = m_users[row].id();
    QString name = m_users[row].realName();

    int ret = QMessageBox::question(this, QString::fromUtf8("确认重置"),
                                   QString::fromUtf8("确定要重置用户「%1」的密码为123456吗？").arg(name),
                                   QMessageBox::Yes | QMessageBox::No,
                                   QMessageBox::No);
    if (ret != QMessageBox::Yes) {
        return;
    }

    QJsonObject userData;
    userData["password"] = "123456";

    HttpClient client;
    client.setAuthToken(QString::number(m_userId));

    QJsonObject response = client.syncPut(Constants::API_BASE + "/users/" + QString::number(id), userData);

    if (response["success"].toBool()) {
        QMessageBox::information(this, QString::fromUtf8("成功"),
                                 QString::fromUtf8("密码重置成功，新密码为：123456"));
    } else {
        QMessageBox::warning(this, QString::fromUtf8("失败"),
                             response["message"].toString());
    }
}
