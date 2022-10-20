
#include <cstddef>
#include <cstdlib>

#include <QApplication>
#include <QDir>
#include <QLibraryInfo>
#include <QLocale>
#include <QtGlobal>
#include <QMessageBox>
#if QT_VERSION < QT_VERSION_CHECK(5, 0, 0)
#include <QTextCodec>
#endif
#include <QTranslator>

#include "dpso/dpso.h"
#include "ui_common/ui_common.h"

#include "main_window.h"


static void installQtTranslations(QApplication& app)
{
    const auto qtTranslationsPath =
        #if defined(Q_OS_UNIX) && !defined(Q_OS_DARWIN)
        QLibraryInfo::location(QLibraryInfo::TranslationsPath);
        #else
        QDir::fromNativeSeparators(uiGetAppDir(UiAppDirData))
            + "/translations";
        #endif

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

    if (!uiInitAppDirs(argv[0])) {
        QMessageBox::critical(
            nullptr,
            uiAppName,
            QString("uiInitAppDirs(): ") + dpsoGetError());
        std::exit(EXIT_FAILURE);
    }

    uiInitIntl();
    installQtTranslations(app);

    MainWindow mainWindow;

    return app.exec();
}
