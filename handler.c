/*
 * 6502 Co Processor Emulation
 *
 * (c) 2016 David Banks and Ed Spittles
 *
 * based on code by
 * - Reuben Scratton.
 * - Tom Walker
 *
 */

#include <stdio.h>
#include <string.h>
#include "pico/stdlib.h"
#include "vic.h"

#include "handler.h"

#define CHAR_BUFFER_ADDRESS 0x277
#define CHAR_BUFFER_NUM_ADDRESS 0xC6

//extern uint32_t ticks;

uint64_t ticks = 0;

unsigned char mpu_memory[64*1024];



inline void _disable_interrupts()
{
   __asm volatile ("cpsid i");
}

inline void _enable_interrupts()
{
   __asm volatile ("cpsie i");
}

uint16_t char_mem_addresses[] = {0x8000,0x8400  ,0x8800  ,0x8C00  ,0x0000 ,0x0000 ,0x0000,0x0000 ,0x1000,0x1400,0x1800,0x1C00};


absolute_time_t gotchar;
absolute_time_t lastgotchar;

extern void callback(uint8_t inst)
{
   ticks += inst; //timing_table[inst];

   gotchar = get_absolute_time();
   if (absolute_time_diff_us(lastgotchar, gotchar) > 16000) {

      // absolute_time_t now = get_absolute_time();
      // int64_t elapsed = absolute_time_diff_us(start, now);

      // float khz = (float)ticks / (float)elapsed;

      // printf("test %ld after %ld ticks speed is %.2f MHz\n", mpu_memory[SCREEN_ADDR], ticks, khz);
   
      
      //Get characters from serial
      lastgotchar = gotchar;
      int chr = getchar_timeout_us(0);
      if (chr != PICO_ERROR_TIMEOUT) {
         // POke character in memory
         printf("got char %d\n", (unsigned char)chr);
         uint8_t nb_chars = mpu_memory[CHAR_BUFFER_NUM_ADDRESS];
         if (chr > 0x60) {
            chr = chr-0x20;
         }
         if ((chr & 0xFF) == 3) {
            printf("ctrl C\n");
            chr = 23;
         }
         printf("converted char %d\n", (unsigned char)chr);
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
         if (y < V_BORDER_SIZE || y > (_VIC20_STD_DISPLAY_HEIGHT - V_BORDER_SIZE)) {
            color = (uint8_t) mpu_memory[0x900F] & 0x7;
            for (int x = 0; x < _VIC20_STD_DISPLAY_WIDTH; x++) {
               *b++ = color;
            }
         } else {
            uint8_t c = (uint8_t) mpu_memory[0x900F];
            bgcolor = c & 0xF;
            inverted = (c >> 4) & 0x1;
            for (uint x=0; x < H_BORDER_SIZE; x++) {
               *b++ = c & 0x07;
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

                  *b++ = inverted ? (pixel ? (bgcolor & 0xF) : (fgcolor & 0xF)) : (pixel ? (fgcolor & 0xF) : (bgcolor & 0xF));
               }
            }

            for (uint x=0; x < H_BORDER_SIZE; x++) {
               *b++ = c & 0x07;
            }
         }

         
         
      }
   }
   

   // if (ticks  % 10000000 == 0) {
   //    absolute_time_t now = get_absolute_time();
   //    int64_t elapsed = absolute_time_diff_us(start, now);

   //    float khz = (float)ticks / (float)elapsed;

   //    printf("test %ld after %ld ticks speed is %.2f MHz\n", mpu_memory[0x202], ticks, khz);
   // }
}


unsigned char * mem_reset(int length)
{
   // Wipe memory
   memset(mpu_memory, 0, length);

   // return pointer to memory
   return mpu_memory;
}

