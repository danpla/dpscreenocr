#pragma once

#include "dpso/dpso.h"


#ifdef __cplusplus
extern "C" {
#endif


/**
 * Get URL of the JSON info file for dpsoOcrLangManagerCreate().
 *
 * Returns
 * "{uiAppWebsite}/ocr_engine_data/{ocrEngineInfo->id}_data.json".
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
