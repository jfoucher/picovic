// copro-65tube.h
#ifndef HANDLER_H_
#define HANDLER_H_
#include "vic.h"


#define FAST6502_BIT 128
#define NATIVEARM_BIT 64
#define nativearmlock_bit 32
#define TUBE_ENABLE_BIT  8
#define RESET_BIT   4
#define NMI_BIT     2
#define IRQ_BIT     1


extern uint8_t pxbuf[_VIC20_STD_DISPLAY_WIDTH * _VIC20_STD_DISPLAY_HEIGHT];

extern absolute_time_t start;

extern volatile int tube_irq;



extern unsigned char * mem_reset();

extern void exec_65(unsigned char *memory, unsigned int speed);


extern uint8_t io_read( uint16_t address);
extern void io_write( uint8_t data, uint16_t address);

extern void callback(uint8_t inst);

extern void interrupted(uint8_t val);



#endif