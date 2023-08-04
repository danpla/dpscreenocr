
#pragma once

#include "dpso/dpso.h"


#ifdef __cplusplus
extern "C" {
#endif


/**
 * Get URL of the JSON info file for dpsoOcrLangManagerCreate().
 *
 * The URL will point to a file "{ocrEngineInfo->id}_files.json" at
 * uiAppWebsite.
 */
const char* uiGetOcrDataInfoFileUrl(
    const DpsoOcrEngineInfo* ocrEngineInfo);


/**
 * Generate name for OCR data directory.
 *
 * If DpsoOcrEngineInfo::dataDirPreference is
 * DpsoOcrEngineDataDirPreferencePreferExplicit, returns
 * DpsoOcrEngineInfo::id followed by "_data". Otherwise, returns an
 * empty string.
 */
const char* uiGetOcrDataDirName(
    const DpsoOcrEngineInfo* ocrEngineInfo);


#ifdef __cplusplus
}
#endif
