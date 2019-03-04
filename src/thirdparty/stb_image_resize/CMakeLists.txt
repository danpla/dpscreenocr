add_library(
    stb_image_resize STATIC
    stb_image_resize.c stb_image_resize_progress.c
)
target_include_directories(stb_image_resize PUBLIC .)
