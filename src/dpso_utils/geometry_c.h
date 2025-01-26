#pragma once

#include <stdbool.h>


#ifdef __cplusplus
extern "C" {
#endif


typedef struct DpsoRect {
    int x;
    int y;
    int w;
    int h;
} DpsoRect;


/**
 * Rect is empty if w or h is <= 0.
 */
bool dpsoRectIsEmpty(const DpsoRect* rect);


#ifdef __cplusplus
}
#endif
