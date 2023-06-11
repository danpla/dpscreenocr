
/**
 * \file
 * Main header + library initialization, shutdown, and update
 */

#pragma once

#include <stdbool.h>

#include "key_manager.h"
#include "ocr.h"
#include "ocr_engine.h"
#include "ocr_lang_manager.h"
#include "selection.h"


#ifdef __cplusplus
extern "C" {
#endif


/**
 * Initialize dpso library.
 *
 * On failure, sets an error message (dpsoGetError()) and returns
 * false.
 */
bool dpsoInit(void);


/**
 * Shut down dpso library.
 */
void dpsoShutdown(void);


/**
 * Update the library.
 *
 * dpsoUpdate() process system events, like mouse motion, key press,
 * etc., for hotkeys handling (hotkeys.h) and interactive selection
 * (selection.h). Call this function at a frequency close to the
 * monitor refresh rate, which is usually 60 times per second.
 */
void dpsoUpdate(void);


#ifdef __cplusplus
}


#include <memory>


namespace dpso {


/**
 * RAII for dpsoInit/Shutdown().
 *
 * DpsoInitializer::init() calls dpsoInit(). If the initialization
 * succeeds, the bool operator of DpsoInitializer returns true and its
 * destructor calls dpsoSutdown() once object goes out of scope. If
 * dpsoInit() fails, the bool operator returns false, and the
 * destructor does nothing.
 */
class DpsoInitializer {
public:
    static DpsoInitializer init();

    DpsoInitializer();
    ~DpsoInitializer();

    DpsoInitializer(const DpsoInitializer& other) = delete;
    DpsoInitializer& operator=(const DpsoInitializer& other) = delete;

    DpsoInitializer(DpsoInitializer&& other) noexcept;
    DpsoInitializer& operator=(DpsoInitializer&& other) noexcept;

    explicit operator bool() const;
private:
    explicit DpsoInitializer(bool isActive);

    bool isActive;
};


}


#endif
