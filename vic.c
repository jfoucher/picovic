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


uint16_t multiplier=1;


void vic_init(uint8_t* buf) {
    px_buf = buf;
    multiplier = vga_mode.width / _VIC20_STD_DISPLAY_WIDTH;
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

    
    uint16_t color = px_buf[0];
    uint16_t old_color = px_buf[0];
    uint16_t color_count = 0;
    uint16_t tokens = 0;

    *p++ = COMPOSABLE_COLOR_RUN;
    *p++ = (uint16_t) color;
    *p++ = (uint16_t) ((vga_mode.width - _VIC20_STD_DISPLAY_WIDTH*multiplier)/2 - 3);
    tokens += 3;

    for (uint px = 0; px < _VIC20_STD_DISPLAY_WIDTH; px++) {
        color = px_buf[line_index * _VIC20_STD_DISPLAY_WIDTH + px];

        if (color == old_color) {
            color_count += multiplier;
        } else {
            color_count += multiplier;
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

    *p++ = COMPOSABLE_COLOR_RUN;
    *p++ = (uint16_t) color;
    *p++ = (uint16_t) ((vga_mode.width - _VIC20_STD_DISPLAY_WIDTH*multiplier)/2 - 4);
    tokens += 3;
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
