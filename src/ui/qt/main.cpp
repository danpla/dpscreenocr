
// On Windows, libintl patches setlocale with a macro that expands to
// libintl_setlocale. std::setlocale becomes std::libintl_setlocale,
// so we must use <locale.h> rather than <clocale>.
#include <locale.h>

#include <cstddef>

#include <QApplication>
#include <QDir>
#include <QLibraryInfo>
#include <QLocale>
#if QT_VERSION < QT_VERSION_CHECK(5, 0, 0)
#include <QTextCodec>
#endif
#include <QTranslator>

#include "dpso_intl/dpso_intl.h"

#include "ui_common/ui_common.h"

#include "default_config.h"
#include "main_window.h"


static void installQtTranslations(QApplication& app)
{
    const auto qtTranslationsPath = (
        #ifdef Q_OS_WIN
        QCoreApplication::applicationDirPath() + "/translations"
        #else
        QLibraryInfo::location(QLibraryInfo::TranslationsPath)
        #endif
    );

    const auto qtLocaleName = QLocale::system().name();

    const QString translations[] = {
        "qt",
        #if QT_VERSION >= QT_VERSION_CHECK(5, 0, 0)
        "qtbase",
        #endif
    };
    static const auto numTranslations = (
        sizeof(translations) / sizeof(*translations));

    static QTranslator translators[numTranslations];

    for (std::size_t i = 0; i < numTranslations; ++i) {
        const auto& translation = translations[i];
        auto& translator = translators[i];

        if (translator.load(
                translation + "_" + qtLocaleName, qtTranslationsPath))
            app.installTranslator(&translator);
    }
}


int main(int argc, char *argv[])
{
    // Setting UTF-8 is only necessary in Qt 4, where
    // QString(const char *) constructor uses fromAscii(). In Qt 5,
    // it uses fromUtf8(), and there is no from/toAscii() and
    // setCodecForCStrings().
    #if QT_VERSION < QT_VERSION_CHECK(5, 0, 0)
    QTextCodec::setCodecForCStrings(
        QTextCodec::codecForName("UTF-8"));
    #endif

    QApplication app(argc, argv);

    // Our main window may be hidden to tray. As of Qt 5.15, ignoring
    // QCloseEvent has no effect if the window is hidden when
    // closeEvent() returns, even if WA_QuitOnClose is disabled. A
    // workaround is to either disable quitOnLastWindowClosed and
    // quit explicitly when needed (our approach), or to make sure
    // that the window is visible before returning from closeEvent().
    app.setQuitOnLastWindowClosed(false);

    setlocale(LC_ALL, "");

    #if DPSO_QT_LOCAL_DATA

    const auto localLocaleDir = QDir::toNativeSeparators(
        QCoreApplication::applicationDirPath() + "/locale");
    if (QDir(localLocaleDir).exists())
        bindtextdomain(
            appFileName, localLocaleDir.toLocal8Bit().data());
    else
        bindtextdomain(appFileName, localeDir);

    #else

    bindtextdomain(appFileName, localeDir);

    #endif

    bind_textdomain_codeset(appFileName, "UTF-8");
    textdomain(appFileName);

    installQtTranslations(app);

    MainWindow mainWindow;

    return app.exec();
}
