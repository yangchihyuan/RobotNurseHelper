#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QDebug>
#include <QFile>
#include <QFileDialog>
#include <QMessageBox>
#include <QMetaType>
#include <QSet>
#include <QStandardPaths>
#include <QTcpServer>
#include <QTcpSocket>
#include "ThreadProcessImage.hpp"
#include "ThreadPortAudio.hpp"
#include "ThreadReceiveMessage.hpp"
#include "ThreadWhisper.hpp"
#include "ThreadLLM.hpp"
#include "ThreadStateControl.hpp"
#include <queue>
#include <QMediaDevices>
#include <QAudioDevice>
#include <QAudioSource>
#include "utility_directory.hpp"
#include "SocketBufferParser.hpp"
#include "SendMessageManager.hpp"
#include "SocketClientHandler.hpp"
#include <memory>
#include "../VideoWindow.hpp"
#include "Setting.hpp"
 
using namespace std;


QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();
    void setLanguage(QString Language);
    void setSettingFile(const QString &filePath);
    void startThreads();

protected:
    void closeEvent(QCloseEvent *event) override;


protected:
    QAudioDevice  devAudio;
    QAudioSource* audioSrc = nullptr;
    bool bListening = false;

private:
    Ui::MainWindow *ui;
    QTimer *timer;

    QTcpServer* m_server_receive_image;
    QSet<QTcpSocket*> connection_set;
    ThreadProcessImage thread_process_image;

    QTcpServer* m_server_send_command;
    QSet<QTcpSocket*> connection_set2;   //for send back command

    QTcpServer* m_server_receive_audio;
    QSet<QTcpSocket*> connection_set3;   //for receive audio
    ThreadProcessAudio thread_process_audio;

    QTcpServer* m_server_receive_message;
    QSet<QTcpSocket*> connection_set4;   //for receive messages
    SocketBufferParser socketHandler4;
    ThreadReceiveMessage thread_receive_message;

    QSet<SocketClientHandler*> Handler_set;

    ThreadWhisper thread_whisper;

    ThreadLLM thread_LLM;
    ThreadStateControl thread_state_control;

    std::unique_ptr<VideoWindow> pVideoWindow;

    QString QString_SentCommands;
    void send_move_body_command(float x, float y, int degree, int speed);
    void send_move_head_command(int yaw, int pitch, int speed);

    SendMessageManager sendMessageManager;
    bool bstream_recognition = true;        //whether to stream the voice recognition result

    WhisperData oldWhisperData;     //for timer_event update UI

    Setting msetting;
    int current_body_angle = 0;
    void rotateAndTakePhoto(int targetAngle, const QString& prefix);
    
signals:
    void newMessage(QString);   //where is the connect for this signal?
    void addSendCommandMessage(RobotCommandProtobuf::RobotCommand);

private slots:
    void onPlayVideoRequested(const QString& videoPath);
    void onPlayImageRequested(const QString& imagePath);
    void newConnection_receive_image();
    void newConnection_send_command();
    void newConnection_receive_audio();
    void newConnection_receive_message();
    void appendToSocketList2(QTcpSocket* socket);
    void appendToSocketList3(QTcpSocket* socket);

    void readSocket3();

    void discardSocket2();
    void discardSocket3();

    void displayError(QAbstractSocket::SocketError socketError);


    void on_pushButton_speak_clicked();
    void on_pushButton_movebody_clicked();
    void on_pushButton_movehead_clicked();
    void on_pushButton_stop_action_clicked();
    void on_pushButton_voice_to_text_clicked();
    void on_pushButton_killapp_clicked();
    void on_pushButton_onTTSComplete_clicked();

    void on_listView_FacialExpressions_doubleClicked(const QModelIndex &index);
    void on_listView_PredefinedAction_doubleClicked(const QModelIndex &index);
    void on_listView_Content_doubleClicked(const QModelIndex &index);
    void on_listView_Sentence1_doubleClicked(const QModelIndex &index);
    void on_listView_Sentence1_clicked(const QModelIndex &index);
    void on_listView_Sentence2_doubleClicked(const QModelIndex &index);
    void on_listView_Sentence2_clicked(const QModelIndex &index);
    void on_listView_Sentence3_doubleClicked(const QModelIndex &index);
    void on_listView_Sentence3_clicked(const QModelIndex &index);

    void on_checkBox_SaveImages_clicked();
    void on_checkBox_UseVisualCompass_clicked();

    void timer_event();

    void comboBox_MoveMode_changed();
    void comboBox_DetectionMode_changed();
    void comboBox_Language_changed();

    void keyPressEvent(QKeyEvent *event);
    void on_checkBox_stream_clicked(bool checked);
    void on_pushButton_generate_response_clicked();
    void on_pushButton_speak_2_clicked();
    void on_pushButton_hideface_clicked();
    void on_pushButton_photo_0_clicked();
    void on_pushButton_photo_45_clicked();
    void on_pushButton_photo_90_clicked();
    void on_pushButton_photo_135_clicked();
    void on_pushButton_photo_180_clicked();
    void on_pushButton_photo_225_clicked();
    void on_pushButton_photo_270_clicked();
    void on_pushButton_photo_315_clicked();
};
#endif // MAINWINDOW_H
