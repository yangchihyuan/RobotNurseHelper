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
#include "ProcessImageThread.hpp"
#include <queue>
#include "ProcessAudioThread.hpp"
#include <QMediaDevices>
#include <QAudioDevice>
#include <QAudioSource>
#include <QBuffer>
 
#include <whisper.h>


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

protected:
  QAudioDevice  devAudio;
  QAudioSource* audioSrc = nullptr;
  QBuffer       buffer;
  bool bListening = false;
  whisper_context* ctx = nullptr;


private:
    Ui::MainWindow *ui;

    QTcpServer* m_server_receive_image;
    QSet<QTcpSocket*> connection_set;
    ProcessImageThread thread_process_image;
    std::unique_ptr<char[]> frame_buffer;
    int buffer_length;
    int iEndOfAFrame;

    QTcpServer* m_server_send_command;
    QSet<QTcpSocket*> connection_set2;   //for send back command
    SendCommandThread thread_send_command;

    QTcpServer* m_server_receive_audio;
    QSet<QTcpSocket*> connection_set3;   //for receive audio
    ProcessAudioThread thread_process_audio;

    QString QString_SentCommands;
    void send_move_body_command(float x, float y, int degree, int speed);
    int m_iyaw = 0;
    int m_ipitch = 30;
    void send_move_head_command(int yaw, int pitch, int speed);

signals:
    void newMessage(QString);   //where is the connect for this signal?
    void addSendCommandMessage(ZenboNurseHelperProtobuf::ReportAndCommand);

private slots:
    void newConnection();
    void appendToSocketList(QTcpSocket* socket);
    void appendToSocketList2(QTcpSocket* socket);
    void appendToSocketList3(QTcpSocket* socket);

    void readSocket();
    void readSocket3();

    void discardSocket();
    void discardSocket2();
    void discardSocket3();
    void displayError(QAbstractSocket::SocketError socketError);

    void displayMessage(const QString& str);
 
    void newConnection_send_command();
    void newConnection_receive_audio();

    void on_pushButton_speak_clicked();
    void on_pushButton_movebody_clicked();
    void on_pushButton_movehead_clicked();
    void on_pushButton_stop_action_clicked();
    void on_pushButton_voice_to_text_clicked();

    void on_listView_FacialExpressions_doubleClicked(const QModelIndex &index);
    void on_listView_PredefinedAction_doubleClicked(const QModelIndex &index);
    void on_listView_Sentence1_doubleClicked(const QModelIndex &index);
    void on_listView_Sentence1_clicked(const QModelIndex &index);

    void timer_event();

    void comboBox_MoveMode_changed();

    void keyPressEvent(QKeyEvent *event);
};
#endif // MAINWINDOW_H
