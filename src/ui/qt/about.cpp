
#include "about.h"

#include <QApplication>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QTextEdit>
#include <QVBoxLayout>

#include "dpso_intl/dpso_intl.h"
#define _(S) gettext(S)

#include "common/common.h"

#include "utils.h"


About::About(QWidget* parent)
    : QWidget(parent)
{
    // The icon size and layout margins depend on the font size of
    // the app name label. If the SVG module is not installed, Qt
    // will choose a raster icon <= the requested size.
    auto* nameLabel = new QLabel(appName);
    nameLabel->setAlignment(Qt::AlignCenter);
    nameLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);

    auto nameFont = nameLabel->font();
    nameFont.setBold(true);
    nameFont.setPointSize(nameFont.pointSize() * 2);
    nameLabel->setFont(nameFont);

    const auto nameFontHeight = QFontMetrics(nameFont).height();

    auto* iconLabel = new QLabel();
    iconLabel->setAlignment(Qt::AlignCenter);
    iconLabel->setPixmap(
        QApplication::windowIcon().pixmap(nameFontHeight * 3));

    auto* infoTextLabel = new QLabel();
    infoTextLabel->setAlignment(Qt::AlignCenter);
    infoTextLabel->setOpenExternalLinks(true);
    infoTextLabel->setTextFormat(Qt::RichText);
    infoTextLabel->setTextInteractionFlags(
        Qt::TextSelectableByMouse | Qt::LinksAccessibleByMouse);
    infoTextLabel->setText(QString(
        "%1<br><br>"
        "%2<br><br>"
        "<a href=\"%3\">%3</a><br><br>"
        "%4").arg(
            appVersion,
            _("Program to recognize text on screen"),
            appWebsite,
            appCopyright));

    auto* licenseTextEdit = new QTextEdit();
    licenseTextEdit->setReadOnly(true);

    auto font = licenseTextEdit->currentFont();
    setMonospace(font);
    licenseTextEdit->setCurrentFont(font);

    licenseTextEdit->setText(appLicense);
    licenseTextEdit->setVisible(false);

    auto* licenseButton = new QPushButton(_("License"));
    licenseButton->setCheckable(true);
    connect(
        licenseButton, SIGNAL(toggled(bool)),
        infoTextLabel, SLOT(setHidden(bool)));
    connect(
        licenseButton, SIGNAL(toggled(bool)),
        licenseTextEdit, SLOT(setVisible(bool)));

    auto* layout = new QVBoxLayout(this);

    QMargins margins;
    margins.setTop(nameFontHeight);
    layout->setContentsMargins(margins);

    layout->addWidget(iconLabel);
    layout->addWidget(nameLabel);
    layout->addWidget(infoTextLabel);
    layout->addWidget(licenseTextEdit, 1);
    layout->addStretch();

    auto* buttonsLayout = new QHBoxLayout();
    buttonsLayout->addWidget(licenseButton);
    buttonsLayout->addStretch();

    layout->addLayout(buttonsLayout);
}
