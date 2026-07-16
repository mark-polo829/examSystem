#include <QApplication>
#include <QEventLoop>
#include "LoginDialog.h"
#include "ExamWindow.h"
#include "AdminMainWindow.h"
#include "StudentMainWindow.h"
#include "HttpClient.h"
#include <QDebug>

int main(int argc, char *argv[])
{
    QApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
    QApplication app(argc, argv);
    // 关闭最后一个窗口时不自动退出，便于退出登录后重新显示登录对话框
    app.setQuitOnLastWindowClosed(false);

    // 设置应用样式
    app.setStyle("Fusion");
    QPalette palette;
    palette.setColor(QPalette::Window, QColor(245, 245, 245));
    palette.setColor(QPalette::WindowText, Qt::black);
    palette.setColor(QPalette::Base, Qt::white);
    palette.setColor(QPalette::AlternateBase, QColor(240, 240, 240));
    palette.setColor(QPalette::ToolTipBase, Qt::white);
    palette.setColor(QPalette::ToolTipText, Qt::black);
    palette.setColor(QPalette::Text, Qt::black);
    palette.setColor(QPalette::Button, QColor(240, 240, 240));
    palette.setColor(QPalette::ButtonText, Qt::black);
    palette.setColor(QPalette::BrightText, Qt::red);
    palette.setColor(QPalette::Highlight, QColor(33, 150, 243));
    palette.setColor(QPalette::HighlightedText, Qt::white);
    app.setPalette(palette);

    while (true) {
        // 登录对话框
        LoginDialog loginDialog;
        if (loginDialog.exec() != QDialog::Accepted) {
            break;
        }

        User user = loginDialog.loggedInUser();

        bool logoutRequested = false;
        QMainWindow *mainWindow = nullptr;

        // 根据角色打开不同界面
        if (user.role() == Constants::RoleStudent) {
            StudentMainWindow *studentWindow = new StudentMainWindow(user);
            studentWindow->setAttribute(Qt::WA_DeleteOnClose);

            bool exitApp = false;

            while (!exitApp && !logoutRequested) {
                QEventLoop loop;
                bool startExam = false;
                bool windowDestroyed = false;

                QMetaObject::Connection connStart = QObject::connect(
                    studentWindow, &StudentMainWindow::startExamRequested, &loop, [&]() {
                        startExam = true;
                        loop.quit();
                    });
                QMetaObject::Connection connLogout = QObject::connect(
                    studentWindow, &StudentMainWindow::logoutRequested, &loop, [&]() {
                        logoutRequested = true;
                        loop.quit();
                    });
                QMetaObject::Connection connDestroyed = QObject::connect(
                    studentWindow, &QMainWindow::destroyed, &loop, [&]() {
                        windowDestroyed = true;
                        loop.quit();
                    });

                studentWindow->show();
                loop.exec();

                QObject::disconnect(connStart);
                QObject::disconnect(connLogout);
                QObject::disconnect(connDestroyed);

                // 窗口被关闭（点 X），退出程序
                if (windowDestroyed) {
                    exitApp = true;
                    break;
                }

                // 退出登录，返回登录界面
                if (logoutRequested) {
                    studentWindow->close();
                    break;
                }

                if (!startExam) {
                    // 其他情况，安全退出循环
                    exitApp = true;
                    break;
                }

                // 开始考试
                studentWindow->hide();

                ExamWindow *examWindow = new ExamWindow(user);
                examWindow->setAttribute(Qt::WA_DeleteOnClose);

                QEventLoop examLoop;
                bool examFinishedSignal = false;
                bool examLogout = false;
                bool examWindowDestroyed = false;

                QMetaObject::Connection connExamFinished = QObject::connect(
                    examWindow, &ExamWindow::examFinished, &examLoop, [&]() {
                        examFinishedSignal = true;
                        examLoop.quit();
                    });
                QMetaObject::Connection connExamLogout = QObject::connect(
                    examWindow, &ExamWindow::logoutRequested, &examLoop, [&]() {
                        examLogout = true;
                        examLoop.quit();
                    });
                QMetaObject::Connection connExamDestroyed = QObject::connect(
                    examWindow, &QMainWindow::destroyed, &examLoop, [&]() {
                        examWindowDestroyed = true;
                        examLoop.quit();
                    });

                examWindow->show();
                examWindow->startExam();
                examLoop.exec();

                QObject::disconnect(connExamFinished);
                QObject::disconnect(connExamLogout);
                QObject::disconnect(connExamDestroyed);

                examWindow->close();

                if (examLogout) {
                    logoutRequested = true;
                    studentWindow->close();
                    break;
                }

                // 考试结束或窗口被关闭，刷新记录并回到考生主界面
                studentWindow->refreshRecords();
            }

            if (exitApp) {
                break;
            }
        } else {
            AdminMainWindow *adminWindow = new AdminMainWindow(user);
            adminWindow->setAttribute(Qt::WA_DeleteOnClose);
            QObject::connect(adminWindow, &AdminMainWindow::logoutRequested, [&]() {
                logoutRequested = true;
            });
            adminWindow->show();
            mainWindow = adminWindow;

            // 等待主窗口关闭（正常关闭则退出程序，退出登录则回到登录界面）
            QEventLoop loop;
            QObject::connect(mainWindow, &QMainWindow::destroyed, &loop, &QEventLoop::quit);
            loop.exec();

            if (!logoutRequested) {
                break;
            }
        }
    }

    return 0;
}
