/*
 * vic.h
 *
 *  Created on: Aug 23, 2015
 *      Author: kevin
 */

#ifndef VIC_H_
#define VIC_H_

#include <stdint.h>
#include "scanvideo/scanvideo.h"
#include "scanvideo/scanvideo_base.h"

#include "scanvideo/composable_scanline.h"
#include "pico/multicore.h"
#include "pico/sync.h"
#define _VIC20_STD_DISPLAY_WIDTH (232)  /* actually 229, but rounded up to 8x */
#define _VIC20_STD_DISPLAY_HEIGHT (272)

#define vga_mode vga_mode_1024x768_60

#define        SCREEN_ADDR (0x1000)
#define        COLOUR_ADDR (0x9400)
#define        CHAR_ROM_ADDR (0x8000)

#define        CHAR_WIDTH (22)
#define        CHAR_HEIGHT (23)

#define        H_BORDER_SIZE ((_VIC20_STD_DISPLAY_WIDTH - CHAR_WIDTH*8) / 2)
#define        V_BORDER_SIZE ((_VIC20_STD_DISPLAY_HEIGHT - CHAR_HEIGHT*8) / 2)

typedef struct vic_st_ {
    uint8_t   origin_h;
    uint8_t   origin_v;
    uint8_t   n_cols;
    uint8_t   n_rows;
    uint8_t   raster;
    uint8_t   screen_char_mem;
    uint8_t   pen_h;
    uint8_t   pen_v;
    uint8_t   paddle1;
    uint8_t   paddle2;
    uint8_t   bass;
    uint8_t   alto;
    uint8_t   soprano;
    uint8_t   noise;
    uint8_t   aux_colour_volume;
    uint8_t   screen_border_col;
} vic_st;

#define VIC_INTERLACE_MASK  0x80
#define VIC_ORIGIN_H_MASK   0x7F

typedef struct rgbu8_st_ {
    uint8_t red;
    uint8_t green;
    uint8_t blue;
} rgbu8_st;


void     vic_init(uint8_t *mem_in);
void     vic_get_screen_sz(uint16_t *width, uint16_t *height);

void vic_draw_color_bar(scanvideo_scanline_buffer_t *buffer);


#endif /* INC_VIC_H_ */
