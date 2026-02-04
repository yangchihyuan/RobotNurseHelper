#include "VideoWindow.hpp"
#include <QVideoWidget>
#include <QAudioOutput>
#include <QKeyEvent>
#include <QFileInfo>
#include <QUrl>
#include <iostream>
#include "ThreadStateControl.hpp"

using namespace std;

VideoWindow::VideoWindow(QWidget *parent)
    : QMainWindow(parent)
{
    // Create the video player and widget
    player = new QMediaPlayer(this);
    videoWidget = new QVideoWidget(this);
    audioOutput = new QAudioOutput(this);

    // Set the video widget as the central widget of the window
    setCentralWidget(videoWidget);

    // Set the player output to the video widget
    player->setVideoOutput(videoWidget);
    player->setAudioOutput(audioOutput);

    // Connect the mediaStatusChanged signal to know when the video ends
    connect(player, &QMediaPlayer::mediaStatusChanged, this, &VideoWindow::onMediaStatusChanged);

    // Log any playback errors
    connect(player, &QMediaPlayer::errorOccurred, this, [this](QMediaPlayer::Error error, const QString &errorString){
        cout << "QMediaPlayer error: " << static_cast<int>(error) << " - " << errorString.toStdString() << endl;
    });

    // Set the window size and title
    setWindowTitle("Video Player");
    resize(800, 600);
}

void VideoWindow::playVideo(const QString &fileName)
{
    QFileInfo fi(fileName);
    if (!fi.exists() || !fi.isReadable()) {
        cout << "Error: Video file does not exist or is not readable: " << fileName.toStdString() << endl;
        return;
    }

    // Ensure video widget is visible and on top
    if (videoWidget) {
        videoWidget->show();
        videoWidget->raise();
        videoWidget->update();
    }

    // Load and play the video (use absolute local file URL)
    QString absPath = fi.absoluteFilePath();
    QUrl url = QUrl::fromLocalFile(absPath);
    cout << "Playing video: " << url.toString().toStdString() << endl;
    player->setSource(url);
    player->play();
}

void VideoWindow::onMediaStatusChanged(QMediaPlayer::MediaStatus status)
{
    if (status == QMediaPlayer::EndOfMedia) {
        // This block is executed when the video finishes playing.
        // You can add your code here. For example, to close the window:
        // this->close();

        // I need to notify the ThreadStateControl that the video is finished.
        cout << "Video finished playing." << endl;
        if( pThreadStateControl )
        {
            pThreadStateControl->NotifyEvent("onVideoComplete", chrono::system_clock::now());
        }
    }
}

void VideoWindow::keyPressEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_Escape) {
        showNormal();
    } else {
        QMainWindow::keyPressEvent(event);
    }
}