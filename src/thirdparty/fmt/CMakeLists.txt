add_library(fmt src/format.cc)
target_include_directories(fmt PRIVATE include/fmt PUBLIC include)
