
#pragma once


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
const char* getOcrDataDirName(
    const struct DpsoOcrEngineInfo* ocrEngineInfo);


#ifdef __cplusplus
}
#endif
