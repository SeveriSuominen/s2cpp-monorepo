cmake_minimum_required(VERSION 3.8)

file(GLOB_RECURSE SRC
   "demo.cpp"
)

set(TARGET_NAME "dot_and_cross")

add_executable(${TARGET_NAME} ${SRC})
add_dependencies(${TARGET_NAME} rayext)

target_include_directories(${TARGET_NAME}
PUBLIC
   ${mxlib_SOURCE_DIR}
   $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>/../../common/include
)

target_link_libraries(${TARGET_NAME}
PRIVATE
   raylib
   rayext
)

set(output_directory "${CMAKE_BINARY_DIR}/bin/${TARGET_NAME}")

set_target_properties(${TARGET_NAME}
    PROPERTIES
    ARCHIVE_OUTPUT_DIRECTORY ${output_directory}
    LIBRARY_OUTPUT_DIRECTORY ${output_directory}
    RUNTIME_OUTPUT_DIRECTORY ${output_directory}
)

function(COPY_TO_BIN PATH)
    add_custom_command(TARGET ${TARGET_NAME} POST_BUILD
        COMMAND "${CMAKE_COMMAND}" -E copy
        $<BUILD_INTERFACE:${PATH}>
        $<$<CONFIG:Debug>:"${output_directory}/Debug">
        $<$<CONFIG:Release>:"${output_directory}/Release">
    )
endfunction()

add_custom_command(TARGET ${TARGET_NAME} POST_BUILD
   COMMAND "${CMAKE_COMMAND}" -E copy_directory
   $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>/../../common/fonts/DepartureMono
   $<$<CONFIG:Debug>:"${output_directory}/Debug/fonts">
   $<$<CONFIG:Release>:"${output_directory}/Release/fonts">
)

add_custom_command(TARGET ${TARGET_NAME} POST_BUILD
   COMMAND "${CMAKE_COMMAND}" -E copy_directory
   $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>/images
   $<$<CONFIG:Debug>:"${output_directory}/Debug/images">
   $<$<CONFIG:Release>:"${output_directory}/Release/images">
)

COPY_TO_BIN($<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>/../../common/style/style_dark.rgs)
