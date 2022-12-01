
#include "utils.h"

#include <QApplication>
#include <QDir>
#include <QFileInfo>
#include <QFont>
#include <QMessageBox>
#include <QPushButton>
#include <QtGlobal>

#include "ui_common/ui_common.h"


QString joinInLayoutDirection(
    const QString& separator, const QStringList& list)
{
    QString result;

    const auto rtl = QApplication::isRightToLeft();
    // Since Qt 6, size_type switched from int to qsizetype.
    for (QStringList::size_type i = 0; i < list.size(); ++i) {
        if (i > 0)
            result.insert(rtl ? 0 : result.size(), separator);

        result.insert(rtl ? 0 : result.size(), list[i]);
    }

    return result;
}


QIcon getIcon(const QString &name)
{
    QIcon icon;

    static const auto iconsDir =
        QDir::fromNativeSeparators(uiGetAppDir(UiAppDirData))
        + "/icons";

    static const auto sizes = QDir(iconsDir).entryList(QDir::Dirs);
    for (const auto& size : sizes) {
        const QFileInfo fileInfo(
            QString(iconsDir + "/%1/%2.png").arg(size, name));
        if (fileInfo.exists())
            icon.addFile(fileInfo.filePath());
    }

    return icon;
}


QIcon getThemeIcon(const QString &name)
{
    #if defined(Q_OS_UNIX) && !defined(Q_OS_DARWIN)

    if (QIcon::hasThemeIcon(name))
        return QIcon::fromTheme(name);

    #endif

    return getIcon(name);
}


bool confirmDestructiveAction(
    QWidget* parent,
    const QString& question,
    const QString& cancelText,
    const QString& okText)
{
    QMessageBox confirmBox(parent);

    confirmBox.setWindowTitle(uiAppName);
    confirmBox.setText(question);
    confirmBox.setIcon(QMessageBox::Warning);

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
