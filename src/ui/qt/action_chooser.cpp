
#include "action_chooser.h"

#include <QCheckBox>
#include <QDir>
#include <QFileDialog>
#include <QFileInfo>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QToolButton>
#include <QVBoxLayout>

#include "dpso_intl/dpso_intl.h"
#include "ui_common/ui_common.h"


#define _(S) gettext(S)


ActionChooser::ActionChooser(QWidget* parent)
    : QWidget{parent}
{
    copyToClipboardCheck = new QCheckBox(_("Copy text to clipboard"));
    addToHistoryCheck = new QCheckBox(_("Add text to history"));

    runExeCheck = new QCheckBox(_("Run executable:"));
    runExeCheck->setToolTip(
        _("Run an executable with the recognized text as the "
            "first argument"));
    exeLineEdit = new QLineEdit();
    exeLineEdit->setEnabled(false);

    auto* selectExeButton = new QToolButton();
    selectExeButton->setText("\342\200\246");
    selectExeButton->setEnabled(false);
    connect(
        selectExeButton, SIGNAL(clicked()),
        this, SLOT(chooseExe()));

    connect(
        runExeCheck, SIGNAL(toggled(bool)),
        exeLineEdit, SLOT(setEnabled(bool)));
    connect(
        runExeCheck, SIGNAL(toggled(bool)),
        selectExeButton, SLOT(setEnabled(bool)));

    connect(
        copyToClipboardCheck, SIGNAL(toggled(bool)),
        this, SIGNAL(actionsChanged()));
    connect(
        addToHistoryCheck, SIGNAL(toggled(bool)),
        this, SIGNAL(actionsChanged()));
    connect(
        runExeCheck, SIGNAL(toggled(bool)),
        this, SIGNAL(actionsChanged()));

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


QString ActionChooser::getExePath() const
{
    return exeLineEdit->text();
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
        cfg,
        cfgKeyActionRunExecutablePath,
        exeLineEdit->text().toUtf8().data());
}


void ActionChooser::chooseExe()
{
    auto exePath = QDir::fromNativeSeparators(
        exeLineEdit->text().trimmed());

    auto exeDir = QFileInfo(exePath).dir().path();
    if (exeDir == "." || !QDir(exeDir).exists())
        // exeDir is '.' when QDir is constructed with an empty
        // string. This happens if the line edit is either empty or
        // contains just an exe name.
        exeDir = QDir::homePath();

    exePath = QFileDialog::getOpenFileName(
        this, _("Choose an executable"), exeDir);
    if (exePath.isEmpty())
        return;

    exeLineEdit->setText(QDir::toNativeSeparators(exePath));
}
