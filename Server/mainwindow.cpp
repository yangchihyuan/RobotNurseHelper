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

//#include "cppvader/include/cppvader.hpp" //[MOHAMED]

extern std::mutex gMutex_audio_buffer;
extern std::queue<short> AudioBuffer;
extern std::condition_variable cond_var_audio;
extern cv::Mat outFrame;
extern int PortAudio_stop_and_terminate();
extern bool gbPlayAudio;
extern RobotStatus robot_status;
extern ActionOption action_option;

int question_counter = 0;
int motion_counter = 0;
//extern cv::Mat outFrame; // [MOHAMED]

void MainWindow::startThreads()
{
    //run threads
    thread_process_image.start();
    thread_process_audio.start();
    thread_receive_messages.start();
    thread_whisper.start();
    thread_ollama.start();
    thread_state_control.start();
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

    thread_receive_messages.b_WhileLoop = false;
    thread_receive_messages.cond_var_receive_messages.notify_one();
    thread_receive_messages.wait();
    
    foreach (QTcpSocket* socket, connection_set4)
    {
        socket->close();
        socket->deleteLater();
    }
    m_server_receive_messages->close();
    m_server_receive_messages->deleteLater();
  
    thread_whisper.b_WhileLoop = false;
    thread_whisper.wait();
    if (audioSrc != nullptr)
      delete audioSrc;

    thread_ollama.b_WhileLoop = false;
    thread_ollama.cond_var_ollama.notify_one();
    thread_ollama.wait();

    thread_state_control.b_WhileLoop = false;
    thread_state_control.cond_var_state_control.notify_one();
    thread_state_control.wait();

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


void MainWindow::setStage(int N)
{
    //thread_ollama.start_stage_input = N;
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

void MainWindow::setLanguage( QString Language)
{
    QString SentenceFileName;
    if( Language == "Chinese")
    {
       
        thread_ollama.str_system_message_list[0] = R"(你是一台名叫凱比的醫療機器人，正在和一位年幼的小朋友病患聊天。請遵守以下規則：
        1. 回答要用非常簡單、親切的中文，不能使用其他語言。
        2. 一開始請輕鬆地問一些有趣的問題來暖場，例如：你最喜歡的顏色是什麼？你最喜歡哪種動物？你喜歡上什麼課？你現在是幾年級呢？
        3. 請不要重複或輸出你已經收到的資訊。
        4. 請不要輸出任何表情符號。
        5. 請不要輸出任何括號。
        )";

        thread_ollama.str_system_message_list[1] = R"(你是一台名叫凱比的醫療機器人，正在與一位兒童病患交談。請遵守以下規則：
        1. 回答必須使用非常簡潔的中文，不能使用其他語言。
        2. 所有數字必須使用對應的繁體中文字表示，例如「一」、「二」、「三」，不可使用阿拉伯數字。
        3. 請不要輸出任何表情符號。
        4. 請不要輸出任何括號。
        )";

        thread_ollama.str_system_message_list[5] = R"(你是一台名叫凱比的醫療機器人，正在與一位年幼的兒童病患交談。請遵守以下規則：
        1. 請避免提到自己。
        2. 詢問小朋友是否想讓你跳「埃及舞」或「牛仔舞」。
        3. 請不要輸出任何表情符號。
        4. 請不要輸出任何括號。
        )";

/*
        thread_ollama.str_system_message_list[6] = R"(你是一台名叫凱比的醫療機器人，正在與一位年幼的兒童病患交談。請遵守以下規則：

        1. 不要重複同一個問題兩次。
        2. 請使用非常簡潔且友善的語氣回答。
        3. 每次輸出只能包含一句或兩句簡短的句子。
        4. 請告訴小朋友一些有趣的謎語（如果他們答錯，可以提示後再給一次機會），並回答他們的問題。
        5. 請不要輸出任何表情符號
        6. 請不要輸出任何括號
        )";
        thread_ollama.str_system_message_list[6] = R"(你是一台名叫凱比的醫療機器人，正在與一位年幼的兒童病患交談。

        病患目前正在觀看一段健康教育影片，請你不要說話或輸出任何內容。)";
*/        

        thread_ollama.str_system_message_list[6] = R"(你是一台名叫凱比的醫療機器人，正在與一位年幼的兒童病患交談。請遵守以下規則：

        1. 不要重複同樣的問題。
        2. 回答時請使用非常簡潔且友善的語氣。
        3. 每次回答只能包含一句或兩句簡短的句子。
        4. 和小朋友玩一個猜動物的遊戲：給出關於一種動物的簡短提示，讓小朋友猜。
        5. 如果小朋友猜錯，請提供一個友善的提示，讓他們再試一次。
        6. 如果小朋友提問，請回答他們的問題。
        7. 不要輸出任何表情符號。
        8. 不要輸出任何括號。
        )";


        thread_ollama.str_system_message_list[7] = R"(你是一台名叫凱比的醫療機器人，正在與一位年幼的兒童病患交談。請遵守以下規則：
        1. 請說一個簡短有趣的故事逗病患開心。
        2. 接著請詢問小朋友是否對這個故事有任何問題想問。
        3. 不要輸出任何表情符號。
        4. 不要輸出任何括號。
        )";

        thread_ollama.str_system_message_list[8] = R"(你是一台名叫凱比的醫療機器人，正在與一位年幼的兒童病患交談。請遵守以下規則：
        1. 你要跟兒童病患道別了。
        2. 你要說很多好話祝他早日康復，重新快快樂樂的過生活。
        3. 不要輸出任何表情符號。
        4. 不要輸出任何括號。
        5. 不要提問任何問題。
        )";
        
        
        thread_whisper.strLanguage = "zh"; // set language to Chinese (可維持此行不變)
        SentenceFileName = "Sentence_Chinese.txt";

        //2025/8/13 I no longer use this prompt.
/*
        thread_ollama.bio_summary_prompt = R"(請總結目前收集到的關於病患的重要資訊。格式如下（僅為範例）：
        **病患摘要：**
        - 年齡：8
        - 姓名：楊智淵
        - 主要症狀：胃痛
        - 部位：胃部
        - 疼痛強度與其他問題：感覺胃在喉嚨裡。)";
*/
        thread_ollama.check_stage_prompt = "是否已完整收集病患的年齡、姓名、疼痛強度（或等級）以及症狀／主要主訴資訊？這對於判斷是否繼續提問非常重要。請回答是或否。如果是否，請說明缺失的資訊。";
        thread_ollama.no_response = "病患沒有回應。請繼續你正在說的內容。";
        thread_ollama.dance_complete = "病人選擇的舞蹈已經完成";
    }
    else if( Language == "English")
    {
        thread_ollama.str_system_message_list[0] = 
            "You are a medical robot named Zenbo. You are talking to a child patient. Please answer in CONCISE English. DO NOT OUTPUT INFORMATION YOU HAVE RECEIVED. In addition to recieving text prompts from the patient, you may receive a short sentence that indicates the body language by the patient. Start off, by ONE BY ONE asking the child a few fun questions about their personality, like favourite colour, school subject, ect to break the ice. The robot can move with set actions, but this is handled completely seperately"; // Once information is gathered, do not restate the questions";
        thread_ollama.str_system_message_list[1] = 
            "You are a medical robot named Zenbo. You are talking to a child patient. Please answer in CONCISE English. DO NOT OUTPUT INFORMATION YOU HAVE RECEIVED. In addition to recieving text prompts from the patient, you may receive a short sentence that indicates the body language by the patient. We will need you to issue a series of prompts for data gathering purposes, first to ask one by one for age, name, patient's symptoms, and how they are feeling on a scale from 1 to 5. The robot can move with set actions, but this is handled completely seperately"; // Once information is gathered, do not restate the questions";
        thread_ollama.str_system_message_list[2] = 
            "You are a medical robot named Zenbo. You are talking to a young child patient. Please answer in concise English without mentioning yourself. Ask if child wants robot to do Egypt Dance or Dancing Cowboy";
        thread_ollama.str_system_message_list[3] = 
            "You are a medical robot named Zenbo. You are talking to a young child patient. Do not repeat the same question twice. Please answer in very concise and friendly English. Output only one or two short sentences at a time. Please tell tell the child a few riddles (and give them a second chance with a hint if they get it wrong) and answer their questions if they have any.";
        thread_ollama.str_system_message_list[4] = 
            "You are a medical robot named Zenbo. You are talking to a young child patient. The child is being shown a short health educational video, you do not need to say anything.";
        thread_ollama.str_system_message_list[5] = 
            "You are a medical robot named Zenbo. You are talking to a young child patient. Do not repeat the same question twice. Please answer in very concise and friendly English. Output only one or two short sentences at a time. Play an animal guessing game with the child: give short clues about an animal and let the child guess. If they guess wrong, offer a friendly hint and let them try again. Answer any questions the child may have.";
        thread_ollama.str_system_message_list[6] = 
            "You are a medical robot named Zenbo. You are talking to a young child patient. Tell the child a short funny story, then ask if the child has any questions about the story";
        
        // thread_ollama.str_system_message_list[0] += prompt;
        // We will need you to issue a series of prompts for data gathering purposes, first to ask one by one for age, name, patient's symptoms, and pain intensity. Once information is gathered, do not restate the questions";
        // A raised right hand means that the patient would like to ask a question. We will need you to issue a series of prompts for data gathering purposes, first to ask for age, name and how the patient is feeling. Once information is gathered, do not restate the questions"; 
        //"Only respond if the patient is looking towards you. Do not respond if the patient is NOT looking towards you.";
        thread_whisper.strLanguage = "en"; // set language to English
        SentenceFileName = "Sentence_English.txt";

        thread_ollama.bio_summary_prompt = R"(Summarize only the important information gathered about patient so far. In this format (only as an example):
        **Patient Summary:**
        
        -Age: 35
        -Name: Muhammad
        -Main Complaint: Stomach ache.
        -Location: Stomach.
        -Pain Intensity: Additional Concern:** Feels stomach in throat.)";

        thread_ollama.check_stage_prompt = "Has ALL the patient age, name, how they are feeling on a scale from 1 to 5, and symptom/main complaint information been gathered? Do not concern yourself with any other information and do not ask for clarifications. As soon as the minimum specified info has been gathered, say yes. State yes or no. If no, state what is missing.";
        thread_ollama.no_response = "No response from patient. Continue with what you are saying";
        thread_ollama.dance_complete = "The dance that the patient selected is now complete";
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
    thread_state_control.cond_var_state_control.notify_one();
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
    while (m_server_receive_messages->hasPendingConnections())
        appendToSocketList4(m_server_receive_messages->nextPendingConnection());
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
    //cout << "HELLO\n";
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
    thread_receive_messages.cond_var_receive_messages.notify_one();
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
        case 4:     //Hand
            thread_process_image.setTask("Hand");
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
//        thread_whisper.setStartTime();
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
    thread_ollama.strPrompt = text.toStdString();      //The string is used here.
//    thread_ollama.cond_var_ollama.notify_one();     //2025/8/7 This is the only place where we notify the thread_ollama to generate a response. Did Mohamed call this function?
}


