
#pragma once

#include "dpso/dpso.h"


#ifdef __cplusplus
extern "C" {
#endif


/**
 * Generate name for OCR data directory.
 *
 * If DpsoOcrEngineInfo::dataDirPreference is
 * DpsoOcrEngineDataDirPreferencePreferExplicit, returns
 * DpsoOcrEngineInfo::id followed by "_data". Otherwise, returns null.
 */
const char* uiGetOcrDataDirName(
    const DpsoOcrEngineInfo* ocrEngineInfo);


#ifdef __cplusplus
}
#endif
