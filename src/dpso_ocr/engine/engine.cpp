#include "engine/engine.h"

#include <iterator>

#include "engine/tesseract/engine.h"


namespace dpso::ocr {


static const Engine* const engines[]{
    &tesseract::getEngine()
};


const Engine& Engine::get(std::size_t idx)
{
    return *engines[idx];
}


std::size_t Engine::getCount()
{
    return std::size(engines);
}


}
