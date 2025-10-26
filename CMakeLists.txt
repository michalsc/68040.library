cmake_minimum_required(VERSION 3.14.0)
project(68040 VERSION 1.3.0)

include(cmake/verstring.cmake)
get_verstring(VERSTRING)


add_executable(68040.library
    src/main.c
    src/init.c
    src/cachepredma.c
    src/cachepostdma.c
)

target_compile_options(68040.library PRIVATE -Os -m68040 -msmall-code -mregparm=4 -fomit-frame-pointer)
target_compile_definitions(68040.library PRIVATE  VERSION_STRING="${VERSTRING}")
target_include_directories(68040.library PRIVATE include)
target_link_options(68040.library PRIVATE -ffreestanding -nostdlib -nostartfiles -s -Wl,-e_rom_start -Wl,-T${CMAKE_CURRENT_SOURCE_DIR}/ldscript.lds)
target_link_libraries(68040.library devicetree)

set_target_properties(68040.library PROPERTIES LINK_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/ldscript.lds)
