
#include "about.h"

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
#define _(S) gettext(S)

#include "ui_common/ui_common.h"

#include "utils.h"


const auto* const aboutLicenseUrl = "about:license";


static QString formatLink(const QString& name, const QUrl& url)
{
    return QString("<a href=\"%1\">%2</a>").arg(url.toString(), name);
}


static void appendFileLink(
    QStringList& links,
    const QString& docDirPath,
    const QString& fileName,
    const QString& linkName)
{
    const auto filePath = docDirPath + '/' + fileName;
    if (!QFileInfo(filePath).exists())
        return;

    links.append(formatLink(linkName, QUrl::fromLocalFile(filePath)));
}


static QStringList createLinks()
{
    QStringList result;
    result.append(formatLink("Website", QUrl(appWebsite)));

    const auto docDirPath =
        QCoreApplication::applicationDirPath() + "/doc";
    if (QDir(docDirPath).exists()) {
        appendFileLink(
            result, docDirPath, "manual.html", _("Manual"));
        appendFileLink(
            result, docDirPath, "changelog.txt", _("Changelog"));
        appendFileLink(
            result, docDirPath, "LICENSE.txt", _("License"));
        appendFileLink(
            result,
            docDirPath,
            "third-party-licenses",
            _("Third-party licenses"));
    } else
        result.append(
            formatLink(_("License"), QUrl(aboutLicenseUrl)));

    return result;
}


static QString joinInLayoutDirection(
    const QString& separator, const QStringList& list)
{
    QString result;

    const auto rtl = QApplication::isRightToLeft();
    for (decltype(list.size()) i = 0; i < list.size(); ++i) {
        if (i > 0)
            result.insert(rtl ? 0 : result.size(), separator);

        result.insert(rtl ? 0 : result.size(), list[i]);
    }

    return result;
}


About::About(QWidget* parent)
    : QWidget{parent}
{
    const auto fontHeight = QFontMetrics(font()).height();

    auto* iconLabel = new QLabel();
    iconLabel->setAlignment(Qt::AlignCenter);
    iconLabel->setPixmap(
        getPixmap(QApplication::windowIcon(), fontHeight * 5));

    auto* infoTextLabel = new QLabel();
    infoTextLabel->setAlignment(Qt::AlignCenter);
    infoTextLabel->setTextFormat(Qt::RichText);
    infoTextLabel->setTextInteractionFlags(
        Qt::TextSelectableByMouse
        | Qt::LinksAccessibleByMouse
        | Qt::LinksAccessibleByKeyboard);
    infoTextLabel->setText(QString(
        "<p style=\"line-height: 140%;\">"
        "<b>%1 %2</b><br>"
        "%3<br>"
        "%4<br>"
        "%5</p>"
        ).arg(
            appName,
            appVersion,
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
            appCopyright));

    connect(
        infoTextLabel, SIGNAL(linkActivated(const QString&)),
        this, SLOT(handleLinkActivation(const QString&)));
    connect(
        infoTextLabel, SIGNAL(linkHovered(const QString&)),
        this, SLOT(handleLinkHover(const QString&)));

    textEdit = new QTextEdit();
    textEdit->setReadOnly(true);
    textEdit->setVisible(false);

    auto textEditFont = textEdit->currentFont();
    setMonospace(textEditFont);
    textEdit->setCurrentFont(textEditFont);

    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins({});

    layout->addWidget(iconLabel);
    layout->addWidget(infoTextLabel);
    layout->addWidget(textEdit, 1);
    layout->addStretch();
}


static QString getTextForLink(const QString& link)
{
    if (link == aboutLicenseUrl)
        return appLicense;

    const QUrl url(link);

    if (url.isLocalFile()
            && QFileInfo(url.toLocalFile()).suffix() == "txt") {
        QFile file(url.toLocalFile());
        if (file.open(QFile::ReadOnly | QFile::Text))
            return file.readAll();
    }

    return {};
}


void About::handleLinkActivation(const QString& link)
{
    if (link == currentTextLink) {
        textEdit->setVisible(!textEdit->isVisible());
        return;
    }

    const auto text = getTextForLink(link);
    if (!text.isEmpty()) {
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
