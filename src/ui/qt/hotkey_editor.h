
#pragma once

#include <QWidget>

#include "dpso/dpso.h"


class QCheckBox;
class QComboBox;


namespace ui::qt {


/**
 * Hotkey editor.
 *
 * Keep in mind that the editor doesn't rebind hotkey automatically.
 * If you need this, connect changed() signal to bind() slot.
 */
class HotkeyEditor : public QWidget {
    Q_OBJECT
public:
    /**
     * HtokeyEditor
     *
     * \param action Hotkey action. HotkeyEditor doesn't treat actions
     *     < 0 especially.
     * \param hideNoKey Hide dpsoNoKey from the key combo box so that
     *     the user cannot explicitly disable the hotkey.
     */
    explicit HotkeyEditor(
        DpsoHotkeyAction action,
        bool hideNoKey = true);
signals:
    /**
     * Emitted every time the editor is changed.
     *
     * Keep in mind that this only refers to the editor's state;
     * bind() should be called explicitly to apply the changes.
     */
    void changed();
public slots:
    /**
     * Assign the current hotkey of the action to the editor.
     *
     * This method can be used if the hotkey was changed outside
     * the editor. It's automatically called from the constructor.
     */
    void assignHotkey(bool emitChanged = false);

    /**
     * Bind the current hotkey.
     *
     * Calls dpsoKeyManagerUnbindAction() and
     * dpsoKeyManagerBindHotkey().
     */
    void bind();
private slots:
    void keyChanged();
private:
    DpsoHotkeyAction action;
    QCheckBox* modChecks[dpsoNumKeyMods];
    QComboBox* keyCombo;

    DpsoKey getCurrentKey() const;
};


}
