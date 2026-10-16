/**
 * @file customtooltip.cpp
 * @brief Implementation of CustomToolTip. See the header for what it is for.
 */

#include "customtooltip.h"
#include "apptheme.h"
#include "constants.h"

#include <QEnterEvent>
#include <QLabel>
#include <QTimer>
#include <QVBoxLayout>

CustomToolTip::CustomToolTip(QWidget* parent) : QWidget(parent, Qt::ToolTip) {
    setWindowFlags(Qt::ToolTip | Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint);
    setAttribute(Qt::WA_ShowWithoutActivating);

    // The outer top-level widget is a transparent container; the styled label inside does the
    // actual drawing. Rendering the rounded/bordered CSS on a child rather than directly on a
    // top-level translucent window avoids the rectangular-background artifacts Qt otherwise
    // leaves around the rounded corners.
    setAttribute(Qt::WA_TranslucentBackground);
    setMouseTracking(true);

    // Zero margins so the label fills the whole container.
    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);

    textLabel = new QLabel(this);
    // Theme-aware: the same tooltip surface as QToolTip in global.qss (dark #1e1f22,
    // light #ffffff). Rebuilt at construction; a runtime theme switch takes effect on
    // the next tooltip (the widget is re-shown per hover).
    textLabel->setStyleSheet(
        QStringLiteral("QLabel { background-color: %1; color: %2; border: 1px solid %3;"
                         " border-radius: 4px; padding: 6px 10px; font-size: 13px; }")
            .arg(AppTheme::themed<QString>(QStringLiteral("#1e1f22"), QStringLiteral("#ffffff")))
            .arg(AppTheme::themed<QString>(QStringLiteral("#e0e0e0"), QStringLiteral("#1a1b1c")))
            .arg(AppTheme::themed<QString>(QStringLiteral("#3a3b3c"), QStringLiteral("#c4c7cb")))
    );

    layout->addWidget(textLabel);

    hideTimer = new QTimer(this);
    hideTimer->setSingleShot(true);
    connect(hideTimer, &QTimer::timeout, this, &CustomToolTip::hide);
}

void CustomToolTip::setText(const QString& text) {
    textLabel->setText(text);
}

void CustomToolTip::startHideTimer(int ms) { hideTimer->start(ms); }
void CustomToolTip::stopHideTimer() { hideTimer->stop(); }

void CustomToolTip::enterEvent(QEnterEvent* event) {
    stopHideTimer();
    QWidget::enterEvent(event);
}

void CustomToolTip::leaveEvent(QEvent* event) {
    startHideTimer(Constants::TOOLTIP_HIDE_DELAY_MS);
    QWidget::leaveEvent(event);
}
