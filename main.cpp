#include <QApplication>
#include "mainwindow.h"
#include "grindmanager.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    GrindManager manager;
    MainWindow window(&manager);
    window.setWindowTitle("The Grind");
    window.resize(600, 500);
    window.show();

    return app.exec();
}