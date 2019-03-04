
#include "delay.h"

#include <thread>
#include <chrono>


void delay(int milliseconds)
{
    std::this_thread::sleep_for(
        std::chrono::milliseconds(milliseconds));
}
