

cmake_minimum_required(VERSION 3.12)
include(pico_sdk_import.cmake)

project(picovic C CXX ASM)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(PICO_SDK_PATH $ENV{PICO_SDK_PATH})


add_compile_options(-Wall
        -Wno-format          # int != int32_t as far as the compiler is concerned because gcc has int32_t as long int
        -Wno-unused-function # we have some for the docs that aren't called
        -Wno-maybe-uninitialized
        )

add_definitions(-DPICO=1)
add_definitions(-DUSE_MEMORY_POINTER=1)

pico_sdk_init()

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

include_directories(scanvideo)

add_executable(picovic
    scanvideo/vga_modes.c
    scanvideo/composable_scanline.h
    scanvideo/scanvideo.h
    scanvideo/scanvideo.c
    scanvideo/buffer.h
    scanvideo/buffer.c
    scanvideo/scanvideo_base.h
    vic.c
    vic.h
    main.c
)

# Link libraries
target_link_libraries(picovic pico_stdlib hardware_timer hardware_vreg pico_multicore hardware_pio hardware_dma hardware_irq)

# generate PIO headers
pico_generate_pio_header(picovic ${CMAKE_CURRENT_LIST_DIR}/scanvideo/scanvideo.pio)
pico_generate_pio_header(picovic ${CMAKE_CURRENT_LIST_DIR}/scanvideo/timing.pio)
pico_generate_pio_header(picovic ${CMAKE_CURRENT_LIST_DIR}/ps2/ps2.pio)


# enable usb output, disable uart output
pico_enable_stdio_usb(picovic 1)
pico_enable_stdio_uart(picovic 0)

# create map/bin/hex/uf2 file etc.
pico_add_extra_outputs(picovic)
