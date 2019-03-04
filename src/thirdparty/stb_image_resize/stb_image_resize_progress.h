
#pragma once


#ifdef __cplusplus
extern "C" {
#endif


typedef void (*StbirProgressFn)(float progress, void* userData);


void stbirSetProgressFn(
    StbirProgressFn newProgressFn, void* newUserData);


#ifdef __cplusplus
}
#endif
