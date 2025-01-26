#pragma once


namespace ui::qt::langManager {


// The installation and update are the same in terms of the language
// manager API. We use this enum when we need to parameterize widgets
// that work with installation and therefore only differ visually,
// e.g. showing different text depending on the mode.
enum class InstallMode {
    install,
    update
};


}
