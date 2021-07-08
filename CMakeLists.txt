if (TARGET tinyusb_device)

    pico_sdk_init()
    add_definitions(-DPICO=1)
    add_definitions(-DUSE_MEMORY_POINTER=1)

    add_executable(vic20as
        handler.c
        handler.h
        vic.c
        vic.h
        cpu_6502M0.S
        cpu_6502.h
        main.c
    )



    project(vic20as C ASM)

    # Pull in our pico_stdlib which aggregates commonly used features
    target_link_libraries(vic20as pico_stdlib hardware_timer hardware_vreg pico_multicore pico_scanvideo_dpi)


    # enable usb output, disable uart output
    pico_enable_stdio_usb(vic20as 1)
    pico_enable_stdio_uart(vic20as 0)

    # create map/bin/hex/uf2 file etc.
    pico_add_extra_outputs(vic20as)


    # add url via pico_set_program_url
    example_auto_set_url(vic20as)
    #target_link_options(6502emu PRIVATE LINKER:--sort-section=alignment)
elseif(PICO_ON_DEVICE)
    message(WARNING "not building hello_usb because TinyUSB submodule is not initialized in the SDK")
endif()
