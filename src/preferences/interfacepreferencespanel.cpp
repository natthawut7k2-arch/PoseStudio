/**
 * @file interfacepreferencespanel.cpp
 * @brief Implements the "Interface" preferences page.
 */

#include "interfacepreferencespanel.h"
#include "apptheme.h"
#include "applanguage.h"

#include <QApplication>
#include <QButtonGroup>
#include <QLabel>
#include <QRadioButton>
#include <QVBoxLayout>

InterfacePreferencesPanel::InterfacePreferencesPanel(QWidget* parent)
    : PreferencesPanel(tr("Interface"), parent) {

    // --- Appearance theme ---
    // Radio choice, not a checkbox: exactly one look is active at a time. Applies
    // instantly (no Apply button anywhere in this dialog) and persists, so the next
    // launch boots straight into the chosen theme via AppTheme::install().
    auto* themeLabel = new QLabel(tr("Appearance Theme"), this);
    themeLabel->setStyleSheet(QStringLiteral("font-weight: bold; font-size: 13px;"));
    contentLayout()->addWidget(themeLabel);

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

    contentLayout()->addSpacing(16);

    // --- Language selection ---
    auto* langLabel = new QLabel(tr("Language"), this);
    langLabel->setStyleSheet(QStringLiteral("font-weight: bold; font-size: 13px;"));
    contentLayout()->addWidget(langLabel);

    auto* langGroup = new QButtonGroup(this);
    const QVector<AppLanguage::LanguageInfo> languages = AppLanguage::availableLanguages();
    const QString currentLang = AppLanguage::currentLanguage();

    for (int i = 0; i < languages.size(); ++i) {
        const auto& lang = languages[i];
        auto* radio = new QRadioButton(lang.displayName, this);
        langGroup->addButton(radio, i);
        if (lang.code == currentLang) {
            radio->setChecked(true);
        }
        contentLayout()->addWidget(radio);
    }

    addDescription(
        tr("Chooses the interface display language. The change is remembered between launches. "
           "A restart of PoseStudio is recommended for all menus and windows to refresh completely."));

    connect(langGroup, &QButtonGroup::idClicked, this, [languages](int id) {
        if (id >= 0 && id < languages.size()) {
            AppLanguage::applyLanguage(*qApp, languages[id].code);
        }
    });
}
