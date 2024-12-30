#include "mainwindow.h"
#include "./ui_mainwindow.h"
#include <QPixmap>
#include <QStringListModel>
#include <QStandardItemModel>
#include <QModelIndex>
#include <QAbstractItemView>
#include <iostream>
#include "ServerSend.pb.h"
#include <QTimer>
#include <opencv2/core.hpp>
#include <opencv2/highgui.hpp>
#include <QScrollBar>


extern std::mutex gMutex_audio_buffer;
extern std::queue<short> AudioBuffer;
extern std::condition_variable cond_var_audio;
extern bool bNewoutFrame;
extern cv::Mat outFrame;
extern int PortAudio_stop_and_terminate();
extern bool gbPlayAudio;


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

    QFile textFile("Sentence.txt");
    if(textFile.open(QIODevice::ReadOnly))
    {
        QTextStream textStream(&textFile);
        for( int listView_index = 0; listView_index <= 2; listView_index++)
        {
            QStandardItemModel* ItemModel_sentence = new QStandardItemModel(this);
            for(int i=0;i<15;i++)
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
            
            switch(listView_index)
            {
                case 0:
                    ui->listView_Sentence1->setModel(ItemModel_sentence);
                    ui->listView_Sentence1->setEditTriggers(QAbstractItemView::NoEditTriggers);
                    break;
                case 1:
                    ui->listView_Sentence2->setModel(ItemModel_sentence);
                    ui->listView_Sentence2->setEditTriggers(QAbstractItemView::NoEditTriggers);
                    break;
                case 2:
                    ui->listView_Sentence3->setModel(ItemModel_sentence);
                    ui->listView_Sentence3->setEditTriggers(QAbstractItemView::NoEditTriggers);
                    break;
            }
        }
    } 


    //allocate buffer
    frame_buffer = std::make_unique<char[]>(buffer_size);   //how large should be the buffer size?
    buffer_length = 0;
    iEndOfAFrame = 0;

    //run threads
    thread_process_image.start();
    thread_send_command.start();
    thread_process_image.pSendCommandThread = &thread_send_command;
    thread_process_audio.start();

    //sockets
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

    QTimer *timer = new QTimer(this);
    connect( timer, &QTimer::timeout, this, &MainWindow::timer_event);
    timer->start(10);

    //add move mode items
    QStringList strList_MoveMode;

    ui->comboBox_MoveMode->addItems({"Manual","Look for people"});
    connect(ui->comboBox_MoveMode,static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged),this,&MainWindow::comboBox_MoveMode_changed);

    //Get keyboard press event
    setFocusPolicy(Qt::StrongFocus);

    devAudio = QMediaDevices::defaultAudioInput();
    std::cout << "devAudio.description()" << devAudio.description().toStdString() << std::endl;

    // setup audio format
    QAudioFormat format;
    //2024/8/21 disable whisper.cpp
//    format.setSampleRate(WHISPER_SAMPLE_RATE);
    format.setChannelCount(1);
    format.setSampleFormat(QAudioFormat::Float);

    if (devAudio.isFormatSupported(format))
    {
        audioSrc = new QAudioSource(devAudio, format);
    }

    // initial whisper.cpp
    //2024/8/21 disable whisper.cpp
    /*
    whisper_context_params cparams = whisper_context_default_params();
    cparams.use_gpu = true;
    //language is not set in the cparams
    ctx = whisper_init_from_file_with_params(
//      R"(/home/chihyuan/whisper.cpp/models/ggml-small.bin)", cparams);        
      R"(/home/chihyuan/whisper.cpp/models/ggml-base.bin)", cparams);
//      R"(/home/chihyuan/whisper.cpp/models/ggml-medium.bin)", cparams);    //waiting time too long

    //connect message and SendCommandThread
    */
    connect( this, &MainWindow::addSendCommandMessage, &thread_send_command, &SendCommandThread::AddMessage);
}

MainWindow::~MainWindow()
{
    //close thread's loop
    thread_process_image.b_WhileLoop = false;
    thread_process_image.cond_var_process_image.notify_one();
    thread_process_image.wait();
    foreach (QTcpSocket* socket, connection_set)
    {
        socket->close();
        socket->deleteLater();
    }
    m_server_receive_image->close();
    m_server_receive_image->deleteLater();


    //order: close the thread --> close the socket --> close the server
    thread_send_command.b_KeepLoop = false;
    thread_send_command.cond_var_report_result.notify_one();
    thread_send_command.wait();
    foreach (QTcpSocket* socket, connection_set2)
    {
        socket->close();
        socket->deleteLater();
    }
    m_server_send_command->close();
    m_server_send_command->deleteLater();


    gbPlayAudio = false;
    cond_var_audio.notify_one();      //I need to resume this thread.
    PortAudio_stop_and_terminate();   //otherwise, this funtion will got stuck
    thread_process_audio.wait();
    foreach (QTcpSocket* socket, connection_set3)
    {
        socket->close();
        socket->deleteLater();
    }
    m_server_receive_audio->close();
    m_server_receive_audio->deleteLater();

    if (audioSrc != nullptr)
      delete audioSrc;
 
    //2024/8/21 disable whisper.cpp
//    whisper_free(ctx);

    delete ui;
}

void MainWindow::newConnection()
{
    std::cout << "newConnction()" << std::endl;
    while (m_server_receive_image->hasPendingConnections())
        appendToSocketList(m_server_receive_image->nextPendingConnection());
}

void MainWindow::newConnection_send_command()
{
    while (m_server_send_command->hasPendingConnections())
        appendToSocketList2(m_server_send_command->nextPendingConnection());
}

void MainWindow::newConnection_receive_audio()
{
    while (m_server_receive_audio->hasPendingConnections())
        appendToSocketList3(m_server_receive_audio->nextPendingConnection());
}


void MainWindow::appendToSocketList(QTcpSocket* socket)
{
    connection_set.insert(socket);
    connect(socket, &QTcpSocket::readyRead, this, &MainWindow::readSocket);
    connect(socket, &QTcpSocket::disconnected, this, &MainWindow::discardSocket);
    connect(socket, &QAbstractSocket::errorOccurred, this, &MainWindow::displayError);
}

void MainWindow::appendToSocketList2(QTcpSocket* socket)
{
    connection_set2.insert(socket);
    thread_send_command.pSocket = socket;
    connect(socket, &QTcpSocket::disconnected, this, &MainWindow::discardSocket2);
    connect(socket, &QAbstractSocket::errorOccurred, this, &MainWindow::displayError);
}

void MainWindow::appendToSocketList3(QTcpSocket* socket)
{
    connection_set3.insert(socket);
    connect(socket, &QTcpSocket::readyRead, this, &MainWindow::readSocket3);
    connect(socket, &QTcpSocket::disconnected, this, &MainWindow::discardSocket3);
    connect(socket, &QAbstractSocket::errorOccurred, this, &MainWindow::displayError);
}

void MainWindow::readSocket()
{
    QTcpSocket* socket = reinterpret_cast<QTcpSocket*>(sender());

    QByteArray buffer;

    QDataStream socketStream(socket);
//    socketStream.setVersion(QDataStream::Qt_DefaultCompiledVersion);
    qint64 byteAvailable = socket->bytesAvailable();

//    socketStream.startTransaction();
    char* frame_buffer_head = frame_buffer.get();
    int buffer_length_old = buffer_length;
    //Valgrind reports an error message here Invalid write of size 2. Why?
    //prevent buffer overflow
    if(buffer_length+byteAvailable < buffer_size )
    {
        qint64 readlength = socketStream.readRawData(frame_buffer_head+buffer_length, byteAvailable);     //I can get data by using readRawData
        buffer_length += readlength;     //buffer gets longer from the read data

        //look for the keyword "EndOfAFrame" in the buffer.
        //The bug occurs if buffer_length_old <11
        string pattern("EndOfAFrame");
        int pattern_length = pattern.length();
        string haystack;
        int begin_pos = 0;
        if(buffer_length_old >= pattern_length)
        {
            begin_pos = buffer_length_old - pattern_length;
            haystack.assign(frame_buffer_head + buffer_length_old - pattern_length, readlength+pattern_length); 
        }
        else
            haystack.assign(frame_buffer_head, buffer_length); 

        size_t n = haystack.find(pattern);

        //if found, copy buffer to frame_buffer1
        if (n != string::npos)
        {
            if( iEndOfAFrame % 1000 == 0)
                cout << "Found EndOfAFrame " << iEndOfAFrame++ << endl;
            else
                iEndOfAFrame++;

            int frame_length = begin_pos + n + pattern_length;   //include the pattern "EndOfAFrame"
            thread_process_image.mutex_frame_buffer1.lock();
            //To prevent thread_process_image's frame_buffer1 overflow
            if( frame_length <= thread_process_image.get_buffer_size())
                copy(frame_buffer_head,frame_buffer_head+frame_length,thread_process_image.frame_buffer1.get());
            else
            {
                cout << "error, frame_length > buffer_size" << std::endl;
                throw std::exception();
            }

            thread_process_image.b_frame_buffer1_unused = true;
            thread_process_image.frame_buffer1_length = frame_length;
            thread_process_image.mutex_frame_buffer1.unlock();
            thread_process_image.cond_var_process_image.notify_one();

            //move data
            int remaining_length = buffer_length - frame_length;
            if( remaining_length > 0)
                copy(frame_buffer_head+frame_length,
                        frame_buffer_head+buffer_length,
                        frame_buffer_head);
            
            //update buffer_length
            buffer_length = remaining_length;
        }
    }

//        if(!socketStream.commitTransaction())
//        {
//            QString message = QString("%1 :: Waiting for more data to come..").arg(socket->socketDescriptor());
//            emit newMessage(message);
//            return;
//        }
}


void MainWindow::readSocket3()
{
    QTcpSocket* socket = reinterpret_cast<QTcpSocket*>(sender());

    QByteArray buffer;

    QDataStream socketStream(socket);
    socketStream.setVersion(QDataStream::Qt_DefaultCompiledVersion);
    qint64 byteAvailable = socket->bytesAvailable();

    socketStream.startTransaction();
    std::unique_ptr<char[]> pbuffer = std::make_unique<char[]>(byteAvailable);

    char* buffer_head = pbuffer.get();
    qint64 length = socketStream.readRawData(buffer_head, byteAvailable);
    short *pShort = (short *)buffer_head;
    gMutex_audio_buffer.lock();
    for( long long i = 0; i<length/2 ; i++)
    {
        short value = *(pShort + i);
        AudioBuffer.push(value);
    }
    gMutex_audio_buffer.unlock();

//    std::cout << "AudioBuffer.size()" << AudioBuffer.size() << std::endl;
    if( AudioBuffer.size() >= 1024)
        cond_var_audio.notify_one();

    if(!socketStream.commitTransaction())
    {
        QString message = QString("%1 :: Waiting for more data to come..").arg(socket->socketDescriptor());
        emit newMessage(message);
        return;
    }

}


void MainWindow::discardSocket()
{
    QTcpSocket* socket = reinterpret_cast<QTcpSocket*>(sender());
    QSet<QTcpSocket*>::iterator it = connection_set.find(socket);
    if (it != connection_set.end()){
        displayMessage(QString("INFO :: A client has just left the room").arg(socket->socketDescriptor()));
        connection_set.remove(*it);
    }
    
    socket->deleteLater();
}

void MainWindow::discardSocket2()
{
    QTcpSocket* socket = reinterpret_cast<QTcpSocket*>(sender());
    QSet<QTcpSocket*>::iterator it = connection_set2.find(socket);
    if (it != connection_set2.end()){
        displayMessage(QString("INFO :: A client has just left the room").arg(socket->socketDescriptor()));
        connection_set2.remove(*it);
        thread_send_command.pSocket = NULL;
    }
    
    socket->deleteLater();
}

void MainWindow::discardSocket3()
{
    QTcpSocket* socket = reinterpret_cast<QTcpSocket*>(sender());
    QSet<QTcpSocket*>::iterator it = connection_set3.find(socket);
    if (it != connection_set3.end()){
        displayMessage(QString("INFO :: A client has just left the room").arg(socket->socketDescriptor()));
        connection_set3.remove(*it);
    }
    
    socket->deleteLater();
}

void MainWindow::displayError(QAbstractSocket::SocketError socketError)
{
    switch (socketError) {
        case QAbstractSocket::RemoteHostClosedError:
        break;
        case QAbstractSocket::HostNotFoundError:
            QMessageBox::information(this, "QTCPServer", "The host was not found. Please check the host name and port settings.");
        break;
        case QAbstractSocket::ConnectionRefusedError:
            QMessageBox::information(this, "QTCPServer", "The connection was refused by the peer. Make sure QTCPServer is running, and check that the host name and port settings are correct.");
        break;
        default:
            QTcpSocket* socket = qobject_cast<QTcpSocket*>(sender());
            QMessageBox::information(this, "QTCPServer", QString("The following error occurred: %1.").arg(socket->errorString()));
        break;
    }
}

//This function is used by the automatically generated moc.
//What does moc mean?
void MainWindow::displayMessage(const QString& str)
{
//    ui->textBrowser_receivedMessages->append(str);
}

void MainWindow::on_pushButton_speak_clicked()
{
    //Get the content of the plainTextEdit_speak object, and send it to Robot.
    QString text = ui->plainTextEdit_speak->toPlainText();   //This line causes an exception. Why?
    QString speed = ui->lineEdit_speed->text();
    QString volume = ui->lineEdit_volume->text();
    QString speak_pitch = ui->lineEdit_speak_pitch->text();
    ZenboNurseHelperProtobuf::ReportAndCommand report_data;
    report_data.set_speak_sentence(text.toStdString());
    report_data.set_speed(speed.toInt());
    report_data.set_volume(volume.toInt());
    report_data.set_speak_pitch(speak_pitch.toInt());
    if( ui->checkBox_withface->isChecked() )
    {
        QModelIndex index = ui->listView_FacialExpressions->currentIndex();
        report_data.set_face(index.row());
    }
    //The segmentation fault only occurs when sockets are connected. Why?
    //Something wrong in the thread?
//    emit addSendCommandMessage(report_data);

    thread_send_command.AddMessage(report_data);

    QString action;
    action = "speak " + ui->plainTextEdit_speak->toPlainText();
    QString_SentCommands.append(action + "\n");
    ui->plainTextEdit_SentCommands->document()->setPlainText(QString_SentCommands);
    ui->plainTextEdit_SentCommands->verticalScrollBar()->setValue(ui->plainTextEdit_SentCommands->verticalScrollBar()->maximum());
}

void MainWindow::on_pushButton_voice_to_text_clicked()
{
    if( !bListening)
    {
        bListening = true;
        ui->pushButton_voice_to_text->setText("Stop(F2)");
        buffer.reset();
        buffer.open(QBuffer::WriteOnly);
        audioSrc->start(&buffer);
    }
    else
    {
        bListening = false;
        audioSrc->stop();
        buffer.close();
    //2024/8/21 disable whisper.cpp
/*
        whisper_full_params wparams = whisper_full_default_params(WHISPER_SAMPLING_GREEDY);
        wparams.translate = false;
        wparams.language = "zh";
        wparams.n_threads = 4;
        wparams.no_context = true;
        if (whisper_full(ctx, wparams, (float*)buffer.buffer().constData(), buffer.size() / 4) == 0)
        {
            std::string text = "";
            const int n_segments = whisper_full_n_segments(ctx);
            for (int i = 0; i < n_segments; ++i)
            text += whisper_full_get_segment_text(ctx, i);

            ui->plainTextEdit_speak->setPlainText(QString::fromStdString(text));
        }
        */
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

void MainWindow::send_move_body_command(float x, float y, int degree, int speed)
{
    ZenboNurseHelperProtobuf::ReportAndCommand report_data;
    x *= 100;
    report_data.set_x(static_cast<int>(x));
    y *= 100;
    report_data.set_y(static_cast<int>(y));
    report_data.set_degree(degree);
    report_data.set_bodyspeed(speed);
    thread_send_command.AddMessage(report_data);
}

void MainWindow::on_pushButton_movehead_clicked()
{
    QString yaw = ui->lineEdit_yaw->text();
    QString pitch = ui->lineEdit_pitch->text();
    QString headspeed = ui->lineEdit_headspeed->text();
    m_iyaw = yaw.toInt();
    m_ipitch = pitch.toInt();
    send_move_head_command(m_iyaw, m_ipitch, headspeed.toInt());
}

void MainWindow::send_move_head_command(int yaw, int pitch, int speed)
{
    ZenboNurseHelperProtobuf::ReportAndCommand report_data;
    report_data.set_yaw(yaw);
    report_data.set_pitch(pitch);
    report_data.set_headspeed(speed);
    thread_send_command.AddMessage(report_data);

    ui->lineEdit_yaw_now->setText(QString::number(m_iyaw));
    ui->lineEdit_pitch_now->setText(QString::number(m_ipitch));
}

void MainWindow::on_listView_FacialExpressions_doubleClicked(const QModelIndex &index)
{
    ZenboNurseHelperProtobuf::ReportAndCommand report_data;
    report_data.set_face(index.row());
    thread_send_command.AddMessage(report_data);
}

void MainWindow::on_listView_PredefinedAction_doubleClicked(const QModelIndex &index)
{
    ZenboNurseHelperProtobuf::ReportAndCommand report_data;
    report_data.set_predefined_action(index.row());
    thread_send_command.AddMessage(report_data);
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

void MainWindow::timer_event()
{
    if(bNewoutFrame )
    {
        //2024/12/30, the bug is here. I use a timer to update the frame. On some low-end PC, 
        //although I call imshow, the window does not refresh unless there is a signal sent
        //to the window such as mouse hovering. It seems caused by the hardward driver.
        //imshow is a high-level GUI. There is no extra argument for this function.
        //How to force the problem to update the window?
        cv::imshow("Image", outFrame);
        bNewoutFrame = false;
    }
}

void MainWindow::comboBox_MoveMode_changed()
{
    ZenboNurseHelperProtobuf::ReportAndCommand report_data;
    switch(ui->comboBox_MoveMode->currentIndex())
    {
        case 0:
            report_data.set_movemode(ZenboNurseHelperProtobuf::ReportAndCommand_MoveModeEnum_Manual);
            thread_process_image.b_HumanPoseEstimation = false;
            break;
        case 1:
            report_data.set_movemode(ZenboNurseHelperProtobuf::ReportAndCommand_MoveModeEnum_LookForPeople);
            thread_process_image.b_HumanPoseEstimation = true;
            break;
    }
    thread_send_command.AddMessage(report_data);
}

void MainWindow::on_pushButton_stop_action_clicked()
{
    ZenboNurseHelperProtobuf::ReportAndCommand report_data;
    report_data.set_stopmove(1);
    thread_send_command.AddMessage(report_data);
}

void MainWindow::keyPressEvent(QKeyEvent *event)
{
    QString action;
    int key = event->key();
//    std::cout << key << std::endl;
    bool bEffective = true;
    switch(key)
    {
        case 87:     //w
            action = "w";
            send_move_body_command(0.5, 0, 0, 3);
            break;
        case 65:     //a
            action = "a";
            send_move_body_command(0, 0, 15, 3);
            break;
        case 68:     //d
            action = "d";
            send_move_body_command(0, 0, -15, 3);
            break;
        case 16777235:  //up
            action = "up";
            m_ipitch += 5;
            if( m_ipitch > 55)
                m_ipitch = 55;
            send_move_head_command(m_iyaw, m_ipitch, 3);
            break;
        case 16777237:  //down
            action = "down";
            m_ipitch -= 5;
            if( m_ipitch < -15)
                m_ipitch = -15;
            send_move_head_command(m_iyaw, m_ipitch, 3);
            break;
        case 16777234:  //left
            action = "left";
            m_iyaw += 5;
            if( m_iyaw > 45)
                m_iyaw = 45;
            send_move_head_command(m_iyaw, m_ipitch, 3);
            break;
        case 16777236:  //right
            action = "right";
            m_iyaw -= 5;
            if( m_iyaw < -45)
                m_iyaw = -45;
            send_move_head_command(m_iyaw, m_ipitch, 3);
            break;
        case 16777264:  //F1
            on_pushButton_speak_clicked();
            break;
        case 16777265:  //F2
            action = "voice to text";
            on_pushButton_voice_to_text_clicked();
            break;
        default:
            bEffective = false;
    }
    if(bEffective)
    {
        QString_SentCommands.append(action + "\n");
        ui->plainTextEdit_SentCommands->document()->setPlainText(QString_SentCommands);
        ui->plainTextEdit_SentCommands->verticalScrollBar()->setValue(ui->plainTextEdit_SentCommands->verticalScrollBar()->maximum());
    }
}

void MainWindow::on_checkBox_SaveImages_clicked()
{
    //std::cout << "on_CheckBox_SaveImages_clicked " << std::endl;
    if( ui->checkBox_SaveImages->isChecked() )
    {

        //Check whether the save folder exist
        std::string str_home_path(getenv("HOME"));
        std::string save_to_directory = str_home_path + "/Downloads";
        string raw_images_directory = save_to_directory + "/raw_images";
        //std::cout << "string raw_images_directory " + raw_images_directory << std::endl;
        if( !CheckDirectoryExist(raw_images_directory))
        {
            CreateDirectory(raw_images_directory);
        }
        thread_process_image.raw_images_directory = raw_images_directory;
        thread_process_image.bSaveTransmittedImage = true;
    }
    else
    {
        thread_process_image.raw_images_directory = "";
        thread_process_image.bSaveTransmittedImage = false;
    }

}
