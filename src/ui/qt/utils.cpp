
#include "utils.h"

#include <QDir>
#include <QFileInfo>
#include <QMessageBox>
#include <QPushButton>

#include "common/common.h"

#include "default_config.h"


#if DPSO_QT_RCC_ICONS
static QIcon getIconFromRcc(const QString &name)
{
    QIcon icon;

    static const auto sizes = QDir(":/icons").entryList(QDir::Dirs);
    for (const auto& size : sizes) {
        const QFileInfo fileInfo(
            QString(":/icons/%1/%2.png").arg(size, name));
        if (!fileInfo.exists())
            continue;

        icon.addFile(fileInfo.filePath());
    }

    return icon;
}
#endif


QIcon getIcon(const QString &name)
{
    #if DPSO_QT_RCC_ICONS

    if (QIcon::hasThemeIcon(name))
        return QIcon::fromTheme(name);

    return getIconFromRcc(name);

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
    confirmBox.addButton(okText, QMessageBox::AcceptRole);

    confirmBox.exec();

    return confirmBox.clickedButton() != cancelButton;
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
