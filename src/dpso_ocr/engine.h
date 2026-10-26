#pragma once

#include <stdbool.h>


#ifdef __cplusplus
extern "C" {
#endif


/**
 * Get the number of available OCR engines.
 */
int dpsoOcrGetNumEngines(void);


typedef struct DpsoOcrEngineInfo {
    /**
     * Unique engine id.
     *
     * An id consists of lower-case ASCII alphabetical letters,
     * numbers, and underscores to separate words.
     */
    const char* id;

    /**
     * Engine name.
     *
     * This is a readable engine name, which, unlike id, doesn't have
     * any restrictions.
     */
    const char* name;

    /**
     * Engine version.
     *
     * If the OCR engine library is linked dynamically, this will
     * normally be the runtime version. May be empty if the engine
     * doesn't provide version information.
     */
    const char* version;

    /**
     * Whether the engine has a language manager.
     *
     * If hasLangManager is false, dpsoOcrLangManagerCreate() is
     * guaranteed to fail.
     */
    bool hasLangManager;
} DpsoOcrEngineInfo;


/**
 * Get OCR engine info.
 *
 * Does nothing if the index is out of [0, dpsoOcrGetNumEngines()).
 */
void dpsoOcrGetEngineInfo(int idx, DpsoOcrEngineInfo* info);


#ifdef __cplusplus
}
#endif
