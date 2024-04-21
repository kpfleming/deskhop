cmake_minimum_required(VERSION 3.15)

project(deskhop_cfgtool LANGUAGES C)

add_library(minIni STATIC
    ${CMAKE_CURRENT_LIST_DIR}/../minIni/minIni.c
)

target_include_directories(minIni PRIVATE
    ${CMAKE_CURRENT_LIST_DIR} # needed for minGlue.h
    ${CMAKE_CURRENT_LIST_DIR}/../minIni
)

add_executable(cfgtool)

target_sources(cfgtool PUBLIC
    ${CMAKE_CURRENT_LIST_DIR}/cfgtool.c
    ${CMAKE_CURRENT_LIST_DIR}/../src/user_config.c
)

target_include_directories(cfgtool PUBLIC
    ${CMAKE_CURRENT_LIST_DIR}
    ${CMAKE_CURRENT_LIST_DIR}/../src
    ${CMAKE_CURRENT_LIST_DIR}/../minIni
    ${CMAKE_CURRENT_LIST_DIR}/../pico-sdk/lib/tinyusb/src
)

target_compile_options(cfgtool PRIVATE
    $<$<C_COMPILER_ID:Clang,AppleClang,GNU>:-Wall -Wextra -Wconversion -Wsign-conversion -Werror -g>
    $<$<C_COMPILER_ID:MSVC>:/W4 /WX /DEBUG>
)

target_link_libraries(cfgtool PRIVATE
    minIni
)
