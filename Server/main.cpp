#include "mainwindow.h"
#include <QApplication>
#include <QCommandLineParser>
#include <utility_time.hpp>
#include <csignal>
#include <fstream>
#include <string>
#include "ThreadOllama.hpp"
#include <csignal>

void handle_sigint(int) {
    std::time_t currentTime = std::time(0); 
    // Convert the time_t object to a string representing local time
    char* dateTimeString = std::ctime(&currentTime);

    // Print the current date and time
    std::cout << "The current date and time is: " << dateTimeString << std::endl;
    
    string filename = "Conversation_Summarys/Conversation_Summary-";
    filename += dateTimeString;
    filename += ".txt";
    std::ofstream file(filename);

    //std::exit(0);  // Exit cleanly
    std::_Exit(0);
}

int main(int argc, char *argv[])
{
    signal(SIGINT, handle_sigint);
    //signal(SIGSEGV, handle_sigint);
    QApplication app(argc, argv);
    QCoreApplication::setApplicationName("Zenbo Nurse Helper");
    QCoreApplication::setApplicationVersion("25.5.25");
    app.setWindowIcon(QIcon(":/ZenboNurse.png"));

    QCommandLineParser parser;
    parser.setApplicationDescription("Zenbo Nurse Helper");
    parser.addHelpOption();
    parser.addVersionOption();
    QString home_directory = QStandardPaths::writableLocation(QStandardPaths::HomeLocation);

    QCommandLineOption ImageSaveDirectoryOption("ImageSaveDirectory", "image save directory.", "directory", home_directory + "/Downloads");
    parser.addOption(ImageSaveDirectoryOption);

    QCommandLineOption whisperModelOption({"wm","WhisperModel"}, "whisper model to be loaded.", "file path", home_directory + "/ZenboNurseHelper_build/whisper.cpp/models/ggml-base.en.bin");
    parser.addOption(whisperModelOption);

    QCommandLineOption languageModelOption({"lm", "LanguageModel"}, "language model", "string", "gemma3:1b");
    parser.addOption(languageModelOption);

    QCommandLineOption imageSaveEveryNFrameOption({"is","ImageSaveEveryNFrame"}, "1 of <N> frames will be saved", "natural number", "1");
    parser.addOption(imageSaveEveryNFrameOption);

    QCommandLineOption languageOption("Language", "Language used", "string", "Chinese");
    parser.addOption(languageOption);

    QCommandLineOption DefaultSaveImageOption("DefaultSaveImage", "The default value of saving images.", "boolean", "false");
    parser.addOption(DefaultSaveImageOption);

    QCommandLineOption previousContextOption({"pf", "previous_context"}, "Previous context text file", "string", "");
    parser.addOption(previousContextOption);

    QCommandLineOption stageOption({"s", "stage"}, "LLM starting stage", "int", 0);
    parser.addOption(stageOption);

    parser.process(app);

    if (parser.isSet(ImageSaveDirectoryOption)) {
        QString ImageSaveDirectory = parser.value(ImageSaveDirectoryOption);
        qDebug() << "ImageSaveDirectory is:" << ImageSaveDirectory;
    }

    QString whisperModel;
    if (parser.isSet(whisperModelOption)) {
        whisperModel = parser.value(whisperModelOption);
        qDebug() << "whisperModel file is:" << whisperModel;
    }

    QString languageModel;
    if (parser.isSet(languageModelOption)) {
        languageModel = parser.value(languageModelOption);
        qDebug() << "languageModel string is:" << languageModel;
    }

    QString strimageSaveEveryNFrame;
    if (parser.isSet(imageSaveEveryNFrameOption)) {
        strimageSaveEveryNFrame = parser.value(imageSaveEveryNFrameOption);
        qDebug() << "imageSaveEveryNFrame value is:" << strimageSaveEveryNFrame;
    }

    QString strLanguage;
    if (parser.isSet(languageOption)) {
        strLanguage = parser.value(languageOption);
        qDebug() << "Language string is:" << strLanguage;
    }

    QString strDefaultSaveImage;
    bool bDefaultSaveImage = false;
    if (parser.isSet(DefaultSaveImageOption)) {
        bDefaultSaveImage = parser.value(DefaultSaveImageOption).toLower() == "true";
        qDebug() << "DefaultSaveImage boolean is:" << bDefaultSaveImage;
    }

    //Program launch time for Fang-yu's need to save images in this directory
    string str_now = GetCurrentTimeString(false);


    QString previousContext;
    if (parser.isSet(previousContextOption)) {
        previousContext = parser.value(previousContextOption);
        qDebug() << "previousContext string is:" << previousContext;
    }

    QString strstage;;
    if (parser.isSet(stageOption)) {
        strstage = parser.value(stageOption);
        qDebug() << "Stage int is:" << strstage;
    }

    MainWindow w;
    w.setWhisperModelFile(whisperModel);
    w.setLanguageModelName(languageModel);
    w.setStage(strstage.toInt());
    w.setImageSaveEveryNFrame(strimageSaveEveryNFrame.toInt());
    w.setLanguage(strLanguage);
    w.setImageSaveDirectory(parser.value(ImageSaveDirectoryOption).append("/").append(str_now.c_str()));
    w.setDefaultSaveImage(bDefaultSaveImage);
    w.startThreads();
    w.show();
    app.exec();
    return 1;
}
