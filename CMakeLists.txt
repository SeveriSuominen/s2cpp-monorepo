cmake_minimum_required(VERSION 3.8)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

# adding Raylib
include(FetchContent)
set(FETCHCONTENT_QUIET FALSE)
set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE) # dont build the supplied examples
set(BUILD_GAMES    OFF CACHE BOOL "" FORCE) # dont build the supplied example games

FetchContent_Declare(
    raylib
    GIT_REPOSITORY "https://github.com/raysan5/raylib.git"
    GIT_TAG "5.5"
    GIT_PROGRESS TRUE
)
FetchContent_MakeAvailable(raylib)

FetchContent_Declare(
    mxlib
    GIT_REPOSITORY "https://github.com/SeveriSuominen/mxlib"
    GIT_PROGRESS TRUE
)
FetchContent_MakeAvailable(mxlib)

project(medium_monorepo LANGUAGES CXX C)

set(CMAKE_MESSAGE_LOG_LEVEL VERBOSE) 

# define runtime intermediate build directory, so that debugging the Visual Studio solution works as expected.
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/")

# set export directory for CMake config.
set(CMAKE_INSTALL_BINARY_DIR    "bin/")
set(CMAKE_INSTALL_LIBRARY_DIR   "bin/")
set(CMAKE_INSTALL_INCLUDE_DIR   "include/")
set(CMAKE_INSTALL_EXPORT_DIR    "cmake/")

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_C_STANDARD 11)

add_compile_options(-fdiagnostics-color)

add_subdirectory(source/common)
add_subdirectory(source/cg-gjk)
add_subdirectory(source/quick_guides/dot_and_cross)


