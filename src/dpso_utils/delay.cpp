
#include "delay.h"

#include <chrono>
#include <thread>


void dpsoDelay(int milliseconds)
{
    std::this_thread::sleep_for(
        std::chrono::milliseconds(milliseconds));
}
