
#include "action_chooser.h"

#include <QDir>
#include <QFileDialog>
#include <QFileInfo>
#include <QHBoxLayout>
#include <QPushButton>
#include <QVBoxLayout>

#include "dpso_intl/dpso_intl.h"
#include "dpso_utils/dpso_utils.h"

#include "common/common.h"


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

    auto* selectExeButton = new QPushButton("\342\200\246");
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


ActionChooser::DynamicStrings::DynamicStrings()
    : chooseExeDialogTitle{_("Choose an executable")}
{

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


void ActionChooser::loadState()
{
    copyToClipboardCheck->setChecked(
        dpsoCfgGetBool(
            cfgKeyActionCopyToClipboard,
            cfgDefaultValueActionCopyToClipboard));
    addToHistoryCheck->setChecked(
        dpsoCfgGetBool(
            cfgKeyActionAddToHistory,
            cfgDefaultValueActionAddToHistory));
    runExeCheck->setChecked(
        dpsoCfgGetBool(
            cfgKeyActionRunExecutable,
            cfgDefaultValueActionRunExecutable));
    exeLineEdit->setText(
        dpsoCfgGetStr(cfgKeyActionRunExecutablePath, ""));
}


void ActionChooser::saveState() const
{
    dpsoCfgSetBool(
        cfgKeyActionCopyToClipboard,
        copyToClipboardCheck->isChecked());
    dpsoCfgSetBool(
        cfgKeyActionAddToHistory,
        addToHistoryCheck->isChecked());
    dpsoCfgSetBool(
        cfgKeyActionRunExecutable,
        runExeCheck->isChecked());
    dpsoCfgSetStr(
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

    QFileDialog::Options options;
    if (!dpsoCfgGetBool(
            cfgKeyUiNativeFileDialogs,
            cfgDefaultValueUiNativeFileDialogs))
        options |= QFileDialog::DontUseNativeDialog;

    exePath = QFileDialog::getOpenFileName(
        this,
        dynStr.chooseExeDialogTitle, exeDir, "", nullptr, options);
    if (exePath.isEmpty())
        return;

    exeLineEdit->setText(QDir::toNativeSeparators(exePath));
}
