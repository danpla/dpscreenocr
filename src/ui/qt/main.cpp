
#include <clocale>

#include <QApplication>
#include <QDir>
#include <QTextCodec>

#include "dpso_utils/intl.h"

#include "common/common.h"

#include "default_config.h"
#include "main_window.h"


int main(int argc, char *argv[])
{
    // Setting UTF-8 is only necessary in Qt 4, where
    // QString(const char *) constructor uses fromAscii(). In Qt 5,
    // it uses fromUtf8(), and there is no from/toAscii() and
    // setCodecForCStrings().
    #if (QT_VERSION < QT_VERSION_CHECK(5, 0, 0))
    QTextCodec::setCodecForCStrings(
        QTextCodec::codecForName("UTF-8"));
    #endif

    QApplication app(argc, argv);

    std::setlocale(LC_ALL, "");

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

    MainWindow mainWindow;
    mainWindow.show();

    return app.exec();
}
