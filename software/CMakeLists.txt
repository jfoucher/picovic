

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

add_subdirectory(FatFs_SPI build)

add_subdirectory(scanvideo)



add_executable(picovic
    vic.c
    vic.h
    main.c
)


# Link libraries
target_link_libraries(picovic
    pico_stdlib
    hardware_timer
    hardware_vreg
    pico_multicore
    hardware_pio
    hardware_dma
    hardware_irq
    FatFs_SPI
    scanvideo
)

pico_set_program_name(picovic "picovic")
pico_set_program_version(picovic "0.0.2")

# generate PIO headers
pico_generate_pio_header(picovic ${CMAKE_CURRENT_LIST_DIR}/ps2/ps2.pio)

# Get number of SD cards from the environment, else set to 1:
#   In Visual Studio this can be set in .vscode/settings.json. E.g.:
#      "cmake.configureArgs": ["-DN_SD_CARDS=2"],
IF (NOT DEFINED N_SD_CARDS)
    SET(N_SD_CARDS 1)
ENDIF()
# Set DEBUG and N_SD_CARDS compile definitions:
target_compile_definitions(picovic PUBLIC DEBUG=1 N_SD_CARDS=${N_SD_CARDS})


# enable usb output, disable uart output
pico_enable_stdio_usb(picovic 1)
pico_enable_stdio_uart(picovic 0)

# create map/bin/hex/uf2 file etc.
pico_add_extra_outputs(picovic)
