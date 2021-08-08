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
#include "cpu_6502.c"
#include "vic.h"
#include "kernal.h"
#include "basic.h"
#include "char_rom.h"

#define CHAR_BUFFER_ADDRESS 0x277
#define CHAR_BUFFER_NUM_ADDRESS 0xC6
// If this is active, then an overclock will be applied
#define OVERCLOCK
// Comment this to run your own ROM
#define TESTING

// Delay startup by so many seconds
#define START_DELAY 6

#define VIA2_BASE (0x9120)
#define VIA2_PORTB  (VIA2_BASE)
#define VIA2_PORTA  (VIA2_BASE+1)
#define VIA2_DDRB   (VIA2_BASE+2)
#define VIA2_DDRA   (VIA2_BASE+3)
#define VIA2_T1CL   (VIA2_BASE + 4)
#define VIA2_T1CH   (VIA2_BASE + 5)
#define VIA2_T1LL   (VIA2_BASE + 6)
#define VIA2_T1LH   (VIA2_BASE + 7)
#define VIA2_ACR    (VIA2_BASE + 0xB)
#define VIA2_PCR    (VIA2_BASE + 0xC)
#define VIA2_IFR    (VIA2_BASE + 0xD)
#define VIA2_IER    (VIA2_BASE + 0xE)
#define VIA2_PORTA2    (VIA2_BASE + 0xF)

uint8_t pxbuf[_VIC20_STD_DISPLAY_WIDTH * _VIC20_STD_DISPLAY_HEIGHT];

absolute_time_t start;

volatile int tube_irq;

void core1_func();
static semaphore_t video_initted;

absolute_time_t gotchar;
absolute_time_t lastgotchar;

absolute_time_t start;

volatile int tube_irq;

uint8_t mpu_memory[64*1024];

bool timer_started = false;


uint16_t char_mem_addresses[] = {0x8000,0x8400  ,0x8800  ,0x8C00  ,0x0000 ,0x0000 ,0x0000,0x0000 ,0x1000,0x1400,0x1800,0x1C00};


/* Colours from http://www.pepto.de/projects/colorvic */
const uint8_t rgb8[] = {
        0x00,                                               // Black
        0xFF,                                               // White
        0xE0,   // Dark red
        0x1F,   // Cyan
        0x61,   // Purple
        0x3C,  // Green
        0x07,  // Blue
        0xFC,     // Yellow
        0xD4,     // Orange
        ((0x43 >> 5) | (0x39 >> 5) << 3 | (0x00 >> 6) << 6) & 0xFF,
        ((0x9A >> 5) | (0x67 >> 5) << 3 | (0x59 >> 6) << 6) & 0xFF,
        ((0x44 >> 5) | (0x44 >> 5) << 3 | (0x44 >> 6) << 6) & 0xFF,
        ((0x6C >> 5) | (0x6C >> 5) << 3 | (0x6C >> 6) << 6) & 0xFF,
        ((0x9A >> 5) | (0xD2 >> 5) << 3 | (0x84 >> 6) << 6) & 0xFF,
        ((0x6C >> 5) | (0x5E >> 5) << 3 | (0xB5 >> 6) << 6) & 0xFF,
        ((0x95 >> 5) | (0x95 >> 5) << 3 | (0x95 >> 6) << 6) & 0xFF
};



void push_audio() {

};

absolute_time_t start;
bool running = true;

uint8_t read6502(uint16_t address) {
    if (address == VIA2_PORTA2) {
        address = VIA2_PORTA;
    }
    if (address == VIA2_T1CL && mpu_memory[VIA2_IFR]) {
      mpu_memory[VIA2_IFR] = 0x00;
    //   printf("timer int cleared by T1CL read\n");
    }
    if (address == VIA2_PORTB || address == VIA2_PORTA) {
        return 0;
    }
    return mpu_memory[address];
}

void write6502(uint16_t address, uint8_t data) {
    if (mpu_memory[VIA2_IFR] && address == VIA2_T1CH) {
        // Clear interrupt
        mpu_memory[VIA2_IFR] &= ~0xC0;
        mpu_memory[address] = data;
        // printf("timer int cleared by T1CH write %04X %02X\n", address, data);
        timer_started = true;
        return;
    }
    else if (mpu_memory[VIA2_IFR] && address == VIA2_IFR) {
        // Clear interrupt
        mpu_memory[VIA2_IFR] = data;
        // printf("timer int cleared by IFR write\n");
        // timer_started = true;
        return;
    } 
    else if (address == VIA2_IER) {
        if ((data & 0x80) == 0x80) {
            // set the bits that are set
            mpu_memory[VIA2_IER] |= (data & 0x7F);
        } else {
            // unset the bits that are set
            mpu_memory[VIA2_IER] &= (~data) & 0x7F;
            // if (data & 0x40) {
            //     // T1 interrupt enable bit reset, clear interrupt
            //     mpu_memory[VIA2_IFR] &= ~0xC0;
            // }
        }
        return;
    } 

    mpu_memory[address] = data;
}

uint cnt = 0;

void callback() {


    
    // if (pc == 0xFF72) {
    //     printf("PULS\n");
    //     printf("old status %02X\n", mpu_memory[sp + 0x104]);
    //     pc = 0xFF82;
    // }
    // if (pc == 0xFF7F) {
    //     printf("BRK\n");
    // }
    // if (pc == 0xFF82) {
    //     printf("IRQ\n");
    // }
    // if (pc == 0xFD22) {
    //     printf("START\n");
    // }

    gotchar = get_absolute_time();
    if (absolute_time_diff_us(lastgotchar, gotchar) > 16666) {
        // mpu_memory[VIA2_T1CH] = 0;
        // mpu_memory[VIA2_T1CL] = 0;

        

        cnt++;
        if (cnt % 120 == 0) {
            uint64_t elapsed = absolute_time_diff_us(start, gotchar);
            float mhz = (float) clockticks6502 / (float) elapsed;
            printf("speed: %.3f\n", mhz);
        }
        

            //printf("timer int %02X\n", tube_irq);
        if ((mpu_memory[VIA2_ACR] & mpu_memory[VIA2_IER] & 0x40) && ((status & FLAG_INTERRUPT) == 0)) {
            mpu_memory[VIA2_IFR] = 0xC0;
            status  &= ~FLAG_BREAK;  // WHY ????
            // printf("trigger irq");
            irq6502();
        }


        lastgotchar = gotchar;
        int chr = getchar_timeout_us(0);
        if (chr != PICO_ERROR_TIMEOUT) {
            // POke character in memory
            uint8_t nb_chars = mpu_memory[CHAR_BUFFER_NUM_ADDRESS];
            if (chr > 0x60) {
                chr = chr-0x20;
            }
            if ((chr & 0xFF) == 3) {
                chr = 23;
            }
            unsigned char s[] = {(uint8_t) (chr & 0xFF), 0};
            printf("%s", &s);

            mpu_memory[CHAR_BUFFER_ADDRESS + nb_chars] = (uint8_t) (chr & 0xFF);
            mpu_memory[CHAR_BUFFER_NUM_ADDRESS] = nb_chars + 1;
        }

        //Fill framebuffer
        uint8_t *b = (uint8_t *) pxbuf;
        uint8_t color;
        uint8_t fgcolor;
        uint8_t bgcolor;
        bool inverted;

        for (uint y = 0; y < _VIC20_STD_DISPLAY_HEIGHT; y++) {
            if (y < V_BORDER_SIZE || y >= (_VIC20_STD_DISPLAY_HEIGHT - V_BORDER_SIZE)) {
                color = (uint8_t) mpu_memory[0x900F] & 0x7;
                for (int x = 0; x < _VIC20_STD_DISPLAY_WIDTH; x++) {
                *b++ = rgb8[color];
                }
            } else {
                uint8_t c = (uint8_t) mpu_memory[0x900F];
                bgcolor = (c >> 4) & 0xF;
                inverted = (c >> 3) & 0x1;
                for (uint x=0; x < H_BORDER_SIZE; x++) {
                *b++ = rgb8[c & 0x07];
                }
                for (uint x=0; x < CHAR_WIDTH; x++) {
                uint yy = y - V_BORDER_SIZE;
                uint16_t offset = (yy >> 3) * CHAR_WIDTH + x;

                // Get the character at this position
                uint8_t ch = (uint8_t)mpu_memory[SCREEN_ADDR + offset];
                // Get the character foreground color
                fgcolor = (uint8_t)mpu_memory[COLOUR_ADDR + offset];
                // Get the character data
                uint16_t bit_data_addr = CHAR_ROM_ADDR + (((uint16_t) ch) << 3) + ((uint16_t)yy & 0x7);
                uint8_t pixels = (uint8_t)mpu_memory[bit_data_addr];

                for (uint p = 0; p < 8; p++) {
                    bool pixel = (pixels >> (7-p)) & 1;
                    if (inverted) {
                        *b++ = pixel ? rgb8[fgcolor & 0xF] : rgb8[bgcolor & 0xF];
                    } else {
                        *b++ = pixel ? rgb8[bgcolor & 0xF] : rgb8[fgcolor & 0xF];
                    }
                    
                    //*b++ = inverted ? (pixel ? rgb555[bgcolor & 0xF] : rgb555[fgcolor & 0xF]) : (pixel ? rgb555[fgcolor & 0xF] : rgb555[bgcolor & 0xF]);
                }
                }

                for (uint x=0; x < H_BORDER_SIZE; x++) {
                *b++ = rgb8[c & 0x07];
                }
            }
        }
    }
}

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

    for (int i = 0; i < 0x10000; i++) {
        mpu_memory[i] = 0;
    }
    
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
    gotchar = get_absolute_time();
    lastgotchar = get_absolute_time();

    reset6502();

    //hookexternal(callback);

    while(1) {
        callback();
        step6502();  
    }


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