
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

#include "dpso_intl/dpso_bindtextdomain_utf8.h"
#include "dpso_intl/dpso_intl.h"

#include "ui_common/ui_common.h"

#include "default_config.h"
#include "main_window.h"


static void installQtTranslations(QApplication& app)
{
    const auto qtTranslationsPath =
        #ifdef Q_OS_WIN
        QCoreApplication::applicationDirPath() + "/translations"
        #else
        QLibraryInfo::location(QLibraryInfo::TranslationsPath)
        #endif
    ;

    const auto qtLocaleName = QLocale::system().name();

    const QString translations[] = {
        "qt",
        #if QT_VERSION >= QT_VERSION_CHECK(5, 0, 0)
        "qtbase",
        #endif
    };
    static const auto numTranslations =
        sizeof(translations) / sizeof(*translations);

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
    // QString(const char *) constructor uses fromAscii(). In Qt 5 and
    // newer, it uses fromUtf8(), and there is no from/toAscii() and
    // setCodecForCStrings().
    #if QT_VERSION < QT_VERSION_CHECK(5, 0, 0)
    QTextCodec::setCodecForCStrings(
        QTextCodec::codecForName("UTF-8"));
    #endif

    // High DPI support is enabled by default in Qt 6.
    #if QT_VERSION >= QT_VERSION_CHECK(5, 1, 0) \
        && QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    QCoreApplication::setAttribute(Qt::AA_UseHighDpiPixmaps);

    #if QT_VERSION >= QT_VERSION_CHECK(5, 6, 0)
    QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
    #endif

    // We could also set HighDpiScaleFactorRoundingPolicy to
    // PassThrough since this is default in Qt 6, but high DPI support
    // in Qt 5 is worse than in 6, and the default policy (Round)
    // actually looks better: at least it doesn't result in tiny text
    // on Windows 10 at 150% scale.
    #endif

    QApplication app(argc, argv);

    setlocale(LC_ALL, "");

    #if DPSO_QT_LOCAL_DATA

    const auto localLocaleDir = QDir::toNativeSeparators(
        QCoreApplication::applicationDirPath() + "/locale");
    if (QDir(localLocaleDir).exists())
        bindtextdomainUtf8(
            uiAppFileName, localLocaleDir.toUtf8().data());
    else
        bindtextdomainUtf8(uiAppFileName, uiLocaleDir);

    #else

    bindtextdomainUtf8(uiAppFileName, uiLocaleDir);

    #endif

    bind_textdomain_codeset(uiAppFileName, "UTF-8");
    textdomain(uiAppFileName);

    installQtTranslations(app);

    MainWindow mainWindow;

    return app.exec();
}
