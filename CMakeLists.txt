

cmake_minimum_required(VERSION 3.12)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(PICO_SDK_PATH $ENV{PICO_SDK_PATH})
add_definitions(-DPICO=1)
add_definitions(-DUSE_MEMORY_POINTER=1)
include(pico_sdk_import.cmake)



project(picovic2 C CXX ASM)

include_directories(scanvideo)

add_executable(picovic2
    # handler.c
    # handler.h
    vic.c
    vic.h
    # cpu_6502M0.S
    # cpu_6502.c
    scanvideo/vga_modes.c
    scanvideo/composable_scanline.h
    scanvideo/scanvideo.h
    scanvideo/scanvideo.c
    scanvideo/buffer.h
    scanvideo/buffer.c
    scanvideo/scanvideo_base.h
    main.c
)



pico_sdk_init()


# Pull in our pico_stdlib which aggregates commonly used features
target_link_libraries(picovic2 pico_stdlib hardware_timer hardware_vreg pico_multicore hardware_pio hardware_dma hardware_irq)

pico_generate_pio_header(picovic2 ${CMAKE_CURRENT_LIST_DIR}/scanvideo/scanvideo.pio)
pico_generate_pio_header(picovic2 ${CMAKE_CURRENT_LIST_DIR}/scanvideo/timing.pio)
pico_generate_pio_header(picovic2 ${CMAKE_CURRENT_LIST_DIR}/ps2/ps2.pio)

# enable usb output, disable uart output
pico_enable_stdio_usb(picovic2 1)
pico_enable_stdio_uart(picovic2 0)

# create map/bin/hex/uf2 file etc.
pico_add_extra_outputs(picovic2)

#target_link_options(6502emu PRIVATE LINKER:--sort-section=alignment)
