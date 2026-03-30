#include "VideoWindow.hpp"
#include <QVideoWidget>
#include <QAudioOutput>
#include <QLabel>
#include <QStackedWidget>
#include <QPixmap>
#include <QKeyEvent>
#include <QFileInfo>
#include <QUrl>
#include <iostream>
#include "ThreadStateControl.hpp"

using namespace std;

VideoWindow::VideoWindow(QWidget *parent)
    : QMainWindow(parent)
{
    // Create the widgets
    player = new QMediaPlayer(this);
    videoWidget = new QVideoWidget(this);
    audioOutput = new QAudioOutput(this);
    imageLabel = new QLabel(this);
    stackedWidget = new QStackedWidget(this);

    // Configure the image label to scale its contents
    imageLabel->setAlignment(Qt::AlignCenter);
    imageLabel->setScaledContents(true);

    // Add widgets to the stacked layout
    stackedWidget->addWidget(videoWidget);
    stackedWidget->addWidget(imageLabel);

    setCentralWidget(stackedWidget);
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

    // Switch to the video widget before playing
    stackedWidget->setCurrentWidget(videoWidget);
    // Load and play the video (use absolute local file URL)
    QString absPath = fi.absoluteFilePath();
    QUrl url = QUrl::fromLocalFile(absPath);
    cout << "Playing video: " << url.toString().toStdString() << endl;
    player->setSource(url);
    player->play();
}

void VideoWindow::showImage(const QString &fileName)
{
    QFileInfo fi(fileName);
    if (!fi.exists() || !fi.isReadable()) {
        cout << "Error: Image file does not exist or is not readable: " << fileName.toStdString() << endl;
        return;
    }

    QPixmap pixmap(fileName);
    if (pixmap.isNull()) {
        cout << "Error: Failed to load image: " << fileName.toStdString() << endl;
        return;
    }

    // Stop any video that might be playing
    if (player->playbackState() == QMediaPlayer::PlayingState) {
        player->stop();
    }
    imageLabel->setPixmap(pixmap);
    stackedWidget->setCurrentWidget(imageLabel);
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