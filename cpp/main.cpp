#include "mainwindow.h"
#include <QApplication>
#include <QCommandLineParser>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    QCoreApplication::setApplicationName("Zenbo Nurse Helper");
    QCoreApplication::setApplicationVersion("24.8.6");

    QCommandLineParser parser;
    parser.setApplicationDescription("Zenbo Nurse Helper");
    parser.addHelpOption();
    parser.addVersionOption();
    parser.addPositionalArgument("source", QCoreApplication::translate("main", "Source file to copy."));
    parser.addPositionalArgument("destination", QCoreApplication::translate("main", "Destination directory."));
    

    MainWindow w;
    w.show();
    a.exec();
    return 1;
}
