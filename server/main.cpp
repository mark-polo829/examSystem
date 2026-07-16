#include "ExamServer.h"
#include "Constants.h"
#include <QCoreApplication>
#include <QDebug>
#include <iostream>

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);

    ExamServer server;
    if (!server.start(Constants::SERVER_PORT)) {
        qCritical() << "Failed to start server!";
        return 1;
    }

    std::cout << "Exam server is running on port " << Constants::SERVER_PORT << std::endl;
    std::cout << "Press Ctrl+C to stop." << std::endl;

    return app.exec();
}
