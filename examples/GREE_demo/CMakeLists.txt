cmake_minimum_required(VERSION 3.5)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)

set(EXTRA_COMPONENT_DIRS
    ../../components
    )

add_compile_options(-fdiagnostics-color=always)

# The following line can be removed after LVGL update to v8.3.5.
#add_compile_options(-Wno-format)
add_compile_options(-fdiagnostics-color=always -DLV_LVGL_H_INCLUDE_SIMPLE -Wno-format -w)

project(GREE_demo)