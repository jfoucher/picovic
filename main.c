/**
 * Copyright (c) 2021 Jonathan Foucher
 *
 * SPDX-License-Identifier: MIT
 */

#include <stdio.h>
#include <string.h>

#include "pico/stdlib.h"
#include "pico/time.h"
#include "hardware/clocks.h"
#include "hardware/vreg.h"
#include "pico/time.h"
#include "pico/multicore.h"
#include "scanvideo/scanvideo.h"
#include "scanvideo/composable_scanline.h"
#include "pico/sync.h"
#include "hardware/clocks.h"
#include "hardware/vreg.h"
#include "handler.h"
#include "vic.h"
#include "kernal.h"
#include "basic.h"
#include "char_rom.h"


// If this is active, then an overclock will be applied
#define OVERCLOCK
// Comment this to run your own ROM
#define TESTING

// Delay startup by so many seconds
#define START_DELAY 6

uint8_t pxbuf[_VIC20_STD_DISPLAY_WIDTH * _VIC20_STD_DISPLAY_HEIGHT];

absolute_time_t start;

volatile int tube_irq;

void core1_func();
static semaphore_t video_initted;

absolute_time_t start;

volatile int tube_irq;

uint8_t pxbuf[_VIC20_STD_DISPLAY_WIDTH * _VIC20_STD_DISPLAY_HEIGHT];

void push_audio() {

};

absolute_time_t start;
bool running = true;


int main() {
#ifdef OVERCLOCK
    vreg_set_voltage(VREG_VOLTAGE_1_15);
    set_sys_clock_khz(260000, true);
#endif
    stdio_init_all();

#ifdef START_DELAY
    for(uint8_t i = START_DELAY; i > 0; i--) {
        printf("Starting in %d \n", i);
        sleep_ms(1000);
    }
#endif
    //printf("Starting\n");

    //prepare vga
    // create a semaphore to be posted when video init is complete
    sem_init(&video_initted, 0, 1);

    // launch all the video on core 1, so it isn't affected by USB handling on core 0
    multicore_launch_core1(core1_func);
    // wait for initialization of video to be complete
    sem_acquire_blocking(&video_initted);


    start = get_absolute_time();
    tube_irq |= TUBE_ENABLE_BIT;
    tube_irq &= ~(RESET_BIT + NMI_BIT + IRQ_BIT);

   unsigned char * mpu_memory; // now the arm vectors have moved we can set the core memory to start at 0
   mpu_memory = mem_reset(mpu_memory, 0x8000);


    /*
        VIC-20 CPU memory map:

        0000..03FF      zero-page, stack, system area
        [0400..0FFF]    3 KB Expansion RAM
        1000..1FFF      4 KB Main RAM (block 0)
        [2000..3FFF]    8 KB Expansion Block 1
        [4000..5FFF]    8 KB Expansion Block 2
        [6000..7FFF]    8 KB Expansion Block 3
        8000..8FFF      4 KB Character ROM
        9000..900F      VIC Registers
        9110..911F      VIA #1 Registers
        9120..912F      VIA #2 Registers
        9400..97FF      1Kx4 bit color ram (either at 9600 or 9400)
        [9800..9BFF]    1 KB I/O Expansion 2
        [9C00..9FFF]    1 KB I/O Expansion 3
        [A000..BFFF]    8 KB Expansion Block 5 (usually ROM cartridges)
        C000..DFFF      8 KB BASIC ROM
        E000..FFFF      8 KB KERNAL ROM

    */

    
    
    for (int i = 0x8000; i < 0x9000; i++) {
        mpu_memory[i] = characters[i-0x8000];
    }
    for (int i = 0xC000; i < 0xE000; i++) {
        mpu_memory[i] = basic[i-0xC000];
    }
    for (int i = 0xE000; i < 0x10000; i++) {
        mpu_memory[i] = kernal[i-0xE000];
    }

    vic_init(pxbuf);

    exec_65(mpu_memory, 1);


    return 0;
}



void core1_func() {
    // initialize video and interrupts on core 1
    scanvideo_setup(&vga_mode);
    scanvideo_timing_enable(true);
    sem_release(&video_initted);

    while (true) {
        scanvideo_scanline_buffer_t *scanline_buffer = scanvideo_begin_scanline_generation(true);

        vic_draw_color_bar(scanline_buffer);
        scanvideo_end_scanline_generation(scanline_buffer);
    }
}