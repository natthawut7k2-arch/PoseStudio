#ifndef MOTION_PLAYBACK_BAR_H
#define MOTION_PLAYBACK_BAR_H

#include <QString>
#include <QWidget>

class QPushButton;
class QSlider;
class QLabel;

namespace pose {

class MotionPlaybackBar : public QWidget {
    Q_OBJECT

public:
    explicit MotionPlaybackBar(QWidget* parent = nullptr);
    ~MotionPlaybackBar() override = default;

    void setPlaybackTime(float currentSeconds, float totalSeconds, bool isPlaying);
    void setCameraFollowState(bool follow);
    void setLoopState(bool loop);

signals:
    void playRequested();
    void pauseRequested();
    void stopRequested();
    void seekRequested(float seconds);
    void cameraFollowToggled(bool enabled);
    void loopToggled(bool enabled);

private slots:
    void onPlayClicked();
    void onStopClicked();
    void onSliderValueChanged(int value);
    void onSliderPressed();
    void onSliderReleased();
    void onCameraFollowClicked();
    void onLoopClicked();

private:
    static QString formatTime(float seconds);

    QPushButton* m_playButton = nullptr;
    QPushButton* m_stopButton = nullptr;
    QSlider*     m_slider = nullptr;
    QLabel*      m_timeLabel = nullptr;
    QPushButton* m_cameraFollowButton = nullptr;
    QPushButton* m_loopButton = nullptr;

    float m_totalSeconds = 10.0f;
    float m_currentSeconds = 0.0f;
    bool  m_isPlaying = false;
    bool  m_isUserScrubbing = false;
};

} // namespace pose

#endif // MOTION_PLAYBACK_BAR_H
