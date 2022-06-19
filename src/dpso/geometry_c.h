
#pragma once


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
int dpsoRectIsEmpty(const DpsoRect* rect);


#ifdef __cplusplus
}
#endif
