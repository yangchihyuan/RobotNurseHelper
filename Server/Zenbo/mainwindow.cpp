#include "mainwindow.h"
#include "./ui_mainwindow.h"     //in      RobotNurseHelper/Server/build/RobotNurseHelper_autogen/include/Zenbo
                                 //also in RobotNurseHelper/Server/build/RobotNurseHelper_autogen/include
#include <QSoundEffect>
#include <QUrl>
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
#include "ThreadLLM.hpp"

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
    strList.append("ACTIVE");
    strList.append("AWARE_LEFT");
    strList.append("AWARE_RIGHT");
    strList.append("CONFIDENT");
    strList.append("DEFAULT");
    strList.append("DEFAULT_STILL");
    strList.append("DOUBTING");
    strList.append("EXPECTING");
    strList.append("HAPPY");
    strList.append("HELPLESS");
    strList.append("HIDEFACE");
    strList.append("IMPATIENT");
    strList.append("INNOCENT");
    strList.append("INTERESTED");
    strList.append("LAZY");
    strList.append("PLEASED");
    strList.append("PRETENDING");
    strList.append("PROUD");
    strList.append("QUESTIONING");
    strList.append("SERIOUS");
    strList.append("SHOCKED");
    strList.append("SHY");
    strList.append("SINGING");
    strList.append("TIRED");
    strList.append("WORRIED");

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
    strList_action.append("Body_twist_1 large");
    strList_action.append("Body_twist_2 small");
    strList_action.append("Dance_2_loop nect move");
    strList_action.append("Dance_3_loop nect still");
    strList_action.append("Dance_b_1_loop nod+turn");
    strList_action.append("Dance_s_1_loop nod");
    strList_action.append("Default_1 to 15");
    strList_action.append("Default_2 no difference");
    strList_action.append("Find_face");
    strList_action.append("Head_down_1 slow");
    strList_action.append("Head_down_2 fast");
    strList_action.append("Head_down_3 slow");
    strList_action.append("Head_down_4 very slow");
    strList_action.append("Head_down_5 very slow");
    strList_action.append("Head_down_7 slow to 0");
    strList_action.append("Head_twist_1_loop");
    strList_action.append("Head_up_1 +10 fast");
    strList_action.append("Head_up_2 +10 slow");
    strList_action.append("Head_up_3 to 30 slow");
    strList_action.append("Head_up_4 to 15 slow");
    strList_action.append("Head_up_5 +10 very slow");
    strList_action.append("Head_up_6 +30 normal");
    strList_action.append("Head_up_7 +10 fast");
    strList_action.append("Music_1_loop");
    strList_action.append("Nod_1");
    strList_action.append("Shake_head_1 +5 slow left");
    strList_action.append("Shake_head_2 -15 slow left");
    strList_action.append("Shake_head_3 quick two sides");
    strList_action.append("Shake_head_4_loop");
    strList_action.append("Shake_head_5 slow two sides");
    strList_action.append("Shake_head_6 -10 no shake");
    strList_action.append("Turn_left_1 neck from + to 0");
    strList_action.append("Turn_left_2 body 20");
    strList_action.append("Turn_left_reverse_1 neck +20");
    strList_action.append("Turn_left_reverse_2 body +15");
    strList_action.append("Turn_right_1 neck 22.5");
    strList_action.append("Turn_right_2 body 20");
    strList_action.append("Turn_right_reverse_1 neck 22.5");
    strList_action.append("Turn_right_reverse_2 body 20");

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


    QStringList strList_customaction;
    strList_customaction.append("小布也愛你喲");
    strList_customaction.append("Chipi Chipi");

    QStandardItemModel* ItemModel_customaction = new QStandardItemModel(this);
    nCount = strList_customaction.size();
    for(int i = 0; i < nCount; i++)
    {
        QString string = static_cast<QString>(strList_customaction.at(i));
        QStandardItem *item = new QStandardItem(string);
        ItemModel_customaction->appendRow(item);
    }
    ui->listView_Song->setModel(ItemModel_customaction);
    ui->listView_Song->setEditTriggers(QAbstractItemView::NoEditTriggers);

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

    thread_receive_messages = new QTcpServer();
    if(thread_receive_messages->listen(QHostAddress::Any, 8898))
    {
       connect(thread_receive_messages, &QTcpServer::newConnection, this, &MainWindow::newConnection_Tablet);
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
    QString speed = ui->lineEdit_speed->text();
    QString volume = ui->lineEdit_volume->text();
    QString speak_pitch = ui->lineEdit_speak_pitch->text();
    RobotCommandProtobuf::RobotCommand command;
    command.set_speak_sentence(text.toStdString());
    command.set_speed(speed.toInt());
    command.set_volume(volume.toInt());
    command.set_speak_pitch(speak_pitch.toInt());
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
    command.set_x(static_cast<int>(x));
    y *= 100;
    command.set_y(static_cast<int>(y));
    command.set_degree(degree);
    command.set_bodyspeed(speed);
    sendMessageManager.AddMessage(command);
}

void MainWindow::on_listView_PredefinedAction_doubleClicked(const QModelIndex &index)
{
    RobotCommandProtobuf::RobotCommand command;
    command.set_predefined_action(index.row());
    sendMessageManager.AddMessage(command);
}

void MainWindow::on_listView_Song_doubleClicked(const QModelIndex &index)
{
    RobotCommandProtobuf::RobotCommand command;
    command.set_song(index.row());
    sendMessageManager.AddMessage(command);
    //debug
    cout << "Custom action: " << index.row() << endl;
}

void MainWindow::on_pushButton_speak_2_clicked()
{
    QString text = ui->plainTextEdit_LLM_response->toPlainText();
    QString speed = ui->lineEdit_speed->text();
    QString volume = ui->lineEdit_volume->text();
    QString speak_pitch = ui->lineEdit_speak_pitch->text();
    RobotCommandProtobuf::RobotCommand command;
    command.set_speak_sentence(text.toStdString());
    command.set_speed(speed.toInt());
    command.set_volume(volume.toInt());
    command.set_speak_pitch(speak_pitch.toInt());
    sendMessageManager.AddMessage(command);
}


void MainWindow::on_pushButton_hideface_clicked()
{
    RobotCommandProtobuf::RobotCommand command;
    command.set_hideface(1);
    sendMessageManager.AddMessage(command);
}

void MainWindow::on_pushButton_stop_song_clicked()
{
    RobotCommandProtobuf::RobotCommand command;
    command.set_stopsong(1);
    sendMessageManager.AddMessage(command);
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
        cv::imshow("Image", outFrame);
        cv::waitKey(1);    //I miss this line so that Ubuntu does not update the window.
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

    if( thread_whisper.b_new_RobotSentence )
    {
        thread_whisper.b_new_RobotSentence = false;
        ui->plainTextEdit_received->setPlainText(QString::fromStdString(thread_whisper.strRobotSentence));
    }

    if( thread_whisper.b_RobotSentence_End )
    {
        thread_whisper.b_RobotSentence_End = false;
        //send a command as the push button clicked
        ui->pushButton_generate_response->click();      //This funciton will call the MainWindow::on_pushButton_generate_response_clicked() function.
    }

    if( thread_ollama.b_new_LLM_response )
    {
        thread_ollama.b_new_LLM_response = false;
        ui->plainTextEdit_LLM_response->setPlainText(QString::fromStdString(thread_ollama.strResponse));
        //speak out
        bool bAutoSpeakOut = true;
        if( bAutoSpeakOut)
        {
            RobotCommandProtobuf::RobotCommand command;
            command.set_speak_sentence(thread_ollama.strResponse);
            sendMessageManager.AddMessage(command);
        }
    }

    sendMessageManager.Send();
}

void MainWindow::rotateAndTakePhoto(int targetAngle, const QString& prefix)
{
    static QSoundEffect *shutterSound = nullptr;
    if (!shutterSound) {
        shutterSound = new QSoundEffect(this);
        shutterSound->setSource(QUrl::fromLocalFile("camera-shutter.wav"));
        shutterSound->setVolume(1.0f);
    }

    int relative = targetAngle - current_body_angle;
    // Normalize to [-180, 180]
    while (relative > 180) relative -= 360;
    while (relative <= -180) relative += 360;

    int speed = ui->lineEdit_bodyspeed->text().toInt();
    if (speed <= 0) speed = 3; // Default fallback

    if (relative != 0) {
        send_move_body_command(0, 0, relative, speed);
        std::cout << "Rotating robot body by: " << relative << " degrees to target " << targetAngle << " degrees at speed " << speed << std::endl;
    } else {
        std::cout << "Robot body already at target " << targetAngle << " degrees." << std::endl;
    }
    int delay = (relative != 0) ? 2500 : 500;

    QTimer::singleShot(delay, this, [this, prefix]() {
        thread_process_image.requestedPhotoPrefix = prefix.toStdString();
        thread_process_image.bSaveRequestedPhoto = true;
        std::cout << "Triggered single photo capture for: " << prefix.toStdString() << std::endl;
        if (shutterSound) {
            shutterSound->play();
        }
    });

    current_body_angle = targetAngle;
}