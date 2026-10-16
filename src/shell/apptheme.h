/**
 * @file apptheme.h
 * @brief The one place the application's look is installed: icon, Fusion proxy style, the
 *        concatenated QSS modules, tooltip effects, and the app-wide menu-shadow filter.
 *
 * main.cpp used to carry all of this inline (a MenuShadowFilter class, a loadStylesheets()
 * helper and a "Branding & theming" block). Keeping it here leaves main.cpp as pure wiring and
 * gives the theming decisions a single home; nothing in a widget constructor should set the
 * style, the stylesheet, or the tooltip effect flags — call AppTheme::install() once, and only
 * from main().
 */

#ifndef APPTHEME_H
#define APPTHEME_H

class QApplication;
class QString;

namespace AppTheme {

/// The available application looks. Dark is the original theme; light mirrors
/// the same QSS module structure with a light palette (resources/styles/light/).
enum class Theme {
    Dark,
    Light,
};

/**
 * @brief Applies PoseStudio's theme to @p app. Call once, after the database and preferences
 *        are up (the saved theme is read from them) and BEFORE any widget is constructed,
 *        so every widget polishes under the final style + stylesheet.
 *
 * Installs, in order: the window icon; the tooltip effect toggles; AppProxyStyle over Fusion
 * (never the native OS style — the theme must render identically on every platform); the
 * QSS modules for the saved theme in cascade order; and the MenuShadowFilter that strips
 * the native popup shadow from every QMenu while keeping the rounded corners (see apptheme.cpp).
 */
void install(QApplication& app);

/**
 * @brief Switches the running app to @p theme immediately: reloads the QSS modules,
 *        re-polishes every existing widget, and persists the choice to preferences.
 * Safe to call from the Interface preferences page; the startup path uses install().
 */
void applyTheme(QApplication& app, Theme theme);

/// The theme saved in preferences (Constants::PREF_APPEARANCE_THEME). Unknown or
/// absent values map to Dark, so pre-light-theme installs keep their look.
Theme currentTheme();

/// "dark"/"light" names for the preference value (Constants::THEME_DARK/LIGHT).
QString themeName(Theme theme);

/// Parses a stored preference value; anything unrecognised maps to Dark.
Theme themeFromName(const QString& name);

/// True when the active theme is Light. Hardcoded C++ colors that QSS cannot
/// reach (tooltips, viewport strip, manual pages) branch on this.
bool isLightTheme();

/**
 * @brief Picks the per-theme variant of a hardcoded color: @p dark when the
 *        active theme is Dark, @p light otherwise. For C++-painted surfaces
 *        whose colors live outside the QSS modules.
 */
template <typename T>
T themed(const T& dark, const T& light) {
    return isLightTheme() ? light : dark;
}

} // namespace AppTheme

#endif // APPTHEME_H
