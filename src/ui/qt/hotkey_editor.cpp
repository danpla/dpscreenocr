
#include "hotkey_editor.h"

#include <QBoxLayout>

#include "dpso_intl/dpso_intl.h"


HotkeyEditor::HotkeyEditor(
        DpsoHotkeyAction action,
        const QVector<DpsoKey>& hiddenKeys,
        Qt::Orientation orientation,
        QWidget* parent)
    : QWidget(parent)
    , action {action}
    , keyCombo {}
    , modCheckBoxes {}
{
    QBoxLayout::Direction layoutDir;
    if (orientation == Qt::Horizontal)
        layoutDir = QBoxLayout::LeftToRight;
    else
        layoutDir = QBoxLayout::TopToBottom;

    auto* layout = new QBoxLayout(layoutDir, this);
    layout->setContentsMargins({});

    keyCombo = new QComboBox();
    layout->addWidget(
        keyCombo, orientation == Qt::Horizontal ? 1 : 0);

    // To avoid an extra iteration over keyCombo items, we set
    // setCurrentIndex() here manually instead of calling
    // assignHotkey() .
    DpsoHotkey hotkey;
    dpsoFindActionHotkey(action, &hotkey);

    int keyComboIdx = 0;
    for (int i = dpsoUnknownKey; i < dpsoNumKeys; ++i) {
        const DpsoHotkey keyHotkey {
            static_cast<DpsoKey>(i), dpsoKeyModNone};

        // We use QVector and linear search assuming the hiddenKeys
        // list is relatively small.
        if (hiddenKeys.contains(keyHotkey.key))
            continue;

        const char* keyName;
        if (keyHotkey.key == dpsoUnknownKey)
            keyName = pgettext("hotkey.key", "None");
        else
            keyName = dpsoHotkeyToString(&keyHotkey);
        keyCombo->addItem(keyName, keyHotkey.key);

        if (keyHotkey.key == hotkey.key)
            keyCombo->setCurrentIndex(keyComboIdx);
        ++keyComboIdx;
    }

    connect(
        keyCombo, SIGNAL(currentIndexChanged(int)),
        this, SLOT(keyChanged()));

    auto* modsLayout = new QHBoxLayout();
    layout->addLayout(modsLayout);

    const auto keySelected = getCurrentKey() != dpsoUnknownKey;

    for (int i = 0; i < dpsoNumKeyMods; ++i) {
        const auto mod = dpsoGetKeyModAt(i);

        const DpsoHotkey modHotkey {dpsoUnknownKey, mod};
        auto* modCheckBox = new QCheckBox(
            dpsoHotkeyToString(&modHotkey));
        modCheckBoxes[i] = modCheckBox;

        modCheckBox->setEnabled(keySelected);
        modCheckBox->setChecked(keySelected && (hotkey.mods & mod));

        connect(
            modCheckBox, SIGNAL(stateChanged(int)),
            this, SIGNAL(changed()));

        modsLayout->addWidget(modCheckBox);
    }

    if (orientation == Qt::Vertical)
        modsLayout->addStretch();
}


void HotkeyEditor::assignHotkey(bool emitChanged)
{
    keyCombo->blockSignals(true);
    for (auto* modCheckBox : modCheckBoxes)
        modCheckBox->blockSignals(true);

    DpsoHotkey hotkey;
    dpsoFindActionHotkey(action, &hotkey);

    bool hotkeyChanged = false;

    if (hotkey.key != getCurrentKey())
        for (int i = 0; i < keyCombo->count(); ++i)
            if (keyCombo->itemData(i) == hotkey.key) {
                keyCombo->setCurrentIndex(i);
                hotkeyChanged = true;
                break;
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
    DpsoHotkey hotkey {getCurrentKey(), dpsoKeyModNone};

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
