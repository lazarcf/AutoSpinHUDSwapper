#include "mainwindow.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    MainWindow w;
    w.show();

    a.setQuitOnLastWindowClosed(false);

    qRegisterMetaType<HWND>("HWND");
    return a.exec();
}
