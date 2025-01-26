#include "about.h"

#include <initializer_list>

#include <QApplication>
#include <QCursor>
#include <QDesktopServices>
#include <QDir>
#include <QFileInfo>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
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

    appendFileLink(result, docDirPath, "changelog.txt", _("Changes"));

    result.append(formatLink(_("License"), QUrl(aboutLicenseUrl)));

    appendFileLink(
        result,
        docDirPath,
        "third-party-licenses",
        _("Third-party licenses"));

    return result;
}


About::About()
{
    const auto fontHeight = fontMetrics().height();

    auto* iconLabel = new QLabel();
    iconLabel->setPixmap(
        getIcon(uiIconNameApp).pixmap(fontHeight * 4));

    auto* infoTextLabel = new QLabel();
    infoTextLabel->setTextFormat(Qt::RichText);
    infoTextLabel->setTextInteractionFlags(
        Qt::LinksAccessibleByMouse | Qt::LinksAccessibleByKeyboard);
    infoTextLabel->setText(QString(
        "<p style=\"line-height: 130%;\">"
        "<b>%1</b><br>"
        "%2<br>"
        "%3<br>"
        "%4</p>"
        ).arg(
            joinInLayoutDirection(" ", {uiAppName, uiAppVersion}),
            _("Program to recognize text on screen"),
            createLinks().join("<br>"),
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

    auto* checkUpdatesButton = new QPushButton(
        _("Check for updates"));
    checkUpdatesButton->setVisible(uiUpdateCheckerIsAvailable());
    connect(
        checkUpdatesButton, &QPushButton::clicked,
        this, &About::checkUpdates);

    auto* buttonsLayout = new QHBoxLayout();
    buttonsLayout->addWidget(checkUpdatesButton);
    buttonsLayout->addStretch();

    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins({});

    layout->addWidget(iconLabel);
    layout->addWidget(infoTextLabel);
    layout->addWidget(textEdit, 1);
    layout->addStretch();
    layout->addLayout(buttonsLayout);
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

    QToolTip::showText(
        QCursor::pos(),
        url.isLocalFile()
            ? QDir::toNativeSeparators(url.toLocalFile()) : link);
}


}
