
#include "hotkey_editor.h"

#include <QCheckBox>
#include <QComboBox>
#include <QHBoxLayout>


HotkeyEditor::HotkeyEditor(
        DpsoHotkeyAction action,
        bool hideNoKey,
        QWidget* parent)
    : QWidget{parent}
    , action{action}
    , modChecks{}
    , keyCombo{}
{
    auto* layout = new QHBoxLayout(this);
    layout->setContentsMargins({});

    DpsoHotkey hotkey;
    dpsoFindActionHotkey(action, &hotkey);

    const auto keySelected = hotkey.key != dpsoNoKey;

    for (int i = 0; i < dpsoNumKeyMods; ++i) {
        const auto mod = dpsoGetKeyModAt(i);

        const DpsoHotkey modHotkey{dpsoNoKey, mod};
        auto* modCheck = new QCheckBox(
            dpsoHotkeyToString(&modHotkey));
        modChecks[i] = modCheck;

        modCheck->setEnabled(keySelected);
        modCheck->setChecked(keySelected && (hotkey.mods & mod));

        connect(
            modCheck, &QCheckBox::stateChanged,
            this, &HotkeyEditor::changed);

        layout->addWidget(modCheck);
    }

    keyCombo = new QComboBox();
    layout->addWidget(keyCombo, 1);

    for (int i = dpsoNoKey; i < dpsoNumKeys; ++i) {
        const auto key = static_cast<DpsoKey>(i);
        if (key == dpsoNoKey && hideNoKey)
            continue;

        const DpsoHotkey keyHotkey{key, dpsoNoKeyMods};
        keyCombo->addItem(dpsoHotkeyToString(&keyHotkey), key);

        if (key == hotkey.key)
            keyCombo->setCurrentIndex(keyCombo->count() - 1);
    }

    connect(
        keyCombo, qOverload<int>(&QComboBox::currentIndexChanged),
        this, &HotkeyEditor::keyChanged);
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

    const auto keySelected = getCurrentKey() != dpsoNoKey;

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
    DpsoHotkey hotkey{getCurrentKey(), dpsoNoKeyMods};

    for (int i = 0; i < dpsoNumKeyMods; ++i)
        if (modChecks[i]->isChecked())
            hotkey.mods |= dpsoGetKeyModAt(i);

    dpsoUnbindAction(action);
    dpsoBindHotkey(&hotkey, action);
}


void HotkeyEditor::keyChanged()
{
    const auto keySelected = getCurrentKey() != dpsoNoKey;

    // When the key is switched to dpsoNoKey, we need to uncheck
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
        return dpsoNoKey;

    return static_cast<DpsoKey>(keyCombo->itemData(curIdx).toInt());
}
