/*
 * vic.c
 *
 *  Created on: Dec 25, 2015
 *      Author: kevin
 */
#include <stdlib.h>
#include <stdio.h>
#include "vic.h"

uint8_t* px_buf;




/* Colours from http://www.pepto.de/projects/colorvic */
static uint16_t rgb555[] = {
        0x00,                                               // Black
        0xFF,                                               // White
        (0x68 >> 3) | (0x37 >> 3) << 5 | (0x2B >> 3) << 10,   // Dark red
        (0x70 >> 3) | (0xA4 >> 3) << 5 | (0xB2 >> 3) << 10,   // Cyan
        (0x6F >> 3) | (0x3D >> 3) << 5 | (0x86 >> 3) << 10,   // Purple
        (0x58 >> 3) | (0x8D >> 3) << 5 | (0x43 >> 3) << 10,
        (0x35 >> 3) | (0x28 >> 3) << 5 | (0x79 >> 3) << 10,
        (0xB8 >> 3) | (0xC7 >> 3) << 5 | (0x6F >> 3) << 10,
        (0x6F >> 3) | (0x4F >> 3) << 5 | (0x25 >> 3) << 10,
        (0x43 >> 3) | (0x39 >> 3) << 5 | (0x00 >> 3) << 10,
        (0x9A >> 3) | (0x67 >> 3) << 5 | (0x59 >> 3) << 10,
        (0x44 >> 3) | (0x44 >> 3) << 5 | (0x44 >> 3) << 10,
        (0x6C >> 3) | (0x6C >> 3) << 5 | (0x6C >> 3) << 10,
        (0x9A >> 3) | (0xD2 >> 3) << 5 | (0x84 >> 3) << 10,
        (0x6C >> 3) | (0x5E >> 3) << 5 | (0xB5 >> 3) << 10,
        (0x95 >> 3) | (0x95 >> 3) << 5 | (0x95 >> 3) << 10
};



void vic_init(uint8_t* buf) {
    px_buf = buf;
}


void vic_get_screen_sz(uint16_t *width, uint16_t *height)
{
    *width = CHAR_WIDTH * 8;
    *height = CHAR_HEIGHT * 8;
}


void vic_draw_color_bar(scanvideo_scanline_buffer_t *buffer) {
    uint line_num = scanvideo_scanline_number(buffer->scanline_id);
    uint line_index = (line_num * _VIC20_STD_DISPLAY_HEIGHT) / vga_mode.height;
    uint16_t *p = (uint16_t *) buffer->data;

    
    uint16_t color;
    uint16_t old_color;
    uint16_t color_count = 0;
    uint16_t tokens = 0;

    for (uint px = 0; px < _VIC20_STD_DISPLAY_WIDTH; px++) {
        color = rgb555[px_buf[line_index * _VIC20_STD_DISPLAY_WIDTH + px]];

        if (color == old_color) {
            color_count+=2;
        } else {
            color_count+=2;
            //write old_color color_count times
            //printf("cnt %ld px %ld color %ld\n", color_count, px, old_color);
            if (color_count == 1) {
                *p++ = COMPOSABLE_RAW_1P;
                *p++ = (uint16_t) old_color;
                tokens += 2;
            } else if (color_count == 2) {
                *p++ = COMPOSABLE_RAW_2P;
                *p++ = (uint16_t) old_color;
                *p++ = (uint16_t) old_color;
                tokens += 3;
            } else if (color_count >= 3) {
                *p++ = COMPOSABLE_COLOR_RUN;
                *p++ = (uint16_t) old_color;
                *p++ = (uint16_t) (color_count - 3);
                tokens += 3;
            }
            old_color = color;
            color_count = 0;
        }
    }

    // Write last color
    if (color_count == 1) {
        *p++ = COMPOSABLE_RAW_1P;
        *p++ = (uint16_t) color;
        tokens += 2;
    } else if (color_count == 2) {
        *p++ = COMPOSABLE_RAW_2P;
        *p++ = (uint16_t) color;
        *p++ = (uint16_t) color;
        tokens += 3;
    } else if (color_count >= 3) {
        *p++ = COMPOSABLE_COLOR_RUN;
        *p++ = (uint16_t) color;
        *p++ = (uint16_t) (color_count - 3);
        tokens += 3;
    }

    // black pixel to end line
    *p++ = COMPOSABLE_RAW_1P;
    *p++ = 0;
    tokens += 2;
    // // end of line with alignment padding
    if (tokens % 2 == 0) {
        *p++ = COMPOSABLE_EOL_SKIP_ALIGN;
        *p++ = 0;
    } else {
        *p++ = COMPOSABLE_EOL_ALIGN;
        // *p++ = 0;
    }


    buffer->data_used = ((uint32_t *) p) - buffer->data;
    assert(buffer->data_used < buffer->data_max);

    buffer->status = SCANLINE_OK;
}
