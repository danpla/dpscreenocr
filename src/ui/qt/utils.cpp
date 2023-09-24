
#include "utils.h"

#include <QApplication>
#include <QDir>
#include <QFileInfo>
#include <QFont>
#include <QMessageBox>
#include <QPushButton>
#include <QtGlobal>

#include "dpso_intl/dpso_intl.h"
#include "ui_common/ui_common.h"


#define _(S) gettext(S)


namespace ui::qt {


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
    QMessageBox messageBox(parent);

    messageBox.setWindowTitle(uiAppName);
    messageBox.setText(question);
    messageBox.setIcon(QMessageBox::Warning);

    auto* cancelButton = messageBox.addButton(
        cancelText, QMessageBox::RejectRole);
    messageBox.setDefaultButton(cancelButton);
    auto* okButton = messageBox.addButton(
        okText, QMessageBox::AcceptRole);

    messageBox.exec();

    return messageBox.clickedButton() == okButton;
}


void showError(
    QWidget* parent,
    const QString& text,
    const QString& informativeText,
    const QString& detailedText)
{
    QMessageBox messageBox(parent);

    messageBox.setWindowTitle(uiAppName);
    messageBox.setIcon(QMessageBox::Critical);

    messageBox.setText(text);
    messageBox.setInformativeText(informativeText);
    messageBox.setDetailedText(detailedText);

    messageBox.addButton(_("Close"), QMessageBox::RejectRole);

    messageBox.exec();
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


}
