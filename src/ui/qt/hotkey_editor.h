
#pragma once

#include <QCheckBox>
#include <QComboBox>
#include <QVector>
#include <QWidget>

#include "dpso/dpso.h"


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
     * \param hiddenKeys The list of keys to be excluded from the key
     *     combo box.
     * \param orientation Orientation specifies where to put the list
     *     of key modifier checkboxes relative to the key combo box:
     *     on the right of the combo box if Qt::Horizontal, or below
     *     it if Qt::Vertical.
     */
    explicit HotkeyEditor(
        DpsoHotkeyAction action,
        const QVector<DpsoKey>& hiddenKeys = {},
        Qt::Orientation orientation = Qt::Horizontal,
        QWidget* parent = nullptr);
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
     * Calls dpsoUnbindAction() and dpsoBindHotkey().
     */
    void bind();
private slots:
    void keyChanged();
private:
    DpsoHotkeyAction action;

    QComboBox* keyCombo;
    QCheckBox* modCheckBoxes[dpsoNumKeyMods];

    DpsoKey getCurrentKey() const;
};
