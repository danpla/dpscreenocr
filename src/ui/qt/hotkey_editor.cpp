
#include "hotkey_editor.h"

#include <QCheckBox>
#include <QComboBox>
#include <QHBoxLayout>

#include "dpso_intl/dpso_intl.h"


HotkeyEditor::HotkeyEditor(
        DpsoHotkeyAction action,
        bool hideNoneKey,
        QWidget* parent)
    : QWidget{parent}
    , action{action}
    , hideNoneKey{hideNoneKey}
    , modChecks{}
    , keyCombo{}
{
    auto* layout = new QHBoxLayout(this);
    layout->setContentsMargins({});

    DpsoHotkey hotkey;
    dpsoFindActionHotkey(action, &hotkey);

    const auto keySelected = hotkey.key != dpsoUnknownKey;

    for (int i = 0; i < dpsoNumKeyMods; ++i) {
        const auto mod = dpsoGetKeyModAt(i);

        const DpsoHotkey modHotkey{dpsoUnknownKey, mod};
        auto* modCheck = new QCheckBox(
            dpsoHotkeyToString(&modHotkey));
        modChecks[i] = modCheck;

        modCheck->setEnabled(keySelected);
        modCheck->setChecked(keySelected && (hotkey.mods & mod));

        connect(
            modCheck, SIGNAL(stateChanged(int)),
            this, SIGNAL(changed()));

        layout->addWidget(modCheck);
    }

    keyCombo = new QComboBox();
    layout->addWidget(keyCombo, 1);

    for (int i = dpsoUnknownKey; i < dpsoNumKeys; ++i) {
        const auto key = static_cast<DpsoKey>(i);
        if (key == dpsoUnknownKey && hideNoneKey)
            continue;

        const DpsoHotkey keyHotkey{key, dpsoKeyModNone};
        keyCombo->addItem(dpsoHotkeyToString(&keyHotkey), key);

        if (key == hotkey.key)
            keyCombo->setCurrentIndex(keyCombo->count() - 1);
    }

    connect(
        keyCombo, SIGNAL(currentIndexChanged(int)),
        this, SLOT(keyChanged()));
}


void HotkeyEditor::assignHotkey(bool emitChanged)
{
    keyCombo->blockSignals(true);
    for (auto* modCheck : modChecks)
        modCheck->blockSignals(true);

    DpsoHotkey hotkey;
    dpsoFindActionHotkey(action, &hotkey);

    auto hotkeyChanged = false;

    if (hotkey.key != getCurrentKey()) {
        keyCombo->setCurrentIndex(keyCombo->findData(hotkey.key));
        hotkeyChanged = true;
    }

    const auto keySelected = getCurrentKey() != dpsoUnknownKey;

    for (int i = 0; i < dpsoNumKeyMods; ++i) {
        const auto mod = dpsoGetKeyModAt(i);
        auto* modCheck = modChecks[i];

        modCheck->setEnabled(keySelected);

        const auto newChecked = keySelected && (hotkey.mods & mod);
        if (modCheck->isChecked() != newChecked) {
            hotkeyChanged = true;
            modCheck->setChecked(newChecked);
        }
    }

    keyCombo->blockSignals(false);
    for (auto* modCheck : modChecks)
        modCheck->blockSignals(false);

    if (emitChanged && hotkeyChanged)
        emit changed();
}


void HotkeyEditor::bind()
{
    DpsoHotkey hotkey{getCurrentKey(), dpsoKeyModNone};

    for (int i = 0; i < dpsoNumKeyMods; ++i)
        if (modChecks[i]->isChecked())
            hotkey.mods |= dpsoGetKeyModAt(i);

    dpsoUnbindAction(action);
    dpsoBindHotkey(&hotkey, action);
}


void HotkeyEditor::keyChanged()
{
    const auto keySelected = getCurrentKey() != dpsoUnknownKey;

    // When the key is switched to dpsoUnknownKey, we need to uncheck
    // every modifier checkbox without emitting changed().
    for (auto* modCheck : modChecks) {
        modCheck->blockSignals(true);

        modCheck->setEnabled(keySelected);
        if (!keySelected)
            modCheck->setChecked(false);

        modCheck->blockSignals(false);
    }

    emit changed();
}


DpsoKey HotkeyEditor::getCurrentKey() const
{
    const auto curIdx = keyCombo->currentIndex();
    if (curIdx < 0)
        return dpsoUnknownKey;

    return static_cast<DpsoKey>(keyCombo->itemData(curIdx).toInt());
}
