
#pragma once

#include <stdio.h>

#include "history.h"


#ifdef __cplusplus
extern "C" {
#endif


void dpsoHistoryLoadFp(FILE* fp);
void dpsoHistorySaveFp(FILE* fp);


#ifdef __cplusplus
}
#endif
