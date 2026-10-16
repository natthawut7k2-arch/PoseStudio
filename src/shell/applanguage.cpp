/**
 * @file applanguage.cpp
 * @brief Implementation of AppLanguage — translation catalog loading and runtime switching.
 */

#include "applanguage.h"
#include "constants.h"
#include "preferencesmanager.h"

#include <QApplication>
#include <QDebug>
#include <QTranslator>

namespace {

static QTranslator* s_appTranslator = nullptr;

const QVector<AppLanguage::LanguageInfo>& languageRegistry() {
    static const QVector<AppLanguage::LanguageInfo> s_languages = {
        {
            QLatin1String(Constants::LANG_ENGLISH),
            QStringLiteral("English"),
            QStringLiteral("English"),
            QString()
        },
        {
            QLatin1String(Constants::LANG_THAI),
            QStringLiteral("ไทย (Thai)"),
            QStringLiteral("ภาษาไทย"),
            QStringLiteral(":/resources/translations/posestudio_th.qm")
        }
    };
    return s_languages;
}

} // namespace

namespace AppLanguage {

QVector<LanguageInfo> availableLanguages() {
    return languageRegistry();
}

QString currentLanguage() {
    return PreferencesManager::instance()
        .getValue(QLatin1String(Constants::PREF_LANGUAGE),
                  QLatin1String(Constants::LANG_ENGLISH)).toString();
}

LanguageInfo currentLanguageInfo() {
    const QString current = currentLanguage();
    for (const auto& lang : languageRegistry()) {
        if (lang.code == current) {
            return lang;
        }
    }
    return languageRegistry().first();
}

void install(QApplication& app) {
    const QString lang = currentLanguage();
    if (lang == QLatin1String(Constants::LANG_ENGLISH)) {
        return; // English is the built-in base language, no translator needed
    }

    LanguageInfo info;
    bool found = false;
    for (const auto& l : languageRegistry()) {
        if (l.code == lang) {
            info = l;
            found = true;
            break;
        }
    }

    if (!found || info.qmResource.isEmpty()) {
        return;
    }

    auto* translator = new QTranslator(&app);
    if (translator->load(info.qmResource)) {
        app.installTranslator(translator);
        s_appTranslator = translator;
        qDebug() << "[i18n] Installed translation catalog for language:" << lang;
    } else {
        qWarning() << "[!] Failed to load translation catalog from:" << info.qmResource;
        delete translator;
    }
}

bool applyLanguage(QApplication& app, const QString& langCode) {
    PreferencesManager::instance().setValue(
        QLatin1String(Constants::PREF_LANGUAGE), langCode);

    if (s_appTranslator) {
        app.removeTranslator(s_appTranslator);
        delete s_appTranslator;
        s_appTranslator = nullptr;
    }

    if (langCode == QLatin1String(Constants::LANG_ENGLISH)) {
        return true;
    }

    LanguageInfo info;
    bool found = false;
    for (const auto& l : languageRegistry()) {
        if (l.code == langCode) {
            info = l;
            found = true;
            break;
        }
    }

    if (!found || info.qmResource.isEmpty()) {
        return false;
    }

    auto* translator = new QTranslator(&app);
    if (translator->load(info.qmResource)) {
        app.installTranslator(translator);
        s_appTranslator = translator;
        qDebug() << "[i18n] Switched translation catalog to language:" << langCode;
        return true;
    } else {
        qWarning() << "[!] Failed to load translation catalog from:" << info.qmResource;
        delete translator;
        return false;
    }
}

} // namespace AppLanguage
