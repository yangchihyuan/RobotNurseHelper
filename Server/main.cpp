#include "mainwindow.h"
#include <QApplication>
#include <QCommandLineParser>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    QCoreApplication::setApplicationName("Zenbo Nurse Helper");
    QCoreApplication::setApplicationVersion("25.4.18");
    app.setWindowIcon(QIcon(":/ZenboNurse.png"));

    QCommandLineParser parser;
    parser.setApplicationDescription("Zenbo Nurse Helper");
    parser.addHelpOption();
    parser.addVersionOption();
    QCommandLineOption outputDirectoryOption("d", "output-directory", "Write output to <directory>", "directory");
    parser.addOption(outputDirectoryOption);

    QCommandLineOption languageModelOption("m", "language-model", "language model <file> to be loaded for whipser.cpp", "file");
    parser.addOption(languageModelOption);

    parser.process(app);

    if (parser.isSet(outputDirectoryOption)) {
        QString outputDirectory = parser.value(outputDirectoryOption);
        qDebug() << "outputDirectory is:" << outputDirectory;
    }

    QString languageModel;
    if (parser.isSet(languageModelOption)) {
        languageModel = parser.value(languageModelOption);
        qDebug() << "languageModel file is:" << languageModel;
    }

    MainWindow w;
    w.setWhisperModelFile(languageModel);
    w.startThreads();
    w.show();
    app.exec();
    return 1;
}
