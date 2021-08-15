add_library(scanvideo INTERFACE)
target_sources(scanvideo INTERFACE
    ${CMAKE_CURRENT_LIST_DIR}/buffer.c
    ${CMAKE_CURRENT_LIST_DIR}/vga_modes.c
    ${CMAKE_CURRENT_LIST_DIR}/scanvideo.c
)
target_include_directories(scanvideo INTERFACE
    include
)

target_link_libraries(scanvideo INTERFACE
    hardware_pio
    hardware_dma
    hardware_irq
    pico_stdlib
)
pico_generate_pio_header(scanvideo ${CMAKE_CURRENT_LIST_DIR}/scanvideo.pio)
pico_generate_pio_header(scanvideo ${CMAKE_CURRENT_LIST_DIR}/timing.pio)