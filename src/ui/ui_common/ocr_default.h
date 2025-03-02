#pragma once

#include "dpso_ocr/dpso_ocr.h"


#ifdef __cplusplus
extern "C" {
#endif


DpsoOcr* dpsoOcrCreateDefault(int engineIdx);
DpsoOcrLangManager* dpsoOcrLangManagerCreateDefault(int engineIdx);


#ifdef __cplusplus
}
#endif
