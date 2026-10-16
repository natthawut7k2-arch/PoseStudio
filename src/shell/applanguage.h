/**
 * @file applanguage.h
 * @brief Manages the application display language and translation catalogs.
 *
 * Provides language switching between English (built-in base language) and
 * translations (e.g. Thai, with easy extensibility for future languages).
 * The preference is persisted in the Preferences table.
 */

#ifndef APPLANGUAGE_H
#define APPLANGUAGE_H

#include <QString>
#include <QVector>

class QApplication;

namespace AppLanguage {

struct LanguageInfo {
    QString code;         ///< Language code, e.g. "en", "th"
    QString displayName;  ///< Label for UI display, e.g. "English", "ไทย (Thai)"
    QString nativeName;   ///< Native name, e.g. "English", "ภาษาไทย"
    QString qmResource;   ///< Resource path to .qm file, e.g. ":/resources/translations/posestudio_th.qm" (empty for English)
};

/**
 * @brief Returns the list of all available languages.
 * Extensible for future language additions.
 */
QVector<LanguageInfo> availableLanguages();

/**
 * @brief Installs the saved language on application startup.
 * Call from main() before constructing the UI.
 */
void install(QApplication& app);

/**
 * @brief Changes the active language, loads/unloads translators, and persists the choice.
 * @return True if the language switch succeeded.
 */
bool applyLanguage(QApplication& app, const QString& langCode);

/**
 * @brief Current active language code (e.g. "en", "th").
 */
QString currentLanguage();

/**
 * @brief LanguageInfo of the currently active language.
 */
LanguageInfo currentLanguageInfo();

} // namespace AppLanguage

#endif // APPLANGUAGE_H
