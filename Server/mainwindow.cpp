#include "mainwindow.h"
#include "utility_compass.hpp"
#include <QAbstractItemView>
#include <QDir>
#include <QMessageBox>
#include <QModelIndex>
#include <QPixmap>
#include <QStandardItemModel>
#include <QStringListModel>
#include <cmath>
#include <iostream>
#include <opencv2/calib3d.hpp>
#include <opencv2/features2d.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>
#ifdef USE_KEBBI
#include "Kebbi/ui_mainwindow.h"
#elif USE_ZENBO
#include "Zenbo/ui_mainwindow.h"
#endif
#include "RobotCommand.pb.h"
#include "ActionOption.hpp"
#include "LandmarkToRobotAction.hpp" //[MOHAMED]
#include "RobotStatus.hpp"
#include "ThreadLLM.hpp"
#include <QScrollBar>
#include <QTimer>
#include <opencv2/core.hpp>
#include <opencv2/highgui.hpp>

extern std::mutex gMutex_audio_buffer;
extern std::queue<short> AudioBuffer;
extern std::condition_variable cond_var_audio;
extern int PortAudio_stop_and_terminate();
extern bool gbPlayAudio;
extern RobotStatus robot_status;
// extern ActionOption action_option;

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    UISetting(ui);

    // One QTcpServer only listens to one port. If you want to listen to
    // multiple ports, you need to create multiple QTcpServer objects.
    m_server_receive_image = new QTcpServer();
    // 2024/12/27 The port number is also hard-coded. I need to modify it in the
    // future.
    if (m_server_receive_image->listen(QHostAddress::Any, 8895))
    {
        connect(m_server_receive_image, &QTcpServer::newConnection, this,
                &MainWindow::newConnection_receive_image);
    }
    else
    {
        exit(EXIT_FAILURE);
    }

    m_server_send_command = new QTcpServer();
    if (m_server_send_command->listen(QHostAddress::Any, 8896))
    {
        connect(m_server_send_command, &QTcpServer::newConnection, this,
                &MainWindow::newConnection_send_command);
    }
    else
    {
        exit(EXIT_FAILURE);
    }

    m_server_receive_audio = new QTcpServer();
    if (m_server_receive_audio->listen(QHostAddress::Any, 8897))
    {
        connect(m_server_receive_audio, &QTcpServer::newConnection, this,
                &MainWindow::newConnection_receive_audio);
    }
    else
    {
        exit(EXIT_FAILURE);
    }

    m_server_receive_message = new QTcpServer();
    if (m_server_receive_message->listen(QHostAddress::Any, 8898))
    {
        connect(m_server_receive_message, &QTcpServer::newConnection, this,
                &MainWindow::newConnection_receive_message);
        cout << "Listening port 8898" << endl;
    }
    else
    {
        exit(EXIT_FAILURE);
    }

    QTimer *timer = new QTimer(this);
    connect(timer, &QTimer::timeout, this, &MainWindow::timer_event);
    timer->start(10); // interval 10 milliseconds.

    /*
        ui->comboBox_Processor->addItems({"CPU",
            "GPU"});
        connect(ui->comboBox_Processor,static_cast<void
       (QComboBox::*)(int)>(&QComboBox::currentIndexChanged),this,&MainWindow::comboBox_Processor_changed);
    */

    // Get keyboard press event
    setFocusPolicy(Qt::StrongFocus);

    devAudio = QMediaDevices::defaultAudioInput();
    std::cout << "devAudio.description()"
              << devAudio.description().toStdString() << std::endl;

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
    thread_receive_message.mpWhisper = &thread_whisper;
    thread_receive_message.mpLogger = &mlogger;

    thread_state_control.m_pSendMessageManager = &sendMessageManager;
    thread_state_control.mpThreadWhisper = &thread_whisper;
    thread_state_control.mpThreadLLM = &thread_LLM;
    thread_state_control.mpThreadProcessImage = &thread_process_image;

    thread_LLM.mpThreadStateControl = &thread_state_control;

    thread_whisper.mpLogger = &mlogger;

    pVideoWindow = std::make_unique<VideoWindow>(nullptr);
    thread_state_control.pVideoWindow = pVideoWindow.get();
    pVideoWindow->pThreadStateControl = &thread_state_control;

    connect(&thread_state_control, &ThreadStateControl::playVideoRequest, this,
            &MainWindow::onPlayVideoRequested);
    connect(&thread_state_control, &ThreadStateControl::playImageRequest, this,
            &MainWindow::onPlayImageRequested);
    connect(&thread_state_control, &ThreadStateControl::playTextRequest, this,
            &MainWindow::onPlayTextRequested);

    // Delay showing the video window so it renders after MainWindow
    // and successfully steals focus to become the top window.
    QTimer::singleShot(300, this, [this]()
                       {
        if (pVideoWindow) {
            if (msetting.bVideoWindowFullScreen) {
                pVideoWindow->showFullScreen();
            } else {
                pVideoWindow->show();
            }
            pVideoWindow->raise();
            pVideoWindow->activateWindow();
        } });

    mlogger.SetLogDirectory(ReplaceShellVariable(msetting.LogDirectory));
    mlogger.SetLogFileName(msetting.LogFileName);
}

void MainWindow::startThreads()
{
    // run threads
    thread_process_image.start();
    thread_process_audio.start();
    thread_receive_message.start();
    thread_whisper.start();
    thread_LLM.start();
    thread_state_control.start();
}

void MainWindow::setSettingFile(const QString &filePath)
{
    LoadJSONFile(msetting, filePath.toStdString());

    // Use the loaded Language setting to show and use the correct language
    setLanguage(QString::fromStdString(msetting.Language));
    ui->comboBox_Language->setCurrentText(
        QString::fromStdString(msetting.Language));
    cout << "setLanguage msetting.Language: " << msetting.Language << endl;

    // Apply the other settings loaded from the file
    thread_whisper.model_file_path =
        QString::fromStdString(ReplaceShellVariable(msetting.WhisperModel));
    thread_process_image.ImageSaveDirectory =
        ReplaceShellVariable(msetting.ImageSaveDirectory);
    thread_process_image.bSaveTransmittedImage = msetting.bSaveImages;

    // After loading the setting, I can set the initial state of the state
    // control thread.
    // thread_state_control.SetSettingFile(filePath);
    thread_state_control.mpsetting = &msetting;
    thread_state_control.InitializeStates();
    //    thread_LLM.SetSettingFile(filePath);
    thread_LLM.mpsetting = &msetting;
    // thread_process_image.SetSettingFile(filePath);
    thread_process_image.mpsetting = &msetting;
    thread_receive_message.mpsetting = &msetting;
    thread_receive_message.SetRobotModel(msetting.RobotModel);
    thread_whisper.mpsetting = &msetting;

    ui->checkBox_UseVisualCompass->setChecked(msetting.bUseVisualCompass);
    ui->checkBox_SaveImages->setChecked(msetting.bSaveImages);
    thread_process_image.action_option.bUseVisualCompass =
        msetting.bUseVisualCompass;
}

MainWindow::~MainWindow()
{
    timer->stop(); // Stop the timer immediately

    // kill the app
    if (sendMessageManager.pSocket != nullptr &&
        sendMessageManager.pSocket->state() ==
            QAbstractSocket::ConnectedState)
    {
        RobotCommandProtobuf::RobotCommand command;
        command.set_hideface(1);
        sendMessageManager.AddMessage(command);
        sendMessageManager.Send();

        // I need to send 2 separate commands to ensure the first command is
        // sent out.
        RobotCommandProtobuf::RobotCommand command2;
        command2.set_killapp(true);
        sendMessageManager.AddMessage(command2);
        sendMessageManager.Send();

        sleep(1); // wait for 1 second to ensure the command is sent out.
    }

    // close thread_process_image's loop
    cout << "Waiting for thread_process_image to exit" << endl;
    thread_process_image.b_WhileLoop = false;
    thread_process_image.wait();
    foreach (QTcpSocket *socket, connection_set)
    {
        socket->close();
        socket->deleteLater();
    }
    m_server_receive_image->close();
    m_server_receive_image->deleteLater();

    foreach (QTcpSocket *socket, connection_set2)
    {
        socket->close();
        socket->deleteLater();
    }

    cout << "Waiting for thread_recieve_message to exit" << endl;
    thread_receive_message.b_WhileLoop = false;
    thread_receive_message.cond_var_receive_message.notify_one();
    thread_receive_message.wait();

    foreach (QTcpSocket *socket, connection_set4)
    {
        socket->close();
        socket->deleteLater();
    }
    m_server_receive_message->close();
    m_server_receive_message->deleteLater();

    thread_whisper.b_WhileLoop = false;
    thread_whisper.wait();
    if (audioSrc != nullptr)
        delete audioSrc;

    cout << "Waiting for thread_LLM to exit" << endl;
    thread_LLM.b_WhileLoop = false;
    thread_LLM.cond_var_thread_LLM.notify_one();
    thread_LLM.wait();

    cout << "Waiting for thread_state_control to exit" << endl;
    thread_state_control.b_WhileLoop = false;
    thread_state_control.cond_var_state_control.notify_one();
    thread_state_control.wait();

    // Bug: sometimes, the app got stuck here becuase thread_process_audio is
    // waiting for cond_var_audio.
    cout << "Waiting for thread_process_audio to exit" << endl;
    gbPlayAudio = false;         // This variable does not work yet.
    cond_var_audio.notify_one(); // I need to resume this thread.
    foreach (QTcpSocket *socket, connection_set3)
    {
        socket->close();
        socket->deleteLater();
    }
    m_server_receive_audio->close();
    m_server_receive_audio->deleteLater();
    PortAudio_stop_and_terminate();
    thread_process_audio.wait();

    delete ui;
}

/*
void MainWindow::setWhisperModelFile( QString filePath)
{
    thread_whisper.model_file_path = filePath;
}

void MainWindow::setState(int N)
{
    thread_state_control.SetIntialStateIndex(N);
}

void MainWindow::setImageSaveDirectory( QString ImageSaveDirectory)
{
    thread_process_image.ImageSaveDirectory = ImageSaveDirectory.toStdString();
}

void MainWindow::setDefaultSaveImage(bool bDefaultSaveImage)
{
    if( bDefaultSaveImage )
    {
        thread_process_image.bSaveTransmittedImage = true;
        ui->checkBox_SaveImages->setChecked(true);
    }
    else
    {
        thread_process_image.bSaveTransmittedImage = false;
        ui->checkBox_SaveImages->setChecked(false);
    }
}
*/

void MainWindow::setLanguage(QString Language)
{
    QString SentenceFileName;
    if (Language == "Chinese")
    {
        thread_whisper.strLanguage =
            "zh"; // set language to Chinese (keep this line as-is)
        SentenceFileName = "Sentence_Chinese.txt";
    }
    else if (Language == "English")
    {
        thread_whisper.strLanguage = "en"; // set language to English
        SentenceFileName = "Sentence_English.txt";
    }
    else if (Language == "Arabic")
    {
        thread_whisper.strLanguage = "ar"; // set language to Arabic
        SentenceFileName = "Sentence_English.txt";
    }
    else
    {
        throw "Unsupported language: " + Language.toStdString();
    }

    QFile textFile(SentenceFileName);
    if (textFile.open(QIODevice::ReadOnly))
    {
        QTextStream textStream(&textFile);
        for (int listView_index = 0; listView_index <= 2; listView_index++)
        {
            QStandardItemModel *ItemModel_sentence =
                new QStandardItemModel(this);
            for (int i = 0; i < 15; i++)
            {
                QString line = textStream.readLine();
                if (line.isNull())
                    break;
                else
                {
                    QStandardItem *item = new QStandardItem(line);
                    ItemModel_sentence->appendRow(item);
                }
            }

            switch (listView_index)
            {
            case 0:
                ui->listView_Sentence1->setModel(ItemModel_sentence);
                ui->listView_Sentence1->setEditTriggers(
                    QAbstractItemView::NoEditTriggers);
                break;
            case 1:
                ui->listView_Sentence2->setModel(ItemModel_sentence);
                ui->listView_Sentence2->setEditTriggers(
                    QAbstractItemView::NoEditTriggers);
                break;
            case 2:
                ui->listView_Sentence3->setModel(ItemModel_sentence);
                ui->listView_Sentence3->setEditTriggers(
                    QAbstractItemView::NoEditTriggers);
                break;
            }
        }
    }
    else
    {
        throw "Cannot open sentence file: " + SentenceFileName.toStdString();
    }
}

void MainWindow::newConnection_receive_image()
{
    thread_state_control.cond_var_state_control.notify_one();

    while (m_server_receive_image->hasPendingConnections())
    {
        QTcpSocket *socket = m_server_receive_image->nextPendingConnection();

        //        SocketClientHandler_Image* handler = new
        //        SocketClientHandler_Image(socket, this);
        SocketClientHandler *handler = new SocketClientHandler(socket, this);
        Handler_set.insert(handler);
        handler->socketBufferParser.pDataFrames_queue =
            &thread_process_image.DataFrames_queue;
        //        handler->socketBufferParser_Image.thread_process_image =
        //        &thread_process_image;
        qDebug() << "New connection 8895 from:"
                 << socket->peerAddress().toString();
    }
}

void MainWindow::newConnection_send_command()
{
    std::cout << "newConnction() 8896" << std::endl;
    while (m_server_send_command->hasPendingConnections())
        appendToSocketList2(
            m_server_send_command
                ->nextPendingConnection()); // the nextPendingConnection() will
                                            // retrieve a socket
}

void MainWindow::newConnection_receive_audio()
{
    std::cout << "newConnction() 8897" << std::endl;
    while (m_server_receive_audio->hasPendingConnections())
        appendToSocketList3(m_server_receive_audio->nextPendingConnection());
}

void MainWindow::newConnection_receive_message()
{
    while (m_server_receive_message->hasPendingConnections())
    {
        QTcpSocket *socket = m_server_receive_message->nextPendingConnection();

        SocketClientHandler *handler = new SocketClientHandler(socket, this);
        Handler_set.insert(handler);
        handler->socketBufferParser.pDataFrames_queue =
            &thread_receive_message.DataFrames_queue;
        handler->socketBufferParser.pNofitiedCondVar =
            &thread_receive_message.cond_var_receive_message;
        qDebug() << "New connection 8898 from:"
                 << socket->peerAddress().toString();
    }
}

// There is no readSocket because I only use this connection to send commands.
void MainWindow::appendToSocketList2(QTcpSocket *socket)
{
    connection_set2.insert(socket);
    sendMessageManager.pSocket = socket;
    connect(socket, &QTcpSocket::disconnected, this,
            &MainWindow::discardSocket2);
    connect(socket, &QAbstractSocket::errorOccurred, this,
            &MainWindow::displayError);
}

void MainWindow::appendToSocketList3(QTcpSocket *socket)
{
    connection_set3.insert(socket);
    connect(socket, &QTcpSocket::readyRead, this, &MainWindow::readSocket3);
    connect(socket, &QTcpSocket::disconnected, this,
            &MainWindow::discardSocket3);
    connect(socket, &QAbstractSocket::errorOccurred, this,
            &MainWindow::displayError);
}

// read audio data from socket
void MainWindow::readSocket3()
{
    QTcpSocket *socket = reinterpret_cast<QTcpSocket *>(sender());

    QDataStream socketStream(socket);
    socketStream.setVersion(QDataStream::Qt_DefaultCompiledVersion);
    qint64 byteAvailable = socket->bytesAvailable();

    socketStream.startTransaction();
    std::unique_ptr<char[]> pbuffer = std::make_unique<char[]>(byteAvailable);

    char *buffer_head = pbuffer.get();
    qint64 length = socketStream.readRawData(buffer_head, byteAvailable);
    short *pShort = (short *)buffer_head;
    long long sampleCount = length / 2;

    // Disable here if I don't want to play audio on the server side.
    if (msetting.bServerPlaysRobotReceivedAudio)
    {
        gMutex_audio_buffer.lock();
        for (long long i = 0; i < sampleCount; i++)
        {
            short value = *(pShort + i);
            AudioBuffer.push(
                value); // This AudioBuffer is used to play audio on the server
        }
        gMutex_audio_buffer.unlock();
    }

    if (bstream_recognition)
    {
        thread_whisper.mtx_whisper_buffer.lock();
        for (long long i = 0; i < sampleCount; i++)
        {
            short value = *(pShort + i);
            thread_whisper.pcmf32_new[i + thread_whisper.bufferlength] =
                ((float)value / 32768.0f);
        }
        thread_whisper.bufferlength += sampleCount;
        thread_whisper.mtx_whisper_buffer.unlock();
    }

    if (AudioBuffer.size() >= 1024)
        cond_var_audio.notify_one();

    if (!socketStream.commitTransaction())
    {
        QString message = QString("%1 :: Waiting for more data to come..")
                              .arg(socket->socketDescriptor());
        emit newMessage(message);
        return;
    }
}

void MainWindow::discardSocket2()
{
    QTcpSocket *socket = reinterpret_cast<QTcpSocket *>(sender());
    QSet<QTcpSocket *>::iterator it = connection_set2.find(socket);
    if (it != connection_set2.end())
    {
        cout << "INFO :: A client has just left the room 8896" << endl;
        connection_set2.remove(*it);
    }
    socket->deleteLater();
    sendMessageManager.pSocket = nullptr;
}

void MainWindow::discardSocket3()
{
    QTcpSocket *socket = reinterpret_cast<QTcpSocket *>(sender());
    QSet<QTcpSocket *>::iterator it = connection_set3.find(socket);
    if (it != connection_set3.end())
    {
        cout << "INFO :: A client has just left the room 8897" << endl;
        connection_set3.remove(*it);
    }

    socket->deleteLater();
}

void MainWindow::displayError(QAbstractSocket::SocketError socketError)
{
    switch (socketError)
    {
    case QAbstractSocket::RemoteHostClosedError:
        break;
    case QAbstractSocket::HostNotFoundError:
        QMessageBox::information(
            this, "QTCPServer",
            "The host was not found. Please check the host "
            "name and port settings.");
        break;
    case QAbstractSocket::ConnectionRefusedError:
        QMessageBox::information(
            this, "QTCPServer",
            "The connection was refused by the peer. Make sure QTCPServer is "
            "running, and check that the host name and port settings are "
            "correct.");
        break;
    default:
        QTcpSocket *socket = qobject_cast<QTcpSocket *>(sender());
        QMessageBox::information(this, "QTCPServer",
                                 QString("The following error occurred: %1.")
                                     .arg(socket->errorString()));
        break;
    }
}

void MainWindow::on_pushButton_voice_to_text_clicked()
{
    if (!bListening)
    {
        bListening = true;
        if (thread_whisper.pOperatorBuffer != NULL)
        {
            thread_whisper.pOperatorBuffer->close();
            delete thread_whisper.pOperatorBuffer;
            thread_whisper.pOperatorBuffer = NULL;
        }
        ui->pushButton_voice_to_text->setText("Stop(F2)");
        thread_whisper.pOperatorBuffer = new QBuffer();
        thread_whisper.pOperatorBuffer->open(QBuffer::WriteOnly);
        thread_whisper.pOperatorBuffer->reset();
        thread_whisper.bOperatorBuffer_open = true;
        audioSrc->start(thread_whisper.pOperatorBuffer);
    }
    else
    {
        bListening = false;
        audioSrc->stop();
        thread_whisper.pOperatorBuffer->close();
        thread_whisper.bOperatorBuffer_open = false;
        ui->pushButton_voice_to_text->setText("Voice to Text(F2)");
    }
}

void MainWindow::on_pushButton_movebody_clicked()
{
    QString x = ui->lineEdit_x->text();
    QString y = ui->lineEdit_y->text();
    QString degree = ui->lineEdit_degree->text();
    QString bodyspeed = ui->lineEdit_bodyspeed->text();
    float fx = x.toFloat();
    float fy = y.toFloat();
    send_move_body_command(fx, fy, degree.toInt(), bodyspeed.toInt());
}

void MainWindow::on_pushButton_movehead_clicked()
{
    QString yaw = ui->lineEdit_yaw->text();
    QString pitch = ui->lineEdit_pitch->text();
    QString headspeed = ui->lineEdit_headspeed->text();
    robot_status.yaw_degree = yaw.toInt();
    robot_status.pitch_degree = pitch.toInt();
    send_move_head_command(robot_status.yaw_degree, robot_status.pitch_degree,
                           headspeed.toInt());
}

void MainWindow::send_move_head_command(int yaw, int pitch, int speed)
{
    RobotCommandProtobuf::RobotCommand command;
    command.set_yaw(yaw);
    command.set_pitch(pitch);
    command.set_headspeed(speed);
    sendMessageManager.AddMessage(command);

    ui->lineEdit_yaw_now->setText(QString::number(robot_status.yaw_degree));
    ui->lineEdit_pitch_now->setText(QString::number(robot_status.pitch_degree));
}

void MainWindow::on_listView_FacialExpressions_doubleClicked(
    const QModelIndex &index)
{
    RobotCommandProtobuf::RobotCommand command;
    command.set_face(index.row());
    sendMessageManager.AddMessage(command);
}

void MainWindow::on_listView_Sentence1_doubleClicked(const QModelIndex &index)
{
    on_pushButton_speak_clicked();
}

void MainWindow::on_listView_Sentence1_clicked(const QModelIndex &index)
{
    QString itemText = index.data(Qt::DisplayRole).toString();
    ui->plainTextEdit_speak->setPlainText(itemText);
}

void MainWindow::on_listView_Sentence2_doubleClicked(const QModelIndex &index)
{
    on_pushButton_speak_clicked();
}

void MainWindow::on_listView_Sentence2_clicked(const QModelIndex &index)
{
    QString itemText = index.data(Qt::DisplayRole).toString();
    ui->plainTextEdit_speak->setPlainText(itemText);
}

void MainWindow::on_listView_Sentence3_doubleClicked(const QModelIndex &index)
{
    on_pushButton_speak_clicked();
}

void MainWindow::on_listView_Sentence3_clicked(const QModelIndex &index)
{
    QString itemText = index.data(Qt::DisplayRole).toString();
    ui->plainTextEdit_speak->setPlainText(itemText);
}

void MainWindow::comboBox_MoveMode_changed()
{
    switch (ui->comboBox_MoveMode->currentIndex())
    {
    case 0:
        thread_process_image.action_option.move_mode =
            ActionOption::MOVE_MANUAL;
        break;
    case 1: // move body
        thread_process_image.action_option.move_mode = ActionOption::MOVE_BODY;
        break;
    case 2: // move head
        thread_process_image.action_option.move_mode = ActionOption::MOVE_HEAD;
        break;
    }
}

// Control the detection mode
// This will no longer need because I can simutaneously detect face, pose, hand
// 20260219 ToDo: remove it. no longer needed it.
void MainWindow::comboBox_DetectionMode_changed()
{
    switch (ui->comboBox_DetectionMode->currentIndex())
    {
    case 0: // Off
        thread_process_image.b_HumanPoseEstimation = false;
        break;
    case 1: // On
        thread_process_image.b_HumanPoseEstimation = true;
        break;
    }
}

void MainWindow::comboBox_Language_changed()
{
    QString language = ui->comboBox_Language->currentText();
    setLanguage(language);
}

void MainWindow::on_pushButton_stop_action_clicked()
{
    RobotCommandProtobuf::RobotCommand command;
    command.set_stopmove(1);
    sendMessageManager.AddMessage(command);
}

void MainWindow::keyPressEvent(QKeyEvent *event)
{
    QString action;
    int key = event->key();
    //    std::cout << key << std::endl;
    bool bEffective = true;
    switch (key)
    {
    case 87: // w
        action = "w";
        send_move_body_command(0.5, 0, 0, 3);
        break;
    case 65: // a
        action = "a";
        send_move_body_command(0, 0, 15, 3);
        break;
    case 68: // d
        action = "d";
        send_move_body_command(0, 0, -15, 3);
        break;
    case 16777235: // up
        action = "up";
        robot_status.pitch_degree += 5;
        if (robot_status.pitch_degree > 55)
            robot_status.pitch_degree = 55;
        send_move_head_command(robot_status.yaw_degree,
                               robot_status.pitch_degree, 3);
        break;
    case 16777237: // down
        action = "down";
        robot_status.pitch_degree -= 5;
        if (robot_status.pitch_degree < -15)
            robot_status.pitch_degree = -15;
        send_move_head_command(robot_status.yaw_degree,
                               robot_status.pitch_degree, 3);
        break;
    case 16777234: // left
        action = "left";
        robot_status.yaw_degree += 5;
        if (robot_status.yaw_degree > 45)
            robot_status.yaw_degree = 45;
        send_move_head_command(robot_status.yaw_degree,
                               robot_status.pitch_degree, 3);
        break;
    case 16777236: // right
        action = "right";
        robot_status.yaw_degree -= 5;
        if (robot_status.yaw_degree < -45)
            robot_status.yaw_degree = -45;
        send_move_head_command(robot_status.yaw_degree,
                               robot_status.pitch_degree, 3);
        break;
    case 16777264: // F1
        on_pushButton_speak_clicked();
        break;
    case 16777265: // F2
        action = "voice to text";
        on_pushButton_voice_to_text_clicked();
        break;
    default:
        bEffective = false;
    }
    if (bEffective)
    {
        QString_SentCommands.append(action + "\n");
        ui->plainTextEdit_SentCommands->document()->setPlainText(
            QString_SentCommands);
        ui->plainTextEdit_SentCommands->verticalScrollBar()->setValue(
            ui->plainTextEdit_SentCommands->verticalScrollBar()->maximum());
    }
}

void MainWindow::on_checkBox_SaveImages_clicked()
{
    // std::cout << "on_CheckBox_SaveImages_clicked " << std::endl;
    if (ui->checkBox_SaveImages->isChecked())
    {
        thread_process_image.bSaveTransmittedImage = true;
    }
    else
    {
        thread_process_image.bSaveTransmittedImage = false;
    }
}

void MainWindow::on_checkBox_UseVisualCompass_clicked()
{
    if (ui->checkBox_UseVisualCompass->isChecked())
    {
        thread_process_image.action_option.bUseVisualCompass = true;
        msetting.bUseVisualCompass = true;
    }
    else
    {
        thread_process_image.action_option.bUseVisualCompass = false;
        msetting.bUseVisualCompass = false;
    }
}

void MainWindow::on_checkBox_stream_clicked(bool checked)
{
    if (checked)
    {

        bstream_recognition = true;
    }
    else
    {
        bstream_recognition = false;
    }
}

void MainWindow::on_pushButton_generate_response_clicked()
{
    QString text = ui->plainTextEdit_received->toPlainText();
    thread_whisper.ClearBuffer();
    //    thread_ollama.strPrompt = text.toStdString();      //The string is
    //    used here. thread_LLM.strPrompt = text.toStdString();      //The
    //    string is used here.
}

void MainWindow::on_pushButton_take_photo_clicked()
{
    bool ok = false;
    int angle = ui->lineEdit_photo_angle->text().toInt(&ok);
    if (!ok)
    {
        QMessageBox::warning(this, "Invalid Input",
                             "Please enter a valid integer for the angle.");
        return;
    }

    // Normalize angle to [0, 359]
    angle = (angle % 360 + 360) % 360;

    rotateAndTakePhoto(angle, "photo_" + QString::number(angle));
}

void MainWindow::on_pushButton_test_clicked()
{
    // 1. Get current image
    cv::Mat queryImg = thread_process_image.getLatestFrame();
    if (queryImg.empty())
    {
        std::cout << "Error: Current camera frame is empty." << std::endl;
        QMessageBox::warning(this, "Orientation Estimation",
                             "Error: Current camera frame is empty.");
        return;
    }

    std::cout
        << "Test button clicked: Estimating robot orientation using ORB..."
        << std::endl;
    std::string errorDetails;
    double estimatedAngle = ::ComputeVisualCompassTheta(
        queryImg, msetting.ImageSaveDirectory, errorDetails);
    if (estimatedAngle < 0.0)
    {
        if (!errorDetails.empty())
        {
            if (errorDetails.rfind("Error:", 0) == 0)
            {
                QMessageBox::warning(this, "Orientation Estimation",
                                     QString::fromStdString(errorDetails));
            }
            else
            {
                QMessageBox::information(this, "Orientation Estimation",
                                         QString::fromStdString(errorDetails));
            }
        }
        return;
    }
    QString resultMessage =
        QString("Estimated orientation angle: %1° (interpolated)\n")
            .arg(QString::number(estimatedAngle, 'f', 1));

    std::cout << resultMessage.toStdString() << std::endl;
    QMessageBox::information(this, "Orientation Estimation", resultMessage);
}

void MainWindow::onPlayVideoRequested(const QString &videoPath)
{
    if (pVideoWindow)
    {
        // Calling showFullScreen() is often enough to show, raise, and focus
        // the window. Combining it with other calls like move() can confuse the
        // window manager. temporary comment to avoid the window being
        // full-screen at the beginning pVideoWindow->showFullScreen();
        pVideoWindow->raise();
        pVideoWindow->activateWindow();
        pVideoWindow->playVideo(videoPath);
    }
}

void MainWindow::onPlayImageRequested(const QString &imagePath)
{
    if (pVideoWindow)
    {
        pVideoWindow->showImage(imagePath);
        // Also bring the window to the front
        //    pVideoWindow->raise();
        //    pVideoWindow->activateWindow();
    }
}

void MainWindow::onPlayTextRequested(const QString &ShowString)
{
    if (pVideoWindow)
    {
        QString formattedText = ShowString;
        formattedText.replace("\n", "<br>");
        QString styledText = "<span style='font-family: Arial; font-size: " +
                             QString::number(msetting.iTextFontSize) +
                             "pt; font-weight: bold;'>" + formattedText +
                             "</span>";
        pVideoWindow->showString(styledText);
        //    pVideoWindow->raise();
        //    pVideoWindow->activateWindow();
    }
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    if (pVideoWindow)
    {
        pVideoWindow->close();
    }
    QMainWindow::closeEvent(event);
}

void MainWindow::on_pushButton_onTTSComplete_clicked()
{
    thread_state_control.NotifyEvent("onTTSComplete",
                                     chrono::system_clock::now());
}

void MainWindow::on_pushButton_killapp_clicked()
{
    RobotCommandProtobuf::RobotCommand command;
    command.set_killapp(true);
    sendMessageManager.AddMessage(command);
}

void MainWindow::timer_event()
{
    if (thread_process_image.bNewoutFrame)
    {
        // 2024/12/30, Debug info: I use a timer to update the frame. On some
        // low-end PC, although I call imshow, the window does not refresh
        // unless there is a signal sent to the window such as mouse hovering.
        // It seems caused by the hardward driver. imshow is a high-level GUI.
        // There is no extra argument for this function. How to force the
        // problem to update the window?
        if (msetting.bShowPreviewWindow)
        {
            cv::imshow("Image", thread_process_image.getOutFrame());
            cv::waitKey(1); // I miss this line so that Ubuntu does not update
                            // the window.
        }
        thread_process_image.bNewoutFrame = false;
        // update pitch and yaw
        ui->lineEdit_yaw_now->setText(QString::number(robot_status.yaw_degree));
        ui->lineEdit_pitch_now->setText(
            QString::number(robot_status.pitch_degree));
    }

    if (thread_whisper.b_new_OperatorSentence)
    {
        thread_whisper.b_new_OperatorSentence = false;
        ui->plainTextEdit_speak->setPlainText(
            QString::fromStdString(thread_whisper.strOperatorSentence));
    }

    // If the voice recognition result is ready, update the
    // plainTextEdit_received.
    WhisperData thisWhisperData = thread_whisper.getLatestResult();
    if (thisWhisperData.tSTTComplete != oldWhisperData.tSTTComplete)
    {
        ui->plainTextEdit_received->setPlainText(
            QString::fromStdString(thisWhisperData.sOutput));
        oldWhisperData = thisWhisperData;
    }

    if (thread_LLM.b_new_LLM_response)
    {
        thread_LLM.b_new_LLM_response = false;
        ui->plainTextEdit_LLM_response->setPlainText(
            QString::fromStdString(thread_LLM.strResponse));

        // speak out
        bool bAutoSpeakOut = false;
        if (bAutoSpeakOut)
        {
            RobotCommandProtobuf::RobotCommand command;
            command.set_speak_sentence(thread_LLM.strResponse);
            sendMessageManager.AddMessage(command);
        }
    }
    sendMessageManager.Send();
}

// Kebbi's mbtx file
void MainWindow::on_listView_Content_doubleClicked(const QModelIndex &index)
{
    RobotCommandProtobuf::RobotCommand command;
    command.set_content(
        static_cast<QString>(
            ui->listView_Content->model()->data(index).toString())
            .toStdString());
    sendMessageManager.AddMessage(command);
    // debug
    cout << "Content: "
         << static_cast<QString>(
                ui->listView_Content->model()->data(index).toString())
                .toStdString()
         << endl;
}

// Zenbo's stop song button
void MainWindow::on_pushButton_stop_song_clicked()
{
    RobotCommandProtobuf::RobotCommand command;
    command.set_stopsong(1);
    sendMessageManager.AddMessage(command);
}

void MainWindow::on_pushButton_stop_speak_clicked()
{
    RobotCommandProtobuf::RobotCommand command;
    command.set_stop_speaking(1);
    sendMessageManager.AddMessage(command);
    // Notify ThreadStateControl that TTS is complete
    thread_state_control.NotifyEvent("onTTSComplete", chrono::system_clock::now());
    mlogger.LogToFile("Notify ThreadStateControl that TTS is complete");
}

void MainWindow::comboBox_Processor_changed()
{
    // Placeholder implementation to satisfy the Qt connection.
}

void MainWindow::on_listView_Song_doubleClicked(const QModelIndex &index)
{
    RobotCommandProtobuf::RobotCommand command;
    command.set_song(index.row());
    sendMessageManager.AddMessage(command);
    // debug
    cout << "Custom action: " << index.row() << endl;
}
