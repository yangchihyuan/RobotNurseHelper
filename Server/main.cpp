#include "mainwindow.h"
#include <QApplication>
#include <QCommandLineParser>
#include <csignal>
#include <fstream>
#include <string>
#include "ThreadOllama.hpp"
#include <csignal>
//extern std::string chosen_action;
//extern string chosen_action;
void handle_sigint(int) {
    std::ofstream file("chosen_action.txt");
    if (file.is_open()) {
        file << chosen_action << "\n\n";
        for (int i = 0; i < summary.size(); i++)
        {
            file << summary[i] << "\n\n";
        }
        file.close();
        cout << "\n" << chosen_action << "\nSaved chosen_action on Ctrl+C\n";
    }
    //std::exit(0);  // Exit cleanly
    std::_Exit(0);
}
// Register at exit
//__attribute__((constructor)) void register_saver() {
//    std::atexit(save_chosen_action);
//}

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


    MainWindow w;
    w.setWhisperModelFile(whisperModel);
    w.setLanguageModelName(languageModel);
    w.setImageSaveEveryNFrame(strimageSaveEveryNFrame.toInt());
    w.setLanguage(strLanguage);
    w.setImageSaveDirectory(parser.value(ImageSaveDirectoryOption));
    w.setDefaultSaveImage(bDefaultSaveImage);
    w.startThreads();
    w.show();
    app.exec();
    return 1;
}
