#ifndef __VIDEO_WINDOW_HPP__
#define __VIDEO_WINDOW_HPP__

#include <QMainWindow>
#include <QMediaPlayer>

class QVideoWidget;
class QAudioOutput;
class QKeyEvent;
class ThreadStateControl;

class VideoWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit VideoWindow(QWidget *parent = nullptr);
    void playVideo(const QString &fileName);
    ThreadStateControl* pThreadStateControl = nullptr;

public slots:
    void onMediaStatusChanged(QMediaPlayer::MediaStatus status);

protected:
    void keyPressEvent(QKeyEvent *event) override;

private:
    QMediaPlayer *player;
    QVideoWidget *videoWidget;
    QAudioOutput *audioOutput;
};

#endif // __VIDEO_WINDOW_HPP__