#include "mainwindow.h"
#include <QApplication>
#include <QCommandLineParser>
#include <utility_time.hpp>
#include <csignal>
#include <fstream>
#include <string>
#include "ThreadLLM.hpp"
#include <csignal>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    QCoreApplication::setApplicationName("Robot Nurse Helper");
    QCoreApplication::setApplicationVersion("2026.03.31");
    //It does not work. My application does not have a icon.
    app.setWindowIcon(QIcon(":/ZenboNurse.png"));

    QCommandLineParser parser;
    parser.setApplicationDescription("Robot Nurse Helper");
    parser.addHelpOption();
    parser.addVersionOption();
    QString home_directory = QStandardPaths::writableLocation(QStandardPaths::HomeLocation);

    QCommandLineOption SettingFileOption("SettingFile", "Setting File", "string", "Setting.json");
    parser.addOption(SettingFileOption);

    parser.process(app);

    QString strSetting;
    if (parser.isSet(SettingFileOption)) {
        strSetting = parser.value(SettingFileOption);
        qDebug() << "Setting file is:" << strSetting;
    }

    MainWindow w;
    w.setSettingFile(strSetting);
    w.startThreads();

    Setting msetting;
    LoadJSONFile(msetting, strSetting.toStdString());

    // Hide the mouse cursor globally for the application
    //debug
    cout << "bHideCursor: " << msetting.bHideCursor << endl;
    if (msetting.bHideCursor) {
        app.setOverrideCursor(Qt::BlankCursor);
    }
    
    w.show();
    app.exec();
    return 1;
}