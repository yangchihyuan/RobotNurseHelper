#ifndef __VIDEO_WINDOW_HPP__
#define __VIDEO_WINDOW_HPP__

#include <QWidget>
#include <QMainWindow>
#include <QMediaPlayer>
#include <QVideoWidget>
#include <QAudioOutput>
#include <QKeyEvent>
#include <QPushButton>
#include <QVBoxLayout>
#include <QFileDialog>

// Forward declaration for the MainWindow class
class MainWindow;

// VideoWindow: The separate window for video playback
class VideoWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit VideoWindow(QWidget *parent = nullptr)
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

        // Set the window size and title
        setWindowTitle("Video Player");
        resize(800, 600);
        //showFullScreen();
    }

    void playVideo(const QString &fileName)
    {
        // Load and play the video
        player->setSource(QUrl::fromLocalFile(fileName));
        player->play();
    }

protected:
    void keyPressEvent(QKeyEvent *event) override
    {
        if (event->key() == Qt::Key_Escape) {
            showNormal();
        } else {
            QMainWindow::keyPressEvent(event);
        }
    }

private:
    QMediaPlayer *player;
    QVideoWidget *videoWidget;
    QAudioOutput *audioOutput;
};

#endif // __VIDEO_WINDOW_HPP__
