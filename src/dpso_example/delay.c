
#include "delay.h"


#ifdef __unix__


#include <time.h>


void delay(int milliseconds)
{
    struct timespec ts;
    ts.tv_sec = milliseconds / 1000;
    ts.tv_nsec = (milliseconds % 1000) * 1000000;
    nanosleep(&ts, NULL);
}


#elif defined(_WIN32)


#include <windows.h>


void delay(int milliseconds)
{
    Sleep(milliseconds);
}


#else


void delay(int milliseconds);
{
    (void)milliseconds;
}


#endif
