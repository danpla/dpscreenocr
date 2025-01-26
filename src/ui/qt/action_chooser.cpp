#include "action_chooser.h"

#include <QCheckBox>
#include <QDir>
#include <QFileDialog>
#include <QFileInfo>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QStyle>
#include <QToolButton>
#include <QVBoxLayout>

#include "dpso_intl/dpso_intl.h"
#include "ui_common/ui_common.h"


#define _(S) gettext(S)


namespace ui::qt {


ActionChooser::ActionChooser()
{
    copyToClipboardCheck = new QCheckBox(_("Copy text to clipboard"));
    addToHistoryCheck = new QCheckBox(_("Add text to history"));

    runExeCheck = new QCheckBox(_("Run executable:"));
    runExeCheck->setToolTip(
        _("Run an executable with the recognized text as the "
            "first argument"));
    exeLineEdit = new QLineEdit();
    exeLineEdit->setEnabled(false);
    connect(
        exeLineEdit, &QLineEdit::textChanged,
        [&](const QString& text)
        {
            exePath = text.trimmed().toUtf8();
        });

    auto* selectExeButton = new QToolButton();
    selectExeButton->setIcon(
        selectExeButton->style()->standardIcon(
            QStyle::SP_DialogOpenButton, nullptr, selectExeButton));
    selectExeButton->setEnabled(false);
    connect(
        selectExeButton, &QToolButton::clicked,
        this, &ActionChooser::chooseExe);

    connect(
        runExeCheck, &QCheckBox::toggled,
        exeLineEdit, &QLineEdit::setEnabled);
    connect(
        runExeCheck, &QCheckBox::toggled,
        selectExeButton, &QToolButton::setEnabled);

    connect(
        copyToClipboardCheck, &QCheckBox::toggled,
        this, &ActionChooser::actionsChanged);
    connect(
        addToHistoryCheck, &QCheckBox::toggled,
        this, &ActionChooser::actionsChanged);
    connect(
        runExeCheck, &QCheckBox::toggled,
        this, &ActionChooser::actionsChanged);

    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins({});

    layout->addWidget(copyToClipboardCheck);
    layout->addWidget(addToHistoryCheck);

    auto* execLayout = new QHBoxLayout();
    execLayout->addWidget(runExeCheck);
    execLayout->addWidget(exeLineEdit, 1);
    execLayout->addWidget(selectExeButton);
    layout->addLayout(execLayout);
}


ActionChooser::Actions ActionChooser::getSelectedActions() const
{
    Actions actions = Action::none;

    if (copyToClipboardCheck->isChecked())
        actions |= Action::copyToClipboard;

    if (addToHistoryCheck->isChecked())
        actions |= Action::addToHistory;

    if (runExeCheck->isChecked())
        actions |= Action::runExe;

    return actions;
}


const char* ActionChooser::getExePath() const
{
    return exePath.data();
}


void ActionChooser::loadState(const DpsoCfg* cfg)
{
    copyToClipboardCheck->setChecked(
        dpsoCfgGetBool(
            cfg,
            cfgKeyActionCopyToClipboard,
            cfgDefaultValueActionCopyToClipboard));
    addToHistoryCheck->setChecked(
        dpsoCfgGetBool(
            cfg,
            cfgKeyActionAddToHistory,
            cfgDefaultValueActionAddToHistory));
    runExeCheck->setChecked(
        dpsoCfgGetBool(
            cfg,
            cfgKeyActionRunExecutable,
            cfgDefaultValueActionRunExecutable));
    exeLineEdit->setText(
        dpsoCfgGetStr(cfg, cfgKeyActionRunExecutablePath, ""));
}


void ActionChooser::saveState(DpsoCfg* cfg) const
{
    dpsoCfgSetBool(
        cfg,
        cfgKeyActionCopyToClipboard,
        copyToClipboardCheck->isChecked());
    dpsoCfgSetBool(
        cfg,
        cfgKeyActionAddToHistory,
        addToHistoryCheck->isChecked());
    dpsoCfgSetBool(
        cfg,
        cfgKeyActionRunExecutable,
        runExeCheck->isChecked());
    dpsoCfgSetStr(
        cfg, cfgKeyActionRunExecutablePath, exePath.data());
}


void ActionChooser::chooseExe()
{
    auto exeDir = QFileInfo(
        QDir::fromNativeSeparators(exePath)).dir().path();
    if (exeDir == "." || !QDir(exeDir).exists())
        // exeDir is '.' when QDir is constructed with an empty
        // string. This happens if the exe path is either empty or
        // contains just an exe name.
        exeDir = QDir::homePath();

    const auto path = QFileDialog::getOpenFileName(
        this, _("Choose an executable"), exeDir);
    if (!path.isEmpty())
        exeLineEdit->setText(QDir::toNativeSeparators(path));
}


}
