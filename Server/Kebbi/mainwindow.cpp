#include "mainwindow.h"
#include "./ui_mainwindow.h"
#include <QPixmap>
#include <QStringListModel>
#include <QStandardItemModel>
#include <QModelIndex>
#include <QAbstractItemView>
#include <iostream>
#ifdef USE_KEBBI
    #include "Kebbi/RobotCommand.pb.h"
#elif USE_ZENBO
    #include "Zenbo/RobotCommand.pb.h"
#endif
#include <QTimer>
#include <opencv2/core.hpp>
#include <opencv2/highgui.hpp>
#include <QScrollBar>
#include "RobotStatus.hpp"
#include "ActionOption.hpp"
#include <memory>
#include <QCloseEvent>

extern std::mutex gMutex_audio_buffer;
extern std::queue<short> AudioBuffer;
extern std::condition_variable cond_var_audio;
extern int PortAudio_stop_and_terminate();
extern bool gbPlayAudio;    //I don't use this variable yet.
extern RobotStatus robot_status;
extern ActionOption action_option;

time_t start_dance_time = 0;

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    LoadJSONFile(msetting, "json/Setting.json");    

    thread_whisper.model_file_path = QString::fromStdString(ReplaceShellVariable(msetting.WhisperModel));    
    setLanguage(QString::fromStdString(msetting.Language));
    thread_process_image.ImageSaveDirectory = ReplaceShellVariable(msetting.ImageSaveDirectory);
    thread_process_image.bSaveTransmittedImage = msetting.bSaveImages;


    QStringList strList;
    strList.append("TTS_AngerA");
    strList.append("TTS_AngerB");
    strList.append("TTS_Contempt");
    strList.append("TTS_Disgust");
    strList.append("TTS_Fear");
    strList.append("TTS_JoyA"); //5
    strList.append("TTS_JoyB");
    strList.append("TTS_JoyC");
    strList.append("TTS_PeaceA");
    strList.append("TTS_PeaceB");
    strList.append("TTS_PeaceC");
    strList.append("TTS_SadnessA"); //11
    strList.append("TTS_SadnessB");
    strList.append("TTS_Surprise");

    QStandardItemModel* ItemModel = new QStandardItemModel(this);
    int nCount = strList.size();
    for(int i = 0; i < nCount; i++)
    {
        QString string = static_cast<QString>(strList.at(i));
        QStandardItem *item = new QStandardItem(string);
        ItemModel->appendRow(item);
    }
    ui->listView_FacialExpressions->setModel(ItemModel);
    ui->listView_FacialExpressions->setEditTriggers(QAbstractItemView::NoEditTriggers);

    QStringList strList_action;
    strList_action.append("TA_DictateL");
    strList_action.append("DA_Full");
    strList_action.append("EM_Mad02");
    strList_action.append("BA_Nodhead");
    strList_action.append("SP_Swim02"); 
    strList_action.append("PE_RotateA"); //5
    strList_action.append("SP_Karate");
    strList_action.append("RE_Cheer");
    strList_action.append("SP_Climb");
    strList_action.append("DA_Hit"); 
    strList_action.append("TA_DictateR"); //10
    strList_action.append("SP_Bowling");
    strList_action.append("SP_Walk");
    strList_action.append("SA_Find");
    strList_action.append("BA_TurnHead");
    strList_action.append("SA_Toothache"); //15
    strList_action.append("SA_Sick");
    strList_action.append("SA_Shocked");
    strList_action.append("SP_Dumbbell");
    strList_action.append("SA_Discover");
    strList_action.append("RE_Thanks"); //15
    strList_action.append("PE_Changing");
    strList_action.append("SP_HorizontalBar");
    strList_action.append("WO_Traffic");
    strList_action.append("RE_HiR");
    strList_action.append("RE_HiL"); //20
    strList_action.append("DA_Brushteeth");
    strList_action.append("RE_Encourage");
    strList_action.append("RE_Request");
    strList_action.append("PE_Brewing");
    strList_action.append("RE_Change"); //25
    strList_action.append("PE_Phubbing");
    strList_action.append("RE_Baoquan");
    strList_action.append("SP_Cheer");
    strList_action.append("RE_Ask");
    strList_action.append("PE_Triangel"); //30
    strList_action.append("PE_Sorcery");
    strList_action.append("PE_Sneak");
    strList_action.append("PE_Singing");
    strList_action.append("LE_Yoyo");
    strList_action.append("SP_Throw"); //35
    strList_action.append("SP_RaceWalk");
    strList_action.append("PE_ShakeFart");
    strList_action.append("PE_RotateC");
    strList_action.append("PE_RotateB");
    strList_action.append("EM_Blush"); //40
    strList_action.append("PE_Puff");
    strList_action.append("PE_PlayCello");
    strList_action.append("PE_Pikachu");

    QStandardItemModel* ItemModel_action = new QStandardItemModel(this);
    nCount = strList_action.size();
    for(int i = 0; i < nCount; i++)
    {
        QString string = static_cast<QString>(strList_action.at(i));
        QStandardItem *item = new QStandardItem(string);
        ItemModel_action->appendRow(item);
    }
    ui->listView_PredefinedAction->setModel(ItemModel_action);
    ui->listView_PredefinedAction->setEditTriggers(QAbstractItemView::NoEditTriggers);

    QStringList strList_content;
    strList_content.append("Cataract_health_education_1_20250725095927");

    QStandardItemModel* ItemModel_content = new QStandardItemModel(this);
    nCount = strList_content.size();
    for(int i = 0; i < nCount; i++)
    {
        QString string = static_cast<QString>(strList_content.at(i));
        QStandardItem *item = new QStandardItem(string);
        ItemModel_content->appendRow(item);
    }
    ui->listView_Content->setModel(ItemModel_content);
    ui->listView_Content->setEditTriggers(QAbstractItemView::NoEditTriggers);


    //One QTcpServer only listens to one port. If you want to listen to multiple ports, you need to create multiple QTcpServer objects.
    m_server_receive_image = new QTcpServer();
    //2024/12/27 The port number is also hard-coded. I need to modify it in the future.
    if(m_server_receive_image->listen(QHostAddress::Any, 8895))
    {
       connect(m_server_receive_image, &QTcpServer::newConnection, this, &MainWindow::newConnection_receive_image);
    }
    else
    {
        exit(EXIT_FAILURE);
    }

    m_server_send_command = new QTcpServer();
    if(m_server_send_command->listen(QHostAddress::Any, 8896))
    {
        connect(m_server_send_command, &QTcpServer::newConnection, this, &MainWindow::newConnection_send_command);
    }
    else
    {
        exit(EXIT_FAILURE);
    }

    m_server_receive_audio = new QTcpServer();
    if(m_server_receive_audio->listen(QHostAddress::Any, 8897))
    {
       connect(m_server_receive_audio, &QTcpServer::newConnection, this, &MainWindow::newConnection_receive_audio);
    }
    else
    {
        exit(EXIT_FAILURE);
    }

    m_server_receive_message = new QTcpServer();
    if(m_server_receive_message->listen(QHostAddress::Any, 8898))
    {
       connect(m_server_receive_message, &QTcpServer::newConnection, this, &MainWindow::newConnection_receive_message);
       cout << "Listening port 8898" << endl;
    }
    else
    {
        exit(EXIT_FAILURE);
    }


    timer = new QTimer(this);
    connect( timer, &QTimer::timeout, this, &MainWindow::timer_event);
    timer->start(10);
    
    //add move mode items
    QStringList strList_MoveMode;

    ui->comboBox_MoveMode->addItems({"Manual",
                                     "Move body",
                                     "Move head"});
    connect(ui->comboBox_MoveMode,static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged),this,&MainWindow::comboBox_MoveMode_changed);
    ui->comboBox_MoveMode->setCurrentIndex(1);  //Move body

    ui->comboBox_DetectionMode->addItems({"Off",
        "On"});
    connect(ui->comboBox_DetectionMode,static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged),this,&MainWindow::comboBox_DetectionMode_changed);
    ui->comboBox_DetectionMode->setCurrentIndex(1);  //On

    ui->comboBox_Language->addItems({"Chinese",
        "English",
        "Arabic"});
    connect(ui->comboBox_Language,static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged),this,&MainWindow::comboBox_Language_changed);


    //Get keyboard press event
    setFocusPolicy(Qt::StrongFocus);

    devAudio = QMediaDevices::defaultAudioInput();
    std::cout << "devAudio.description()" << devAudio.description().toStdString() << std::endl;

    // setup audio format
    QAudioFormat format;
    format.setSampleRate(WHISPER_SAMPLE_RATE);
    format.setChannelCount(1);
    format.setSampleFormat(QAudioFormat::Float);

    if (devAudio.isFormatSupported(format))
    {
        audioSrc = new QAudioSource(devAudio, format);
    }
    else
    {
        std::cout << "Audio format not supported" << std::endl;
    }

    thread_process_image.pSendMessageManager = &sendMessageManager;
    thread_receive_message.pSendMessageManager = &sendMessageManager;
    thread_receive_message.mpThreadStateControl = &thread_state_control;
    thread_receive_message.mpThreadProcessImage = &thread_process_image;

    thread_state_control.m_pSendMessageManager = &sendMessageManager;
    thread_state_control.mpThreadWhisper = &thread_whisper;
    thread_state_control.mpThreadLLM = &thread_LLM;
    thread_state_control.mpThreadProcessImage = &thread_process_image;

    thread_LLM.mpThreadStateControl = &thread_state_control;

    pVideoWindow = std::make_unique<VideoWindow>(nullptr);
    thread_state_control.pVideoWindow = pVideoWindow.get();
    pVideoWindow->pThreadStateControl = &thread_state_control;

    connect(&thread_state_control, &ThreadStateControl::playVideoRequest, this, &MainWindow::onPlayVideoRequested);
    connect(&thread_state_control, &ThreadStateControl::playImageRequest, this, &MainWindow::onPlayImageRequested);

    // Delay showing the video window so it renders after MainWindow 
    // and successfully steals focus to become the top window.
    QTimer::singleShot(300, this, [this]() {
        if (pVideoWindow) {
            if( msetting.bVideoWindowFullScreen ) {
                pVideoWindow->showFullScreen();
            } else {
                pVideoWindow->show();
            }
            pVideoWindow->raise();
            pVideoWindow->activateWindow();
        }
    });

}

void MainWindow::on_pushButton_speak_clicked()
{
    //Get the content of the plainTextEdit_speak object, and send it to Robot.
    QString text = ui->plainTextEdit_speak->toPlainText();   //This line causes an exception. Why?
    RobotCommandProtobuf::RobotCommand command;
    command.set_speak_sentence(text.toStdString());
    sendMessageManager.AddMessage(command);

    QString action;
    action = "speak " + ui->plainTextEdit_speak->toPlainText();
    QString_SentCommands.append(action + "\n");
    ui->plainTextEdit_SentCommands->document()->setPlainText(QString_SentCommands);
    ui->plainTextEdit_SentCommands->verticalScrollBar()->setValue(ui->plainTextEdit_SentCommands->verticalScrollBar()->maximum());
}

void MainWindow::on_pushButton_onTTSComplete_clicked()
{
    thread_state_control.NotifyEvent("onTTSComplete", chrono::system_clock::now());
}

void MainWindow::send_move_body_command(float x, float y, int degree, int speed)
{
    RobotCommandProtobuf::RobotCommand command;
    x *= 100;
//    command.set_x(static_cast<int>(x));
    y *= 100;
//    command.set_y(static_cast<int>(y));
//    command.set_degree(degree);
//    command.set_bodyspeed(speed);
    sendMessageManager.AddMessage(command);
}

void MainWindow::on_listView_PredefinedAction_doubleClicked(const QModelIndex &index)
{
    RobotCommandProtobuf::RobotCommand command;
    command.set_motion(index.row());
    sendMessageManager.AddMessage(command);
}

void MainWindow::on_pushButton_speak_2_clicked()
{
    QString text = ui->plainTextEdit_LLM_response->toPlainText();
    RobotCommandProtobuf::RobotCommand command;
    command.set_speak_sentence(text.toStdString());
    sendMessageManager.AddMessage(command);
}


void MainWindow::on_pushButton_hideface_clicked()
{
    RobotCommandProtobuf::RobotCommand command;
    command.set_hideface(true);
    sendMessageManager.AddMessage(command);
}

void MainWindow::on_pushButton_killapp_clicked()
{
    RobotCommandProtobuf::RobotCommand command;
    command.set_killapp(true);
    sendMessageManager.AddMessage(command);
}

void MainWindow::on_listView_Content_doubleClicked(const QModelIndex &index)
{
    RobotCommandProtobuf::RobotCommand command;
    command.set_content(static_cast<QString>(ui->listView_Content->model()->data(index).toString()).toStdString());
    sendMessageManager.AddMessage(command);
    //debug
    cout << "Content: " << static_cast<QString>(ui->listView_Content->model()->data(index).toString()).toStdString() << endl;
}


void MainWindow::timer_event()
{
    if(thread_process_image.bNewoutFrame )
    {
        //2024/12/30, Debug info: I use a timer to update the frame. On some low-end PC, 
        //although I call imshow, the window does not refresh unless there is a signal sent
        //to the window such as mouse hovering. It seems caused by the hardward driver.
        //imshow is a high-level GUI. There is no extra argument for this function.
        //How to force the problem to update the window?
        if( msetting.bShowPreviewWindow )
        {
            cv::imshow("Image", thread_process_image.getOutFrame());
            cv::waitKey(1);    //I miss this line so that Ubuntu does not update the window.
        }
        thread_process_image.bNewoutFrame = false;
        //update pitch and yaw
        ui->lineEdit_yaw_now->setText(QString::number(robot_status.yaw_degree));
        ui->lineEdit_pitch_now->setText(QString::number(robot_status.pitch_degree));
    }

    if( thread_whisper.b_new_OperatorSentence )
    {
        thread_whisper.b_new_OperatorSentence = false;
        ui->plainTextEdit_speak->setPlainText(QString::fromStdString(thread_whisper.strOperatorSentence));
    }
    
    //If the voice recognition result is ready, update the plainTextEdit_received.
    WhisperData thisWhisperData = thread_whisper.getLatestResult();
    if( thisWhisperData.tSTTComplete != oldWhisperData.tSTTComplete)
    {
        ui->plainTextEdit_received->setPlainText(QString::fromStdString(thisWhisperData.sOutput));
        oldWhisperData = thisWhisperData;
    }
                    
    if( thread_LLM.b_new_LLM_response )
    {
        thread_LLM.b_new_LLM_response = false;
        ui->plainTextEdit_LLM_response->setPlainText(QString::fromStdString(thread_LLM.strResponse));

        //speak out
        bool bAutoSpeakOut = false;
        if( bAutoSpeakOut)
        {
            RobotCommandProtobuf::RobotCommand command;
            command.set_speak_sentence(thread_LLM.strResponse);
            sendMessageManager.AddMessage(command);
        }
    }
    sendMessageManager.Send();
}

void MainWindow::onPlayVideoRequested(const QString& videoPath)
{
    if (pVideoWindow) {
        // Calling showFullScreen() is often enough to show, raise, and focus the window.
        // Combining it with other calls like move() can confuse the window manager.
        //temporary comment to avoid the window being full-screen at the beginning
        //pVideoWindow->showFullScreen();
        pVideoWindow->raise();
        pVideoWindow->activateWindow();
        pVideoWindow->playVideo(videoPath);
    }
}

void MainWindow::onPlayImageRequested(const QString& imagePath)
{
    if (pVideoWindow) {
        pVideoWindow->showImage(imagePath);
        // Also bring the window to the front
        pVideoWindow->raise();
        pVideoWindow->activateWindow();
    }
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    if (pVideoWindow) {
        pVideoWindow->close();
    }
    QMainWindow::closeEvent(event);
}
