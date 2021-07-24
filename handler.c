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



absolute_time_t gotchar;
absolute_time_t lastgotchar;

extern void callback(uint8_t inst)
{
   ticks += inst; //timing_table[inst];

   gotchar = get_absolute_time();
   if (absolute_time_diff_us(lastgotchar, gotchar) > 16000) {
      // tube_irq |= IRQ_BIT;

      mpu_memory[VIA2_IFR] = 0xC0;
      // Update elapsed time
      uint time = mpu_memory[0xA0] << 16 | mpu_memory[0xA1] << 8 | mpu_memory[0xA2];
      time++;
      if (time >= 0x4F1A01) {
         time = 0;
      }
      mpu_memory[0xA0] = (time >> 16) & 0xFF;
      mpu_memory[0xA1] = (time >> 8) & 0xFF;
      mpu_memory[0xA2] = (time) & 0xFF;
      
      //Get characters from serial
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
   

   // if (ticks  % 10000000 == 0) {
   //    absolute_time_t now = get_absolute_time();
   //    int64_t elapsed = absolute_time_diff_us(start, now);

   //    float khz = (float)ticks / (float)elapsed;

   //    printf("test %ld after %ld ticks speed is %.2f MHz\n", mpu_memory[0x202], ticks, khz);
   // }
}

extern uint8_t io_read( uint16_t address) {
   //if ((address >> 9) == 0x48) {
      printf("read address: %04X\n", address);
   //}

   if (address == VIA2_T1CL || address == VIA2_T1LL) {
      // Clear interrupt
      tube_irq &= ~IRQ_BIT;
      mpu_memory[VIA2_IFR] = 0x00;
   }
   
   return mpu_memory[address];
}
extern void io_write( uint8_t data, uint16_t address) {
   if ((address >> 9) == 0x48) {
      printf("write address: %04X data: %02X\n", address, data);
   }
}

unsigned char * mem_reset(int length)
{
   // Wipe memory
   memset(mpu_memory, 0, length);

   // return pointer to memory
   return mpu_memory;
}

