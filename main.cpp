#include "mainwindow.h"
#include <QApplication>
#include <QMessageBox>
#include <QDebug>
#include <exception>
#include <windows.h>
#include <dbghelp.h>
#include <QLoggingCategory>

Q_LOGGING_CATEGORY(appLog, "app")



LONG WINAPI MyUnhandledExceptionFilter(EXCEPTION_POINTERS* ExceptionInfo)
{
    qCritical() << "Unhandled exception occurred!";
    return EXCEPTION_EXECUTE_HANDLER;
}

int main(int argc, char *argv[])
{


    SetUnhandledExceptionFilter(MyUnhandledExceptionFilter);

    QApplication a(argc, argv);
    a.setWindowIcon(QIcon("icon.png"));

    try {
        qDebug() << "Application starting...";
        MainWindow w;
        qDebug() << "MainWindow created";
        w.show();
        qDebug() << "MainWindow shown";
        return a.exec();
    }
    catch (const std::exception& e) {
        qCritical() << "Exception caught:" << e.what();
        QMessageBox::critical(nullptr, "程序崩溃", QString("程序发生异常: %1").arg(e.what()));
        return -1;
    }
    catch (...) {
        qCritical() << "Unknown exception caught";
        QMessageBox::critical(nullptr, "程序崩溃", "程序发生未知异常");
        return -1;
    }
    MainWindow window;
    window.show();

    return a.exec();
}
