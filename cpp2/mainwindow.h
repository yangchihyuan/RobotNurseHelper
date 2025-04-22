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
#include "ThreadProcessAudio.hpp"
#include "ThreadTablet.hpp"
#include "ThreadWhisper.hpp"
#include <queue>
#include <QMediaDevices>
#include <QAudioDevice>
#include <QAudioSource>
#include "utility_directory.hpp"
#include "SocketHandler.hpp"
 
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
    void setWhisperModelFile( QString filePath);
    void startThreads();

protected:
    QAudioDevice  devAudio;
    QAudioSource* audioSrc = nullptr;
    bool bListening = false;

private:
    Ui::MainWindow *ui;

    QTcpServer* m_server_receive_image;
    QSet<QTcpSocket*> connection_set;
    ThreadProcessImage thread_process_image;
    SocketHandler socketHandler1;

    QTcpServer* m_server_send_command;
    QSet<QTcpSocket*> connection_set2;   //for send back command
    ThreadSendCommand thread_send_command;

    QTcpServer* m_server_receive_audio;
    QSet<QTcpSocket*> connection_set3;   //for receive audio
    ThreadProcessAudio thread_process_audio;

    QTcpServer* m_server_Tablet;
    QSet<QTcpSocket*> connection_set4;   //for Tablet
    SocketHandler socketHandler4;
    ThreadTablet thread_tablet;

    ThreadWhisper thread_whisper;

    QString QString_SentCommands;
    void send_move_body_command(float x, float y, int degree, int speed);
    void send_move_head_command(int yaw, int pitch, int speed);

signals:
    void newMessage(QString);   //where is the connect for this signal?
    void addSendCommandMessage(ZenboNurseHelperProtobuf::ReportAndCommand);

private slots:
    void newConnection();
    void appendToSocketList(QTcpSocket* socket);
    void appendToSocketList2(QTcpSocket* socket);
    void appendToSocketList3(QTcpSocket* socket);
    void appendToSocketList4(QTcpSocket* socket);

    void readSocket();
    void readSocket3();
    void readSocket4();

    void discardSocket();
    void discardSocket2();
    void discardSocket3();
    void discardSocket4();
    void displayError(QAbstractSocket::SocketError socketError);

    void newConnection_send_command();
    void newConnection_receive_audio();
    void newConnection_Tablet();

    void on_pushButton_speak_clicked();
    void on_pushButton_movebody_clicked();
    void on_pushButton_movehead_clicked();
    void on_pushButton_stop_action_clicked();
    void on_pushButton_voice_to_text_clicked();

    void on_listView_FacialExpressions_doubleClicked(const QModelIndex &index);
    void on_listView_PredefinedAction_doubleClicked(const QModelIndex &index);
    void on_listView_Sentence1_doubleClicked(const QModelIndex &index);
    void on_listView_Sentence1_clicked(const QModelIndex &index);
    void on_listView_Sentence2_doubleClicked(const QModelIndex &index);
    void on_listView_Sentence2_clicked(const QModelIndex &index);
    void on_listView_Sentence3_doubleClicked(const QModelIndex &index);
    void on_listView_Sentence3_clicked(const QModelIndex &index);

    void on_checkBox_SaveImages_clicked();

    void timer_event();

    void comboBox_MoveMode_changed();
    void comboBox_DetectionMode_changed();
    void comboBox_Processor_changed();

    void keyPressEvent(QKeyEvent *event);
};
#endif // MAINWINDOW_H
