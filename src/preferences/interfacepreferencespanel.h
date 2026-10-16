/**
 * @file interfacepreferencespanel.h
 * @brief "Interface" page of the Preferences dialog: the appearance theme choice.
 *
 * Promoted from a PlaceholderPreferencesPanel (see preferencespanel.h for the recipe):
 * Dark is the original look, Light mirrors the same QSS module structure with a light
 * palette. The choice applies instantly via AppTheme::applyTheme() and persists to the
 * Preferences table, so it survives restarts.
 */

#ifndef INTERFACEPREFERENCESPANEL_H
#define INTERFACEPREFERENCESPANEL_H

#include "preferencespanel.h"

/// Interface and appearance settings. Today: the dark/light theme radio choice.
class InterfacePreferencesPanel : public PreferencesPanel {
    Q_OBJECT

public:
    explicit InterfacePreferencesPanel(QWidget* parent = nullptr);
};

#endif // INTERFACEPREFERENCESPANEL_H
