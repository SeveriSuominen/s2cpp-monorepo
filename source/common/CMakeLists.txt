cmake_minimum_required(VERSION 3.8)

# RAYLIB EXTENSION
file(GLOB_RECURSE SRC
   "include/rayext/raygizmo.h"
   "src/rayext/raygizmo.c"
)

set(TARGET_NAME "rayext")

add_library(${TARGET_NAME} ${SRC})

target_include_directories(${TARGET_NAME}
PUBLIC
   ${mxlib_SOURCE_DIR}
   $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>/include/rayext
   raylib
)

target_link_libraries(${TARGET_NAME}
PRIVATE
   raylib
)

