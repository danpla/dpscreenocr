// AVX2 is available since 2013, so disable it in case someone runs
// our binaries on an older hardware.
#define STBIR_NO_AVX2

#define STB_IMAGE_RESIZE_IMPLEMENTATION
#include "stb_image_resize2.h"
