#include "motionplaybackbar.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QSlider>
#include <QStyle>

#include <algorithm>
#include <cmath>

namespace pose {

namespace {
constexpr int kSliderRange = 1000;
} // namespace

MotionPlaybackBar::MotionPlaybackBar(QWidget* parent) : QWidget(parent) {
    setFixedHeight(44);
    setStyleSheet(QStringLiteral(
        "QWidget {"
        "  background-color: #1e1e24;"
        "  border-top: 1px solid #2d2d38;"
        "  color: #e0e0e6;"
        "  font-family: 'Segoe UI', sans-serif;"
        "  font-size: 12px;"
        "}"
        "QPushButton {"
        "  background-color: #2b2b36;"
        "  border: 1px solid #3c3c4d;"
        "  border-radius: 4px;"
        "  padding: 4px 10px;"
        "  font-weight: 500;"
        "}"
        "QPushButton:hover {"
        "  background-color: #383847;"
        "  border-color: #4f4f66;"
        "}"
        "QPushButton:checked {"
        "  background-color: #0d6efd;"
        "  border-color: #0b5ed7;"
        "  color: #ffffff;"
        "}"
        "QSlider::groove:horizontal {"
        "  height: 6px;"
        "  background: #2d2d38;"
        "  border-radius: 3px;"
        "}"
        "QSlider::sub-page:horizontal {"
        "  background: #0d6efd;"
        "  border-radius: 3px;"
        "}"
        "QSlider::handle:horizontal {"
        "  background: #ffffff;"
        "  border: 1px solid #0d6efd;"
        "  width: 14px;"
        "  margin-top: -4px;"
        "  margin-bottom: -4px;"
        "  border-radius: 7px;"
        "}"
        "QSlider::handle:horizontal:hover {"
        "  background: #d0e2ff;"
        "}"
        "QLabel {"
        "  font-family: 'Consolas', monospace;"
        "  font-size: 11px;"
        "  color: #a0a0b2;"
        "}"
    ));

    auto* layout = new QHBoxLayout(this);
    layout->setContentsMargins(12, 4, 12, 4);
    layout->setSpacing(8);

    m_playButton = new QPushButton(QStringLiteral("▶ Play"), this);
    m_playButton->setFixedWidth(70);
    connect(m_playButton, &QPushButton::clicked, this, &MotionPlaybackBar::onPlayClicked);
    layout->addWidget(m_playButton);

    m_stopButton = new QPushButton(QStringLiteral("⏹ Stop"), this);
    m_stopButton->setFixedWidth(65);
    connect(m_stopButton, &QPushButton::clicked, this, &MotionPlaybackBar::onStopClicked);
    layout->addWidget(m_stopButton);

    m_slider = new QSlider(Qt::Horizontal, this);
    m_slider->setRange(0, kSliderRange);
    m_slider->setValue(0);
    connect(m_slider, &QSlider::valueChanged, this, &MotionPlaybackBar::onSliderValueChanged);
    connect(m_slider, &QSlider::sliderPressed, this, &MotionPlaybackBar::onSliderPressed);
    connect(m_slider, &QSlider::sliderReleased, this, &MotionPlaybackBar::onSliderReleased);
    layout->addWidget(m_slider, 1);

    m_timeLabel = new QLabel(QStringLiteral("00:00.00 / 00:10.00"), this);
    m_timeLabel->setFixedWidth(130);
    m_timeLabel->setAlignment(Qt::AlignCenter);
    layout->addWidget(m_timeLabel);

    m_loopButton = new QPushButton(QStringLiteral("🔁 Loop"), this);
    m_loopButton->setCheckable(true);
    m_loopButton->setChecked(true);
    connect(m_loopButton, &QPushButton::clicked, this, &MotionPlaybackBar::onLoopClicked);
    layout->addWidget(m_loopButton);

    m_cameraFollowButton = new QPushButton(QStringLiteral("🎥 Camera Follow"), this);
    m_cameraFollowButton->setCheckable(true);
    m_cameraFollowButton->setChecked(true);
    connect(m_cameraFollowButton, &QPushButton::clicked, this, &MotionPlaybackBar::onCameraFollowClicked);
    layout->addWidget(m_cameraFollowButton);
}

void MotionPlaybackBar::setPlaybackTime(float currentSeconds, float totalSeconds, bool isPlaying) {
    m_currentSeconds = currentSeconds;
    m_totalSeconds = (totalSeconds > 0.05f) ? totalSeconds : 10.0f;
    m_isPlaying = isPlaying;

    m_playButton->setText(m_isPlaying ? QStringLiteral("⏸ Pause") : QStringLiteral("▶ Play"));

    if (!m_isUserScrubbing) {
        float ratio = std::clamp(m_currentSeconds / m_totalSeconds, 0.0f, 1.0f);
        m_slider->blockSignals(true);
        m_slider->setValue(static_cast<int>(ratio * kSliderRange));
        m_slider->blockSignals(false);
    }

    m_timeLabel->setText(QStringLiteral("%1 / %2")
        .arg(formatTime(m_currentSeconds))
        .arg(formatTime(m_totalSeconds)));
}

void MotionPlaybackBar::setCameraFollowState(bool follow) {
    m_cameraFollowButton->setChecked(follow);
}

void MotionPlaybackBar::setLoopState(bool loop) {
    m_loopButton->setChecked(loop);
}

void MotionPlaybackBar::onPlayClicked() {
    if (m_isPlaying) {
        emit pauseRequested();
    } else {
        emit playRequested();
    }
}

void MotionPlaybackBar::onStopClicked() {
    emit stopRequested();
}

void MotionPlaybackBar::onSliderPressed() {
    m_isUserScrubbing = true;
}

void MotionPlaybackBar::onSliderReleased() {
    m_isUserScrubbing = false;
    float ratio = static_cast<float>(m_slider->value()) / static_cast<float>(kSliderRange);
    emit seekRequested(ratio * m_totalSeconds);
}

void MotionPlaybackBar::onSliderValueChanged(int value) {
    if (m_isUserScrubbing) {
        float ratio = static_cast<float>(value) / static_cast<float>(kSliderRange);
        float t = ratio * m_totalSeconds;
        m_timeLabel->setText(QStringLiteral("%1 / %2")
            .arg(formatTime(t))
            .arg(formatTime(m_totalSeconds)));
        emit seekRequested(t);
    }
}

void MotionPlaybackBar::onCameraFollowClicked() {
    emit cameraFollowToggled(m_cameraFollowButton->isChecked());
}

void MotionPlaybackBar::onLoopClicked() {
    emit loopToggled(m_loopButton->isChecked());
}

QString MotionPlaybackBar::formatTime(float seconds) {
    int totalMillis = static_cast<int>(std::round(std::max(0.0f, seconds) * 100.0f));
    int mins = totalMillis / 6000;
    int secs = (totalMillis % 6000) / 100;
    int millis = totalMillis % 100;
    return QStringLiteral("%1:%2.%3")
        .arg(mins, 2, 10, QLatin1Char('0'))
        .arg(secs, 2, 10, QLatin1Char('0'))
        .arg(millis, 2, 10, QLatin1Char('0'));
}

} // namespace pose
