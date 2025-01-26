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
 * The dpsoInit/Shutdown() pair is reference counted: each successful
 * dpsoInit() should have a corresponding dpsoShutdown().
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
 * etc., for hotkey handling (key_manager.h) and interactive selection
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
 * The constructor will call dpsoInit(). If the initialization
 * succeeds, the bool operator returns true and the destructor calls
 * dpsoSutdown() when object goes out of scope. If dpsoInit() fails,
 * the bool operator returns false, and the destructor does nothing.
 */
class DpsoInitializer {
public:
    DpsoInitializer();
    ~DpsoInitializer();

    DpsoInitializer(const DpsoInitializer&) = delete;
    DpsoInitializer& operator=(const DpsoInitializer&) = delete;

    DpsoInitializer(DpsoInitializer&& other) noexcept;
    DpsoInitializer& operator=(DpsoInitializer&& other) noexcept;

    explicit operator bool() const;
private:
    bool isActive;
};


}


#endif
