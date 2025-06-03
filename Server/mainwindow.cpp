#include "mainwindow.h"
#include <QPixmap>
#include <QStringListModel>
#include <QStandardItemModel>
#include <QModelIndex>
#include <QAbstractItemView>
#include <iostream>
#ifdef USE_KEBBI
    #include "Kebbi/RobotCommand.pb.h"
    #include "Kebbi/ui_mainwindow.h"
#elif USE_ZENBO
    #include "Zenbo/RobotCommand.pb.h"
    #include "Zenbo/ui_mainwindow.h"
#endif
#include <QTimer>
#include <opencv2/core.hpp>
#include <opencv2/highgui.hpp>
#include <QScrollBar>
#include "RobotStatus.hpp"
#include "ActionOption.hpp"
#include "ThreadOllama.hpp"
#include "LandmarkToRobotAction.hpp" //[MOHAMED]

extern std::mutex gMutex_audio_buffer;
extern std::queue<short> AudioBuffer;
extern std::condition_variable cond_var_audio;
extern cv::Mat outFrame;
extern int PortAudio_stop_and_terminate();
extern bool gbPlayAudio;
extern RobotStatus robot_status;
extern ActionOption action_option;

void MainWindow::startThreads()
{
    //run threads
    thread_process_image.start();
    thread_process_audio.start();
    thread_tablet.start();
    thread_whisper.start();
    thread_ollama.start();
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

    foreach (QTcpSocket* socket, connection_set2)
    {
        socket->close();
        socket->deleteLater();
    }

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

    thread_tablet.b_WhileLoop = false;
    thread_tablet.cond_var_tablet.notify_one();
    thread_tablet.wait();
    foreach (QTcpSocket* socket, connection_set4)
    {
        socket->close();
        socket->deleteLater();
    }
    m_server_Tablet->close();
    m_server_Tablet->deleteLater();
  
    thread_whisper.b_WhileLoop = false;
    thread_whisper.wait();
    if (audioSrc != nullptr)
      delete audioSrc;

    thread_ollama.b_WhileLoop = false;
    thread_ollama.cond_var_ollama.notify_one();
    thread_ollama.wait();

    delete ui;
}

void MainWindow::setWhisperModelFile( QString filePath)
{
    thread_whisper.model_file_path = filePath;
}

void MainWindow::setLanguageModelName( QString ModelName)
{
    thread_ollama.ModelName = ModelName.toStdString();
}

void MainWindow::setImageSaveDirectory( QString ImageSaveDirectory)
{
    thread_process_image.ImageSaveDirectory = ImageSaveDirectory.toStdString();
    if( !CheckDirectoryExist(thread_process_image.ImageSaveDirectory))
    {
        CreateDirectory(thread_process_image.ImageSaveDirectory);
    }
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

void MainWindow::setLanguage( QString Language)
{
    QString SentenceFileName;
    if( Language == "Chinese")
    {
        thread_ollama.str_system_message = "你是一個醫療用機器人，名字叫作Zenbo，回答要很潔短, 而且要用台灣人習慣的繁體中文回答。";
        thread_whisper.strLanguage = "zh"; // set language to Chinese
        SentenceFileName = "Sentence_Chinese.txt";
    }
    else if( Language == "English")
    {
        thread_ollama.str_system_message = "You are a medical robot named Zenbo. Please answer in concise English.";
        // "There is also visual input, however you (the LLM) will not have direct access to it. In addition to recieving text prompts from the patient, you may receive a short sentence that indicates the body language by the patient. This body language should affect your output"
        thread_whisper.strLanguage = "en"; // set language to English
        SentenceFileName = "Sentence_English.txt";
    }
    else if( Language == "Arabic")
    {
        thread_ollama.str_system_message = "أنت روبوت طبي يُدعى زينبو. يرجى الإجابة باللغة العربية المختصرة.";
        thread_whisper.strLanguage = "ar"; // set language to Arabic
        SentenceFileName = "Sentence_English.txt";
    }
    else
    {
        throw "Unsupported language: " + Language.toStdString();    
    }

    QFile textFile(SentenceFileName);
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
    else
    {
        throw "Cannot open sentence file: " + SentenceFileName.toStdString();
    }

}

void MainWindow::setImageSaveEveryNFrame(int N)
{
    thread_process_image.image_save_every_N_frame = N;
    thread_process_image.bSaveTransmittedImage = (N > 0);
    if( N > 0)
    {
        ui->checkBox_SaveImages->setChecked(true);
    }
}



//This funciton is called when socket is connected.
void MainWindow::newConnection()
{
    std::cout << "newConnction() 8895" << std::endl;
    //Because of the loop, it always waits for new connections.
    while (m_server_receive_image->hasPendingConnections())
        appendToSocketList(m_server_receive_image->nextPendingConnection());
}

void MainWindow::newConnection_send_command()
{
    std::cout << "newConnction() 8896" << std::endl;
    while (m_server_send_command->hasPendingConnections())
        appendToSocketList2(m_server_send_command->nextPendingConnection());    //the nextPendingConnection() will retrieve a socket
}

void MainWindow::newConnection_receive_audio()
{
    std::cout << "newConnction() 8897" << std::endl;
    while (m_server_receive_audio->hasPendingConnections())
        appendToSocketList3(m_server_receive_audio->nextPendingConnection());
}

void MainWindow::newConnection_Tablet()
{
    std::cout << "newConnction() 8898" << std::endl;
    while (m_server_Tablet->hasPendingConnections())
        appendToSocketList4(m_server_Tablet->nextPendingConnection());
}

//Define the behavior of a socket.
void MainWindow::appendToSocketList(QTcpSocket* socket)
{
    connection_set.insert(socket);
    connect(socket, &QTcpSocket::readyRead, this, &MainWindow::readSocket);
    connect(socket, &QTcpSocket::disconnected, this, &MainWindow::discardSocket);
    connect(socket, &QAbstractSocket::errorOccurred, this, &MainWindow::displayError);
}

//There is no readSocket because I only use this connection to send commands.
void MainWindow::appendToSocketList2(QTcpSocket* socket)
{
    connection_set2.insert(socket);
    sendMessageManager.pSocket = socket;
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

void MainWindow::appendToSocketList4(QTcpSocket* socket)
{
    connection_set4.insert(socket);
    connect(socket, &QTcpSocket::readyRead, this, &MainWindow::readSocket4);
    connect(socket, &QTcpSocket::disconnected, this, &MainWindow::discardSocket4);
    connect(socket, &QAbstractSocket::errorOccurred, this, &MainWindow::displayError);
}

void MainWindow::readSocket()
{
    //sender() is a function of Qt to get the data source of this function.
    QTcpSocket* socket = reinterpret_cast<QTcpSocket*>(sender());

    QDataStream socketStream(socket);
    qint64 byteAvailable = socket->bytesAvailable();


    unique_ptr<char[]> pReadData = std::make_unique<char[]>(byteAvailable);
    qint64 readlength = socketStream.readRawData(pReadData.get(), byteAvailable);
    socketHandler1.add_data(pReadData.get(), byteAvailable);
    //If there is no image in the queue, I don't want the thread run.
    thread_process_image.cond_var_process_image.notify_one();
}

void MainWindow::readSocket3()
{
    QTcpSocket* socket = reinterpret_cast<QTcpSocket*>(sender());

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
        AudioBuffer.push(value);       //This AudioBuffer is used to play audio on the server
    }
    gMutex_audio_buffer.unlock();

    if( bstream_recognition)
    {
        thread_whisper.mtx_whisper_buffer.lock();
        for( long long i = 0; i<length/2 ; i++)
        {
            short value = *(pShort + i);
            thread_whisper.pcmf32_new[i+thread_whisper.bufferlength] = ((float)value / 32768.0f);
        }
        thread_whisper.bufferlength += length/2;
        thread_whisper.mtx_whisper_buffer.unlock();
//        std::cout << "thread_whisper.pcmf32_queue size: " << thread_whisper.pcmf32_queue.size() << std::endl;  
    }


    if( AudioBuffer.size() >= 1024)
        cond_var_audio.notify_one();

    if(!socketStream.commitTransaction())
    {
        QString message = QString("%1 :: Waiting for more data to come..").arg(socket->socketDescriptor());
        emit newMessage(message);
        return;
    }

}

void MainWindow::readSocket4()
{
    QTcpSocket* socket = reinterpret_cast<QTcpSocket*>(sender());

    QDataStream socketStream(socket);
    qint64 byteAvailable = socket->bytesAvailable();

    unique_ptr<char[]> pReadData = std::make_unique<char[]>(byteAvailable);
    qint64 readlength = socketStream.readRawData(pReadData.get(), byteAvailable);
    socketHandler4.add_data(pReadData.get(), byteAvailable);
    thread_tablet.cond_var_tablet.notify_one();
}

void MainWindow::discardSocket()
{
    QTcpSocket* socket = reinterpret_cast<QTcpSocket*>(sender());
    QSet<QTcpSocket*>::iterator it = connection_set.find(socket);
    if (it != connection_set.end()){
        cout << "INFO :: A client has just left the room 8895" << endl;
        connection_set.remove(*it);
    }
    
    socket->deleteLater();
}

void MainWindow::discardSocket2()
{
    QTcpSocket* socket = reinterpret_cast<QTcpSocket*>(sender());
    QSet<QTcpSocket*>::iterator it = connection_set2.find(socket);
    if (it != connection_set2.end()){
        cout << "INFO :: A client has just left the room 8896" << endl;
        connection_set2.remove(*it);
    }
    socket->deleteLater();
}

void MainWindow::discardSocket3()
{
    QTcpSocket* socket = reinterpret_cast<QTcpSocket*>(sender());
    QSet<QTcpSocket*>::iterator it = connection_set3.find(socket);
    if (it != connection_set3.end()){
        cout << "INFO :: A client has just left the room 8897" << endl;
        connection_set3.remove(*it);
    }
    
    socket->deleteLater();
}

void MainWindow::discardSocket4()
{
    QTcpSocket* socket = reinterpret_cast<QTcpSocket*>(sender());
    QSet<QTcpSocket*>::iterator it = connection_set4.find(socket);
    if (it != connection_set4.end()){
        cout << "INFO :: A client has just left the room 8898" << endl;
        connection_set4.remove(*it);
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

void MainWindow::on_pushButton_voice_to_text_clicked()
{
    if( !bListening)
    {
        bListening = true;
        if( thread_whisper.pOperatorBuffer != NULL)
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
    send_move_head_command(robot_status.yaw_degree, robot_status.pitch_degree, headspeed.toInt());
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

void MainWindow::on_listView_FacialExpressions_doubleClicked(const QModelIndex &index)
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
        string body_language_added_prompt = "";
        if (!global_landmarks.empty()) //[MOHAMED]
        {
            if (global_landmarks[0][20][2] > global_landmarks[0][12][2] && global_landmarks[0][18][2] > global_landmarks[0][12][2])
            {
                //Check is right pinky and right index y nomralized coordinate is higher than the right shoulder y coordinate, symbolizing raised right hand
                body_language_added_prompt = "[Body Language from Visual Input]: Patients right hand is raised";
            }
        }
        thread_whisper.b_new_RobotSentence = false;
        ui->plainTextEdit_received->setPlainText(QString::fromStdString(thread_whisper.strRobotSentence + body_language_added_prompt));
    }

    if( thread_whisper.b_RobotSentence_End )
    {
        thread_whisper.b_RobotSentence_End = false;
        //send a command as the push button clicked
        ui->pushButton_generate_response->click();
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

void MainWindow::comboBox_MoveMode_changed()
{
    switch(ui->comboBox_MoveMode->currentIndex())
    {
        case 0:
            action_option.move_mode = ActionOption::MOVE_MANUAL;
            break;
        case 1:     //move body
            action_option.move_mode = ActionOption::MOVE_BODY;
            break;
        case 2:     //move head
            action_option.move_mode = ActionOption::MOVE_HEAD;
            break;
    }
}

void MainWindow::comboBox_DetectionMode_changed()
{
    switch(ui->comboBox_DetectionMode->currentIndex())
    {
        case 0:
            thread_process_image.b_HumanPoseEstimation = false;
            thread_process_image.setTask("None");
            break;
        case 1:     //Face
            //change the Processor mode to CPU, because the face detection model is not supported by GPU
            ui->comboBox_Processor->setCurrentIndex(0);
            thread_process_image.setProcessor("CPU");
            thread_process_image.setTask("Face");
            thread_process_image.b_HumanPoseEstimation = true;
            break;
        case 2:     //Pose
            thread_process_image.setTask("Pose");
            thread_process_image.b_HumanPoseEstimation = true;
            break;
        case 3:     //Holistic
            thread_process_image.setTask("Holistic");
            thread_process_image.b_HumanPoseEstimation = true;
            break;
    }
}

void MainWindow::comboBox_Processor_changed()
{
    switch(ui->comboBox_Processor->currentIndex())
    {
        case 0:     //CPU
            thread_process_image.setProcessor("CPU");
            break;
        case 1:     //GPU
            if( ui->comboBox_DetectionMode->currentIndex() == 1 )
            {
                QMessageBox::warning(this, "Warning", "The Face detection model is not supported by GPU. Please select CPU.");
                ui->comboBox_Processor->setCurrentIndex(0);
                return;
            }
            thread_process_image.setProcessor("GPU");
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
            robot_status.pitch_degree += 5;
            if( robot_status.pitch_degree > 55)
                robot_status.pitch_degree = 55;
            send_move_head_command(robot_status.yaw_degree, robot_status.pitch_degree, 3);
            break;
        case 16777237:  //down
            action = "down";
            robot_status.pitch_degree -= 5;
            if( robot_status.pitch_degree < -15)
                robot_status.pitch_degree = -15;
            send_move_head_command(robot_status.yaw_degree, robot_status.pitch_degree, 3);
            break;
        case 16777234:  //left
            action = "left";
            robot_status.yaw_degree += 5;
            if( robot_status.yaw_degree > 45)
                robot_status.yaw_degree = 45;
            send_move_head_command(robot_status.yaw_degree, robot_status.pitch_degree, 3);
            break;
        case 16777236:  //right
            action = "right";
            robot_status.yaw_degree -= 5;
            if( robot_status.yaw_degree < -45)
                robot_status.yaw_degree = -45;
            send_move_head_command(robot_status.yaw_degree, robot_status.pitch_degree, 3);
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
        thread_process_image.bSaveTransmittedImage = true;
    }
    else
    {
        thread_process_image.bSaveTransmittedImage = false;
    }

}

void MainWindow::on_checkBox_stream_clicked(bool checked)
{
    if( checked)
    {
        thread_whisper.strFixed = "";
        thread_whisper.setStartTime();
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
    thread_ollama.strPrompt = text.toStdString();
    thread_ollama.cond_var_ollama.notify_one();   
}


