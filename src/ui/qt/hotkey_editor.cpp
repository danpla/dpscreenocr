
#include "hotkey_editor.h"

#include <cassert>

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
    , keyCombo{}
    , modCheckBoxes{}
{
    auto* layout = new QHBoxLayout(this);
    layout->setContentsMargins({});

    keyCombo = new QComboBox();
    layout->addWidget(keyCombo, 1);

    DpsoHotkey hotkey;
    dpsoFindActionHotkey(action, &hotkey);

    for (int i = dpsoUnknownKey; i < dpsoNumKeys; ++i) {
        const auto key = static_cast<DpsoKey>(i);
        if (key == dpsoUnknownKey && hideNoneKey)
            continue;

        const char* keyName;
        if (key == dpsoUnknownKey)
            keyName = pgettext("hotkey.key", "None");
        else {
            const DpsoHotkey keyHotkey{key, dpsoKeyModNone};
            keyName = dpsoHotkeyToString(&keyHotkey);
        }

        keyCombo->addItem(keyName, key);

        if (key == hotkey.key)
            keyCombo->setCurrentIndex(keyCombo->count() - 1);
    }

    connect(
        keyCombo, SIGNAL(currentIndexChanged(int)),
        this, SLOT(keyChanged()));

    const auto keySelected = getCurrentKey() != dpsoUnknownKey;

    for (int i = 0; i < dpsoNumKeyMods; ++i) {
        const auto mod = dpsoGetKeyModAt(i);

        const DpsoHotkey modHotkey{dpsoUnknownKey, mod};
        auto* modCheckBox = new QCheckBox(
            dpsoHotkeyToString(&modHotkey));
        modCheckBoxes[i] = modCheckBox;

        modCheckBox->setEnabled(keySelected);
        modCheckBox->setChecked(keySelected && (hotkey.mods & mod));

        connect(
            modCheckBox, SIGNAL(stateChanged(int)),
            this, SIGNAL(changed()));

        layout->addWidget(modCheckBox);
    }
}


void HotkeyEditor::assignHotkey(bool emitChanged)
{
    keyCombo->blockSignals(true);
    for (auto* modCheckBox : modCheckBoxes)
        modCheckBox->blockSignals(true);

    DpsoHotkey hotkey;
    dpsoFindActionHotkey(action, &hotkey);

    bool hotkeyChanged = false;

    if (hotkey.key != getCurrentKey()) {
        // If hideNoneKey is true, we pass dpsoUnknownKey (-1) as is
        // so that keyCombo becomes unselected.
        const auto idx = hotkey.key + !hideNoneKey;
        assert(
            idx == -1
            || (idx >=0
                && idx < keyCombo->count()
                && keyCombo->itemData(idx).toInt() == hotkey.key));
        keyCombo->setCurrentIndex(idx);
        hotkeyChanged = true;
    }

    const auto keySelected = getCurrentKey() != dpsoUnknownKey;

    for (int i = 0; i < dpsoNumKeyMods; ++i) {
        const auto mod = dpsoGetKeyModAt(i);
        auto* modCheckBox = modCheckBoxes[i];

        modCheckBox->setEnabled(keySelected);

        const auto newChecked = keySelected && (hotkey.mods & mod);
        if (modCheckBox->isChecked() != newChecked) {
            hotkeyChanged = true;
            modCheckBox->setChecked(newChecked);
        }
    }

    keyCombo->blockSignals(false);
    for (auto* modCheckBox : modCheckBoxes)
        modCheckBox->blockSignals(false);

    if (emitChanged && hotkeyChanged)
        emit changed();
}


void HotkeyEditor::bind()
{
    DpsoHotkey hotkey{getCurrentKey(), dpsoKeyModNone};

    for (int i = 0; i < dpsoNumKeyMods; ++i)
        if (modCheckBoxes[i]->isChecked())
            hotkey.mods |= dpsoGetKeyModAt(i);

    dpsoUnbindAction(action);
    dpsoBindHotkey(&hotkey, action);
}


void HotkeyEditor::keyChanged()
{
    const auto keySelected = getCurrentKey() != dpsoUnknownKey;

    // When the key is switched to dpsoUnknownKey, we need to uncheck
    // every modifier checkbox without emitting changed().
    for (auto* modCheckBox : modCheckBoxes) {
        modCheckBox->blockSignals(true);

        modCheckBox->setEnabled(keySelected);
        if (!keySelected)
            modCheckBox->setChecked(false);

        modCheckBox->blockSignals(false);
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
