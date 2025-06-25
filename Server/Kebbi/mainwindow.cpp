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
#include "ThreadOllama.hpp"

extern std::mutex gMutex_audio_buffer;
extern std::queue<short> AudioBuffer;
extern std::condition_variable cond_var_audio;
extern cv::Mat outFrame;
extern int PortAudio_stop_and_terminate();
extern bool gbPlayAudio;
extern RobotStatus robot_status;
extern ActionOption action_option;

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

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

    //One QTcpServer only listens to one port. If you want to listen to multiple ports, you need to create multiple QTcpServer objects.
    m_server_receive_image = new QTcpServer();
    //2024/12/27 The port number is also hard-coded. I need to modify it in the future.
    if(m_server_receive_image->listen(QHostAddress::Any, 8895))
    {
       connect(m_server_receive_image, &QTcpServer::newConnection, this, &MainWindow::newConnection);
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

    m_server_Tablet = new QTcpServer();
    if(m_server_Tablet->listen(QHostAddress::Any, 8898))
    {
       connect(m_server_Tablet, &QTcpServer::newConnection, this, &MainWindow::newConnection_Tablet);
       cout << "Listening port 8898" << endl;
    }
    else
    {
        exit(EXIT_FAILURE);
    }


    QTimer *timer = new QTimer(this);
    connect( timer, &QTimer::timeout, this, &MainWindow::timer_event);
    timer->start(10);

    //add move mode items
    QStringList strList_MoveMode;

    ui->comboBox_MoveMode->addItems({"Manual",
                                     "Move body",
                                     "Move head"});
    connect(ui->comboBox_MoveMode,static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged),this,&MainWindow::comboBox_MoveMode_changed);

    ui->comboBox_DetectionMode->addItems({"None",
        "Face",
        "Pose",
        "Holistic"});
    connect(ui->comboBox_DetectionMode,static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged),this,&MainWindow::comboBox_DetectionMode_changed);

    ui->comboBox_Processor->addItems({"CPU",
        "GPU"});
    connect(ui->comboBox_Processor,static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged),this,&MainWindow::comboBox_Processor_changed);

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
    thread_process_image.pSocketHandler = &socketHandler1;
    thread_tablet.pSocketHandler = &socketHandler4;
    thread_tablet.pSendMessageManager = &sendMessageManager;
}

void MainWindow::on_pushButton_speak_clicked()
{
    //Get the content of the plainTextEdit_speak object, and send it to Robot.
    QString text = ui->plainTextEdit_speak->toPlainText();   //This line causes an exception. Why?
    RobotCommandProtobuf::RobotCommand command;
    command.set_speak_sentence(text.toStdString());
    if( ui->checkBox_withface->isChecked() )
    {
        QModelIndex index = ui->listView_FacialExpressions->currentIndex();
        command.set_face(index.row());
    }

    sendMessageManager.AddMessage(command);

    QString action;
    action = "speak " + ui->plainTextEdit_speak->toPlainText();
    QString_SentCommands.append(action + "\n");
    ui->plainTextEdit_SentCommands->document()->setPlainText(QString_SentCommands);
    ui->plainTextEdit_SentCommands->verticalScrollBar()->setValue(ui->plainTextEdit_SentCommands->verticalScrollBar()->maximum());
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


