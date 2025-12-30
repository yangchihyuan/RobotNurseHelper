#ifndef __VIDEO_WINDOW_HPP__
#define __VIDEO_WINDOW_HPP__

#include <QWidget>
#include <QMainWindow>
#include <QMediaPlayer>
#include <QVideoWidget>
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

        // Set the video widget as the central widget of the window
        setCentralWidget(videoWidget);

        // Set the player output to the video widget
        player->setVideoOutput(videoWidget);

        // Set the window size and title
        setWindowTitle("Video Player");
        resize(800, 600);
    }

    void playVideo(const QString &fileName)
    {
        // Load and play the video
        player->setMedia(QUrl::fromLocalFile(fileName));
        player->play();
    }


private:
    QMediaPlayer *player;
    QVideoWidget *videoWidget;
};

#endif // __VIDEO_WINDOW_HPP__
