cmake_minimum_required(VERSION 2.8.12)

project(dpso_intl)

option(DPSO_ENABLE_NLS "Enable native language support" YES)

add_library(dpso_intl STATIC dpso_intl.cpp)

target_compile_definitions(
    dpso_intl PUBLIC ENABLE_NLS=$<BOOL:${DPSO_ENABLE_NLS}>
)

target_include_directories(dpso_intl PRIVATE . PUBLIC ..)

if(DPSO_ENABLE_NLS)
    find_package(Intl REQUIRED)
    target_include_directories(dpso_intl PUBLIC ${Intl_INCLUDE_DIRS})
    target_link_libraries(dpso_intl ${Intl_LIBRARIES})
endif()
