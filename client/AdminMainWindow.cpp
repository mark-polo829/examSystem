#include "AdminMainWindow.h"
#include "QuestionManager.h"
#include "ScoreStatistics.h"
#include "UserManager.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QSplitter>
#include <QFrame>
#include <QMessageBox>
#include <QScrollArea>
#include <QApplication>
#include <QScreen>

AdminMainWindow::AdminMainWindow(const User &user, QWidget *parent)
    : QMainWindow(parent), m_user(user)
{
    setWindowTitle(QString::fromUtf8("在线考试管理系统 - %1").arg(user.realName()));

    // 根据屏幕尺寸设置默认窗口大小，避免在小屏上超出可视区域
    QRect screen = QApplication::primaryScreen()->availableGeometry();
    QSize size = screen.size() * 4 / 5;
    size = size.boundedTo(QSize(1200, 800));
    size = size.expandedTo(QSize(900, 600));
    resize(size);
    setMinimumSize(800, 600);

    setupUI();
}

void AdminMainWindow::setupUI()
{
    // 中央部件
    m_centralWidget = new QWidget(this);
    setCentralWidget(m_centralWidget);

    QVBoxLayout *mainLayout = new QVBoxLayout(m_centralWidget);
    mainLayout->setSpacing(0);
    mainLayout->setContentsMargins(0, 0, 0, 0);

    // ===== 顶部状态栏 =====
    QFrame *topBar = new QFrame(this);
    topBar->setStyleSheet(
        "QFrame {"
        "  background-color: #1976D2;"
        "  color: white;"
        "}"
    );
    QHBoxLayout *topLayout = new QHBoxLayout(topBar);
    topLayout->setContentsMargins(15, 10, 15, 10);

    QLabel *titleLabel = new QLabel(QString::fromUtf8("📚 在线考试管理系统"), this);
    QFont titleFont = titleLabel->font();
    titleFont.setPointSize(14);
    titleFont.setBold(true);
    titleLabel->setFont(titleFont);
    titleLabel->setStyleSheet("QLabel { color: white; }");
    topLayout->addWidget(titleLabel);

    topLayout->addStretch();

    m_userInfoLabel = new QLabel(this);
    m_userInfoLabel->setStyleSheet("QLabel { color: white; font-size: 12px; }");
    m_userInfoLabel->setText(QString::fromUtf8("当前用户：%1 | 角色：%2")
                               .arg(m_user.realName())
                               .arg(m_user.roleString()));
    topLayout->addWidget(m_userInfoLabel);

    QPushButton *logoutBtn = new QPushButton(QString::fromUtf8("退出登录"), this);
    logoutBtn->setStyleSheet(
        "QPushButton {"
        "  background-color: transparent;"
        "  color: white;"
        "  border: 1px solid white;"
        "  padding: 5px 15px;"
        "  border-radius: 3px;"
        "}"
        "QPushButton:hover {"
        "  background-color: rgba(255,255,255,0.2);"
        "}"
    );
    connect(logoutBtn, &QPushButton::clicked, this, &AdminMainWindow::onLogout);
    topLayout->addWidget(logoutBtn);

    mainLayout->addWidget(topBar);

    // ===== 主体区域 =====
    QSplitter *splitter = new QSplitter(Qt::Horizontal, this);

    // 左侧菜单
    QWidget *leftPanel = new QWidget(this);
    QVBoxLayout *leftLayout = new QVBoxLayout(leftPanel);
    leftLayout->setContentsMargins(0, 0, 0, 0);
    leftLayout->setSpacing(0);

    m_menuList = new QListWidget(this);
    m_menuList->setMaximumWidth(220);
    m_menuList->setMinimumWidth(180);
    m_menuList->setStyleSheet(
        "QListWidget {"
        "  border: none;"
        "  background-color: #263238;"
        "  outline: none;"
        "}"
        "QListWidget::item {"
        "  color: #b0bec5;"
        "  padding: 15px 20px;"
        "  border-left: 4px solid transparent;"
        "  font-size: 14px;"
        "}"
        "QListWidget::item:selected {"
        "  background-color: #37474f;"
        "  color: white;"
        "  border-left: 4px solid #2196F3;"
        "}"
        "QListWidget::item:hover {"
        "  background-color: #37474f;"
        "}"
    );
    setupMenu();
    leftLayout->addWidget(m_menuList);

    // 右侧内容区
    m_contentStack = new QStackedWidget(this);

    m_questionManager = new QuestionManager(m_user.id(), this);
    m_scoreStatistics = new ScoreStatistics(m_user.id(), this);
    m_userManager = new UserManager(m_user.id(), this);

    m_contentStack->addWidget(m_questionManager);    // 0
    m_contentStack->addWidget(m_scoreStatistics);   // 1
    m_contentStack->addWidget(m_userManager);      // 2

    // 用滚动区域包裹内容区，避免窗口较小时内部组件被截断
    QScrollArea *contentScroll = new QScrollArea(this);
    contentScroll->setWidgetResizable(true);
    contentScroll->setFrameShape(QFrame::NoFrame);
    contentScroll->setWidget(m_contentStack);

    splitter->addWidget(leftPanel);
    splitter->addWidget(contentScroll);
    splitter->setStretchFactor(0, 0);
    splitter->setStretchFactor(1, 1);
    splitter->setSizes(QList<int>() << 220 << 980);

    mainLayout->addWidget(splitter, 1);

    // ===== 底部状态栏 =====
    QFrame *bottomBar = new QFrame(this);
    bottomBar->setStyleSheet(
        "QFrame {"
        "  background-color: #f5f5f5;"
        "  border-top: 1px solid #ddd;"
        "}"
    );
    QHBoxLayout *bottomLayout = new QHBoxLayout(bottomBar);
    bottomLayout->setContentsMargins(15, 5, 15, 5);

    m_statusLabel = new QLabel(QString::fromUtf8("就绪"), this);
    m_statusLabel->setStyleSheet("QLabel { color: #666; font-size: 12px; }");
    bottomLayout->addWidget(m_statusLabel);

    bottomLayout->addStretch();

    QLabel *versionLabel = new QLabel("v1.0.0", this);
    versionLabel->setStyleSheet("QLabel { color: #999; font-size: 11px; }");
    bottomLayout->addWidget(versionLabel);

    mainLayout->addWidget(bottomBar);

    // 连接信号
    connect(m_menuList, &QListWidget::currentRowChanged,
            this, &AdminMainWindow::onMenuSelected);

    // 默认选中第一项
    m_menuList->setCurrentRow(0);
}

void AdminMainWindow::setupMenu()
{
    // 题库管理（教师和管理员可见）
    QListWidgetItem *item1 = new QListWidgetItem(QString::fromUtf8("📖 题库管理"), m_menuList);
    item1->setData(Qt::UserRole, 0);
    m_menuList->addItem(item1);

    // 成绩统计（教师和管理员可见）
    QListWidgetItem *item2 = new QListWidgetItem(QString::fromUtf8("📊 成绩统计"), m_menuList);
    item2->setData(Qt::UserRole, 1);
    m_menuList->addItem(item2);

    // 用户管理（仅管理员可见）
    if (m_user.role() == Constants::RoleAdmin) {
        QListWidgetItem *item3 = new QListWidgetItem(QString::fromUtf8("👥 用户管理"), m_menuList);
        item3->setData(Qt::UserRole, 2);
        m_menuList->addItem(item3);
    }
}

void AdminMainWindow::onMenuSelected(int index)
{
    if (index < 0) return;

    QListWidgetItem *item = m_menuList->item(index);
    int pageIndex = item->data(Qt::UserRole).toInt();

    m_contentStack->setCurrentIndex(pageIndex);
    m_statusLabel->setText(QString::fromUtf8("当前页面：%1").arg(item->text()));
}

void AdminMainWindow::onLogout()
{
    int ret = QMessageBox::question(this, QString::fromUtf8("确认退出"),
                                     QString::fromUtf8("确定要退出登录吗？"),
                                     QMessageBox::Yes | QMessageBox::No,
                                     QMessageBox::No);
    if (ret == QMessageBox::Yes) {
        emit logoutRequested();
        close();
    }
}
