/**
 * @file interfacepreferencespanel.cpp
 * @brief Implements the "Interface" preferences page.
 */

#include "interfacepreferencespanel.h"
#include "apptheme.h"

#include <QApplication>
#include <QButtonGroup>
#include <QRadioButton>
#include <QVBoxLayout>

InterfacePreferencesPanel::InterfacePreferencesPanel(QWidget* parent)
    : PreferencesPanel(tr("Interface"), parent) {

    // --- Appearance theme ---
    // Radio choice, not a checkbox: exactly one look is active at a time. Applies
    // instantly (no Apply button anywhere in this dialog) and persists, so the next
    // launch boots straight into the chosen theme via AppTheme::install().
    auto* darkOption = new QRadioButton(tr("Dark"), this);
    auto* lightOption = new QRadioButton(tr("Light"), this);
    darkOption->setToolTip(tr("The original dark look"));
    lightOption->setToolTip(tr("A light look with the same layout"));

    auto* themeGroup = new QButtonGroup(this);
    themeGroup->addButton(darkOption, 0);
    themeGroup->addButton(lightOption, 1);
    if (AppTheme::currentTheme() == AppTheme::Theme::Light) {
        lightOption->setChecked(true);
    } else {
        darkOption->setChecked(true);
    }

    contentLayout()->addWidget(darkOption);
    contentLayout()->addWidget(lightOption);

    addDescription(
        tr("Chooses the application's look. The change applies immediately and is "
           "remembered between launches."));

    connect(themeGroup, &QButtonGroup::idClicked, this, [](int id) {
        AppTheme::applyTheme(*qApp, id == 1 ? AppTheme::Theme::Light
                                            : AppTheme::Theme::Dark);
    });
}
