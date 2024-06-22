
#include <cstdlib>
#include <optional>

#include <QApplication>
#include <QLibraryInfo>
#include <QLocale>
#include <QtGlobal>
#include <QMessageBox>
#include <QTranslator>

#include "dpso/dpso.h"
#include "dpso_intl/dpso_intl.h"
#include "dpso_utils/dpso_utils.h"
#include "ui_common/ui_common.h"

#include "error.h"
#include "main_window.h"
#include "utils.h"


#define _(S) gettext(S)


static void installQtTranslations(QApplication& app)
{
    const auto translationsPath =
        #if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
        QLibraryInfo::path(
        #else
        QLibraryInfo::location(
        #endif
            QLibraryInfo::TranslationsPath);

    const auto localeName = QLocale::system().name();

    const QString translations[]{"qt", "qtbase"};

    for (const auto& translation : translations) {
        auto* translator = new QTranslator(&app);
        if (translator->load(
                translation + "_" + localeName, translationsPath))
            app.installTranslator(translator);
    }
}


int main(int argc, char* argv[])
{
    UiStartupArgs startupArgs;

    // If uiInit() fails, postpone displaying a message box until
    // QApplication is ready. Actually, we can call uiInit() after
    // QApplication has been constructed, but on some platforms
    // uiInit() can restart the executable, so we want to avoid
    // unnecessary QApplication initialization in this case.
    const auto uiInitOk = uiInit(argc, argv, &startupArgs);

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

    if (!uiInitOk) {
        QMessageBox::critical(
            nullptr,
            uiAppName,
            QString("uiInit(): ") + dpsoGetError());
        return EXIT_FAILURE;
    }

    // On platforms that don't use icon themes, getThemeIcon() loads
    // icons from the application data dir (uiGetAppDir()), and should
    // therefore only be called after a successful uiInit().
    app.setWindowIcon(ui::qt::getThemeIcon(uiIconNameApp));

    installQtTranslations(app);

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

    // Wrap MainWindow in optional to catch ui::qt::Error only from
    // the constructor and exclude QApplication::exec() from try-catch
    // as the other methods are not expected to throw.
    std::optional<ui::qt::MainWindow> mainWindow;

    try {
        mainWindow.emplace(startupArgs);
    } catch (ui::qt::Error& e) {
        QMessageBox::critical(nullptr, uiAppName, e.what());
        return EXIT_FAILURE;
    }

    return app.exec();
}
