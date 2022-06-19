
#include "utils.h"

#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QFont>
#include <QMessageBox>
#include <QPushButton>

#include "ui_common/ui_common.h"

#include "default_config.h"


#if DPSO_QT_LOCAL_DATA
static QIcon loadIcon(const QString &name)
{
    QIcon icon;

    static const auto iconsDir =
        QCoreApplication::applicationDirPath() + "/icons";

    static const auto sizes = QDir(iconsDir).entryList(QDir::Dirs);
    for (const auto& size : sizes) {
        const QFileInfo fileInfo(
            QString(iconsDir + "/%1/%2.png").arg(size, name));
        if (fileInfo.exists())
            icon.addFile(fileInfo.filePath());
    }

    return icon;
}
#endif


QIcon getIcon(const QString &name)
{
    #if DPSO_QT_LOCAL_DATA

    if (QIcon::hasThemeIcon(name))
        return QIcon::fromTheme(name);

    return loadIcon(name);

    #else

    return QIcon::fromTheme(name);

    #endif
}


bool confirmation(
    QWidget* parent,
    const QString& question,
    const QString& cancelText,
    const QString& okText)
{
    QMessageBox confirmBox(parent);

    confirmBox.setWindowTitle(appName);
    confirmBox.setText(question);
    confirmBox.setIcon(QMessageBox::Question);

    auto* cancelButton = confirmBox.addButton(
        cancelText, QMessageBox::RejectRole);
    confirmBox.setDefaultButton(cancelButton);
    auto* okButton = confirmBox.addButton(
        okText, QMessageBox::AcceptRole);

    confirmBox.exec();

    return confirmBox.clickedButton() == okButton;
}


void setMonospace(QFont& font)
{
    #ifdef Q_OS_WIN
    font.setFamily("Consolas");
    #else
    font.setFamily("monospace");
    #endif
    font.setStyleHint(QFont::Monospace);
}
