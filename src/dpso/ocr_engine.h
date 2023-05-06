
#pragma once

#include <stdbool.h>


#ifdef __cplusplus
extern "C" {
#endif


/**
 * Get the number of available OCR engines.
 */
int dpsoOcrGetNumEngines(void);


/**
 * What DpsoOcrArgs::dataDir the OCR engine prefers.
 */
typedef enum {
    /**
     * Engine doesn't use external data.
     *
     * DpsoOcrArgs::dataDir is ignored.
     */
    DpsoOcrEngineDataDirPreferenceNoDataDir,

    /**
     * Engine prefers the default data directory.
     *
     * Usually this means that on the current platform, the data is
     * installed in a system-wide path that is hardcoded into the OCR
     * library at build time.
     */
    DpsoOcrEngineDataDirPreferencePreferDefault,

    /**
     * Engine prefers an explicit path to data directory.
     *
     * This is normally the default mode for non-Unix platforms, where
     * the data is usually installed in the application-specific
     * directory.
     */
    DpsoOcrEngineDataDirPreferencePreferExplicit
} DpsoOcrEngineDataDirPreference;


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

    DpsoOcrEngineDataDirPreference dataDirPreference;

    /**
     * Whether the engine has a language manager.
     *
     * If hasLangManager is false, dpsoOcrLangManagerCreate() is
     * guaranteed to fail. This field is always false for any data
     * directory preference other than
     * DpsoOcrEngineDataDirPreferencePreferExplicit.
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
