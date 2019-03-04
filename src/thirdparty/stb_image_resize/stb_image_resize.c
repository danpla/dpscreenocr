

void stbirInvokeProgressFn(float progress);

#define STBIR_PROGRESS_REPORT(p) stbirInvokeProgressFn(p)


#define STB_IMAGE_RESIZE_IMPLEMENTATION
#include "stb_image_resize.h"
