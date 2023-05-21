
#include <cstddef>
#include <cstdlib>

#include <QApplication>
#include <QDir>
#include <QLibraryInfo>
#include <QLocale>
#include <QtGlobal>
#include <QMessageBox>
#include <QTranslator>

#include "dpso/dpso.h"
#include "dpso_intl/dpso_intl.h"
#include "dpso_utils/dpso_utils.h"
#include "ui_common/ui_common.h"

#include "main_window.h"
#include "utils.h"


#define _(S) gettext(S)


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

    const QString translations[] = {"qt", "qtbase"};
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
    uiPrepareEnvironment(argv);

    // High DPI support is enabled by default in Qt 6.
    #if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
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

    // We want the message boxes below to use the proper application
    // icon. On platforms that don't use icons themes, getThemeIcon()
    // falls back to the application data dir, so we need to
    // initialize app dirs before anything else.
    if (!uiInitAppDirs(argv[0])) {
        QMessageBox::critical(
            nullptr,
            uiAppName,
            QString("uiInitAppDirs(): ") + dpsoGetError());
        return EXIT_FAILURE;
    }

    app.setWindowIcon(getThemeIcon(uiAppFileName));

    uiInitIntl();

    const ui::SingleInstanceGuardUPtr singleInstanceGuard{
        uiSingleInstanceGuardCreate(uiAppFileName)};
    if (!singleInstanceGuard) {
        QMessageBox::critical(
            nullptr,
            uiAppName,
            QString("uiSingleInstanceGuardCreate(): ")
                + dpsoGetError());
        return EXIT_FAILURE;
    }

    if (!uiSingleInstanceGuardIsPrimary(singleInstanceGuard.get())) {
        QMessageBox::information(
            nullptr,
            uiAppName,
            dpsoStrNFormat(
                _("{app_name} is already running"),
                {{"app_name", uiAppName}}));
        return EXIT_SUCCESS;
    }

    installQtTranslations(app);

    MainWindow mainWindow;

    return app.exec();
}
