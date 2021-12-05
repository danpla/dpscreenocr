
#pragma once


#ifdef __cplusplus
extern "C" {
#endif


struct DpsoRect {
    int x;
    int y;
    int w;
    int h;
};


/**
 * Rect is empty if w or h is <= 0.
 */
int dpsoRectIsEmpty(const struct DpsoRect* rect);


#ifdef __cplusplus
}
#endif
