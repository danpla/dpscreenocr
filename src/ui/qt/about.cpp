
#include "about.h"

#include <initializer_list>

#include <QApplication>
#include <QCursor>
#include <QDesktopServices>
#include <QDir>
#include <QFileInfo>
#include <QLabel>
#include <QStringList>
#include <QTextEdit>
#include <QToolTip>
#include <QUrl>
#include <QVBoxLayout>

#include "dpso_intl/dpso_intl.h"

#include "ui_common/ui_common.h"

#include "utils.h"


#define _(S) gettext(S)


namespace ui::qt {


const auto* const aboutLicenseUrl = "about:license";


static QString formatLink(const QString& name, const QUrl& url)
{
    return QString("<a href=\"%1\">%2</a>").arg(url.toString(), name);
}


static bool appendFileLink(
    QStringList& links,
    const QString& docDirPath,
    const QString& fileName,
    const QString& linkName)
{
    const auto filePath = docDirPath + '/' + fileName;
    if (!QFileInfo(filePath).exists())
        return false;

    links.append(formatLink(linkName, QUrl::fromLocalFile(filePath)));
    return true;
}


static QStringList createLinks()
{
    QStringList result;
    result.append(formatLink(_("Website"), QUrl(uiAppWebsite)));

    const auto docDirPath =
        QDir::fromNativeSeparators(uiGetAppDir(UiAppDirDoc));

    for (const auto* ext : {".html", ".txt"})
        if (appendFileLink(
                result,
                docDirPath,
                QString("manual") + ext,
                _("User manual")))
            break;

    appendFileLink(
        result, docDirPath, "changelog.txt", _("Changelog"));

    result.append(formatLink(_("License"), QUrl(aboutLicenseUrl)));

    appendFileLink(
        result,
        docDirPath,
        "third-party-licenses",
        _("Third-party licenses"));

    return result;
}


About::About(QWidget* parent)
    : QWidget{parent}
{
    const auto fontHeight = fontMetrics().height();

    auto* iconLabel = new QLabel();
    iconLabel->setAlignment(Qt::AlignCenter);
    iconLabel->setPixmap(
        getIcon(uiAppFileName).pixmap(fontHeight * 5));

    auto* infoTextLabel = new QLabel();
    infoTextLabel->setAlignment(Qt::AlignCenter);
    infoTextLabel->setTextFormat(Qt::RichText);
    infoTextLabel->setTextInteractionFlags(
        Qt::TextSelectableByMouse
        | Qt::LinksAccessibleByMouse
        | Qt::LinksAccessibleByKeyboard);
    infoTextLabel->setText(QString(
        "<p style=\"line-height: 140%;\">"
        "<b>%1</b><br>"
        "%2<br>"
        "%3<br>"
        "%4</p>"
        ).arg(
            joinInLayoutDirection(" ", {uiAppName, uiAppVersion}),
            _("Program to recognize text on screen"),
            // The proper way to add spacing around a piece of text is
            // to use <span> with the margin or padding CSS property,
            // but it has no effect in Qt 5, so we use non-breaking
            // spaces instead. Another way is to put individual link
            // labels in QHBoxLayout and set the margin, but in Qt 5
            // the keyboard navigation doesn't work properly in this
            // case (in particular, Ctrl + Tab doesn't move focus from
            // the link).
            joinInLayoutDirection(
                "&nbsp;&nbsp;|&nbsp;&nbsp;", createLinks()),
            uiAppCopyright));

    connect(
        infoTextLabel, &QLabel::linkActivated,
        this, &About::handleLinkActivation);
    connect(
        infoTextLabel, &QLabel::linkHovered,
        this, &About::handleLinkHover);

    textEdit = new QTextEdit();
    textEdit->setReadOnly(true);
    textEdit->setVisible(false);

    auto textEditFont = textEdit->currentFont();
    setMonospace(textEditFont);
    textEdit->setCurrentFont(textEditFont);

    auto* layout = new QVBoxLayout(this);

    QMargins margins;
    margins.setTop(fontHeight);
    layout->setContentsMargins(margins);

    layout->addWidget(iconLabel);
    layout->addWidget(infoTextLabel);
    layout->addWidget(textEdit, 1);
    layout->addStretch();
}


static QString getTextForLink(const QString& link)
{
    if (link == aboutLicenseUrl)
        return uiAppLicense;

    if (const QUrl url(link);
            url.isLocalFile()
            && QFileInfo(url.toLocalFile()).suffix() == "txt")
        if (QFile file(url.toLocalFile());
                file.open(QFile::ReadOnly | QFile::Text))
            return file.readAll();

    return {};
}


void About::handleLinkActivation(const QString& link)
{
    if (link == currentTextLink) {
        textEdit->setVisible(!textEdit->isVisible());
        return;
    }

    if (const auto text = getTextForLink(link); !text.isEmpty()) {
        textEdit->setPlainText(text);
        textEdit->show();
        currentTextLink = link;
        return;
    }

    QDesktopServices::openUrl(QUrl(link));
}


void About::handleLinkHover(const QString &link)
{
    const QUrl url(link);
    if (url.scheme() == "about")
        return;

    QString tooltip;
    if (url.isLocalFile())
        tooltip = QDir::toNativeSeparators(url.toLocalFile());
    else
        tooltip = link;

    QToolTip::showText(QCursor::pos(), tooltip);
}


}
