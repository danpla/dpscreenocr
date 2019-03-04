
#include "stb_image_resize_progress.h"


static void nullProgressFn(float progress, void* userData)
{
    (void)progress;
    (void)userData;
}


static StbirProgressFn progressFn = nullProgressFn;
static void* userData;


void stbirSetProgressFn(
    StbirProgressFn newProgressFn, void* newUserData)
{
    progressFn = newProgressFn ? newProgressFn : nullProgressFn;
    userData = newUserData;
}


void stbirInvokeProgressFn(float progress)
{
    progressFn(progress, userData);
}
