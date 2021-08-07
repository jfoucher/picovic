
// used to create a LO temperary register
.macro push_r reg
   mov   temp,\reg
.endm

.macro pop_r reg
   mov   \reg, temp
.endm

.macro SET_FLAGS_NZ reg=rAcc
   sxtb  rflagsNZ, \reg
.endm

.macro CLEAR_ZFLAG
   lsr    rflagsNZ, #8
   lsl    rflagsNZ, #8  // clear Z flag
.endm

.macro SETUP_CARRY reg=rflagsNZ
   LSR   \reg, rCarry, #1
.endm

.macro SAVE_CARRY
// Carry stored in the LSB
// Corrupts V flag
// if rcarry was clear then adding itself to itself is still clear
// if rcarry was set then adding itself to itself is cleared
// so ADC is always adding new carry to a clear bit 0
   ADC   rCarry, rCarry, rCarry
.endm

.macro SAVE_VFLAG_FAST reg=r0
   bvc   1f
   mov   \reg, #pByteVflag // Corrupts NZ but not CV
   add   rPbyteLo, rPbyteLo, \reg
1:
.endm

.macro CLEARV_PART2
   LSR  r1,#28
   mov  rPbyteLo,r1  // Vflag now clear
.endm

// NB BRK and PHP sets B flags on stacking
// IRQ and NMI B =0
// Other interrupts don't set the the B flag

.macro statustoR reg bflag
   LSL   \reg, rflagsNZ,#24   // lower bytes cleared
   BNE   1f
   add   \reg, #pByteZflag
1:
   cmp   rflagsNZ,#0xff
   BLS   2f
   add   \reg, #pByteNflag
2:
   SETUP_CARRY r1
   BCC   3f
   add   \reg, #pByteCflag
3:
.ifc \bflag,bflag
   add   \reg, #pByteXflag+pByteBflag
.else
   add   \reg, #pByteXflag
.endif
   mov   r1, rPbyteLo
   orr   \reg, r1  // add in V, D and I flags
.endm

@ convert from 6502 status byte (nv-bdizc) in r0 to ARMs Pbyte (NZCV)
//  entry with rCarry preset with byte as we get that for free
.macro r0toPByte

   SET_FLAGS_NZ rCarry  // set Nflag

   mov   r1,#255-pByteZflag
   bic   rflagsNZ,rflagsNZ,r1
   mov   r1,#pByteZflag
   eor   rflagsNZ, rflagsNZ, r1

   movs  r1, #(pByteDflag+pByteIflag+pByteVflag)
   and   r1, rCarry, r1
   mov   rPbyteLo, r1 // we leave r1 with rPbyteLo so check IRQ can use it
.endm

// used for instructions that can enable interrupts
// CLI PLP RTI ( shorter version used for CLI
// r1 is rPbyteLo
.macro CHECK_IRQ
   // check for external IRQ
   @ ldr   r0,=tube_irq
   @ ldrb  r0,[r0]
   @ lsr   r0,r0,#1
   @ bcc   1f
   @ // check if 6502 IRQs enabled
   @ lsr   r1,r1, #3  // bit 2 =0 means enabled
   @ bcs   1f
   @ INTR 2
1:
   NEXT_INSTRUCTION 0
.endm

.macro GET_ZP_LOCATION   // $00
   load_operand_byte r1
.endm

.macro GET_ZPX_LOCATION
   load_operand_byte r1
   add   r1, rXreg
   UXTB  r1, r1 @ AND #255
.endm

.macro GET_ZPY_LOCATION
   load_operand_byte r1
   add   r1, rYreg
   UXTB  r1, r1 @ AND #255
.endm

.macro GET_IND_LOCATION
   load_operand_byte r1
   ldrb  r0, [r1, rmem]
   ADD   r1, r1, #1
   ldrb  r1, [r1, rmem]
   LSL   r1, r1, #8
   orr   r1, r0
.endm

.macro GET_ABS_LOCATION reg=r1
.ifc \reg,r0
   ldrb  \reg, [rPC]
   ldrb  r1 , [rPC, #1]
   LSL   r1, r1,#8
   orr   \reg, \reg, r1
.else
   ldrb  \reg, [rPC]
   ldrb  r0 , [rPC, #1]
   LSL   r0, r0,#8
   orr   \reg, \reg, r0
.endif
.endm

.macro GET_INY_LOCATION
   GET_IND_LOCATION
   add   r1, r1, rYreg
.endm

.macro GET_INX_LOCATION // ($00,X)
   load_operand_byte r1
   add   r1, rXreg
   UXTB  r1, r1 @ AND #255
   ldrb  r0, [r1, rmem]
   ADD   r1, #1
   ldrb  r1, [r1, rmem]
   LSL   r1, #8
   orr   r1, r0
.endm

.macro  GET_ABY_LOCATION // $0000,Y
   GET_ABS_LOCATION r1
   add   r1, rYreg
   uxth  r1, r1
.endm

.macro  GET_ABX_LOCATION // $0000,X
   GET_ABS_LOCATION r1
   add   r1, rXreg
   uxth  r1, r1
.endm

.macro load_operand_byte reg=r0
   ldrb  \reg, [rPC]
.endm

.macro   load_2_operand_bytes_jmp /* and adjust PC */
   GET_ABS_LOCATION r0
   add   rPC, r0, rmem
.endm

.macro LOAD_INX reg=r0
   GET_INX_LOCATION
   LOAD_BYTE \reg , r1
.endm

.macro LOAD_IMM reg=r0
   load_operand_byte \reg
.endm

.macro LOAD_ZP reg=r0
   GET_ZP_LOCATION
   LOAD_BYTE \reg, r1
.endm

.macro LOAD_ABS reg=r0
   GET_ABS_LOCATION r1
   LOAD_BYTE \reg, r1
.endm


.macro LOAD_INY reg=r0 // ($00),Y
   GET_INY_LOCATION
   LOAD_BYTE \reg, r1
.endm

.macro LOAD_IND reg=r0
   GET_IND_LOCATION
   LOAD_BYTE \reg, r1
.endm

.macro LOAD_ZPX reg=r0 // $00,X
   GET_ZPX_LOCATION
   LOAD_BYTE \reg, r1
.endm

.macro LOAD_ZPY reg=r0
   GET_ZPY_LOCATION
   LOAD_BYTE \reg, r1
.endm

.macro LOAD_ABY reg=r0 // $0000,Y
   GET_ABY_LOCATION
   LOAD_BYTE \reg, r1
.endm

.macro LOAD_ABX reg=r0 // $0000,X
   GET_ABX_LOCATION
   LOAD_BYTE \reg, r1
.endm

.macro SAVE_REGS rdst=r0
   .ifc r0, \rdst
   push {r1,r2-r3}
   .endif
   .ifc r1, \rdst
   push {r0,r2-r3}
   .endif
   .ifc r2, \rdst
   push {r0-r1,r3}
   .endif
   .ifc r3, \rdst
   push {r0-r1,r2}
   .endif
.endm


.macro RESTORE_REGS rdst=r0
   .ifc r0, \rdst
   @ ldr   r1,=IO_REGS>>3
   @ mov   tregs,r1
   pop {r1,r2-r3}
   .endif
   .ifc r1, \rdst
   @ ldr   r0,=IO_REGS>>3
   @ mov   tregs,r0
   pop {r0,r2-r3}
   .endif
   .ifc r2, \rdst
   @ ldr   r0,=IO_REGS>>3
   @ mov   tregs,r0
   pop {r0-r1,r3}
   .endif
   .ifc r3, \rdst
   @ ldr   r0,=IO_REGS>>3
   @ mov   tregs,r0
   pop {r0-r1,r2}
   .endif
.endm


.macro LOAD_BYTE rdst=r0 rsrc=r1
   @ .print "load byte src: \rsrc dst: \rdst"
   @ rsrc is always r1
   
   push {r0}
   ldr   r0,=IO_REGS>>4    @ load tregs here because it gets cloberred somewhere
   mov   tregs,r0
   pop {r0}

   lsr \rdst, \rsrc, #4
   cmp   \rdst, tregs   // need to compare with 0x9000>>4
   bne   1f

   SAVE_REGS \rdst
   mov r0, \rsrc
   blx io_read
   .ifnc r0, \rdst
   mov \rdst, r0
   .endif
   RESTORE_REGS \rdst
   b 2f
1: 
   ldrb \rdst,[ rmem, \rsrc]
2:
.endm

.macro STORE_BYTE_ONLY
   push {r0-r3}
   lsr   r2, r1, #4
   ldr   r3,=IO_REGS>>4    @ load tregs here because it gets cloberred somewhere
   mov   tregs,r3
   cmp   r2, tregs   // need to compare with 0x9000>>3
   bne   1f
   blx   io_write
   pop   {r0-r3}
   b 2f
1:
   pop   {r0-r3}
   strb r0,[ rmem, r1]
2:
.endm

.macro STORE_BYTE inc=1 rsrc=r0 rdst=r1
   push  {r2-r3}
   lsr   r2, \rdst, #4
   ldr   r3,=IO_REGS>>4    @ load tregs here because it gets cloberred somewhere
   mov   tregs,r3
   cmp   r2, tregs   // need to compare with 0x9000>>3
   bne   1f
   pop   {r2-r3}
   push  {r0-r3}

   // rdst is  r1 or R0 for ABS
   // rsrc can be r0, r3, r4

   // rdst should go in r1
   // rsrc should go in r0
   .ifnc r1, \rdst
      mov r1, \rdst
   .endif
   .ifnc r0, \rsrc
      mov r0, \rsrc
   .endif

   blx   io_write
   pop   {r0-r3}
   
   NEXT_INSTRUCTION \inc noalign
1:
   pop   {r2-r3}
.ifc \rsrc,rXreg
   push {r1}
   mov   r1,rXreg
   strb  r1,[ rmem, \rdst ]
   pop {r1}
.else
   strb  \rsrc,[ rmem, \rdst ]
.endif
   
   NEXT_INSTRUCTION \inc
.endm


/* branches */
.macro BRANCH condition inc=0 zero=nozero
   B\condition 1f
   NEXT_INSTRUCTION 1+\inc noalign
1:
.ifc \zero,nozero
   mov r0,#\inc
.endif
   ldrsb r0, [rPC,r0 ]  @ load a signed operand
   add   rPC, r0        @ ignore case of wrapping at 0xffff
   NEXT_INSTRUCTION 1+\inc
.endm

/* jumps and returns */

.macro INTR vector bflag
   mov   r0, rSP                 @ put stack pointer in r0
   sub   r0, #3                  @ subtract 3 to get stack pointer after BRK
   mov   rSP, r0                 @ put back into rSP
   sub   rPC, rmem               @ get the 6502 PC address
   strb  rPC, [r0, #2]           @ store low byte of pc into stack
   LSR   rPC, rPC,#8             @ shift pc right by 8 to get high byte
   strb  rPC, [r0, #3]           @ store high byte of pc into stack pointer
   mov   rPC, r0                 @ rPC now becomes the stack pointer ?
   @ push pByte in 6502 format
   statustoR rPC \bflag
   @statustoR r0 \bflag             @ put status register into r0

   strb  rPC, [r0,#1]            @ store status register in stack

   mov   r1, rPbyteLo
   mov   r0, #pByteIflag         @ BRK sets interrupt disable just as IRQ would
   orr   r1, r0
   mov   r0, #pByteDflag
   bic   r1, r0
   mov   rPbyteLo, r1            @ I set B clear and clear DEC flag

   ldr   rPC, =0x10000-\vector   @ put the vector into PC
   ldrh  rPC, [rPC, rmem]        @ put address at vector into PC // vectors are aligned so we can use ldrh

   @ push {r0-r3}
   add   rPC, rmem               @ add the base address to it
.endm

/* more stack operations */

.macro pushbyte reg=r0 wrap=nowrap
   mov   r1, rSP
   strb  \reg, [r1]
   sub   r1, #1
.ifc \wrap,wrap
   sub   r1,rmem
   cmp   r1,#255
   BNE   1f
   mov  r1,#0xff
   lsl  r1,#1
   add  r1,#1
1:
   add   r1,rmem
.endif
   mov   rSP, r1
.endm

.macro pullbyte reg
   mov   r1, rSP
   add   r1, #1
   ldrb  \reg, [r1]
   mov   rSP, r1
.endm

.macro NEXT_INSTRUCTION inc align=align
   ldrb  r0, [rPC, #\inc]
   add   rPC, #\inc+1    @ no auto increment in thumb

#ifdef TRACE6502
   BL   trace6502code
#else
   LSL   r0, #I_ALIGN_BITS
   add   r0, r0,insttable
   bx    r0
#endif
.ifc \align,align
   .ltorg
   INSTALIGN
.endif
.endm

.macro LOGICAL inc logic
   \logic rAcc, rAcc, r0
   SET_FLAGS_NZ
   NEXT_INSTRUCTION \inc
.endm

.macro NOP inc align=align
   NEXT_INSTRUCTION \inc \align
.endm

.macro RMB bitnum
   LOAD_ZP
   mov   r1,#1<<\bitnum
   bic   r0, r0, r1
   GET_ZP_LOCATION //reload the address as R1 has been corrupted
   STORE_BYTE
.endm

.macro SMB bitnum
   LOAD_ZP
   mov    r1, #1<<\bitnum
   orr    r0, r0, r1
   GET_ZP_LOCATION //reload the address as R1 has been corrupted
   STORE_BYTE
.endm

.macro BBR bitnum
   LOAD_ZP
   LSR   r0, r0, #\bitnum+1
   BRANCH CC 1
.endm

.macro BBS bitnum
   LOAD_ZP
   LSR   r0, r0, #\bitnum+1
   BRANCH CS 1
.endm

.macro ASL6 inc=1 reg=r0
// unsigned input
   lsl   \reg,\reg,#1
   lsr   rCarry, \reg, #8
   SET_FLAGS_NZ \reg
   STORE_BYTE \inc
.endm

.macro ROL6 inc=1 reg=r0
//unsigned input
   SETUP_CARRY
   adc   \reg,\reg,\reg  // corrupts V flag ( clears )
   lsr   rCarry, \reg, #8
   SET_FLAGS_NZ \reg
.ifc \reg,r0
   STORE_BYTE \inc
.else
   uxtb  \reg,\reg
   NEXT_INSTRUCTION \inc
.endif
.endm

.macro LSR6 inc=1
   //NB unsigned input in rCarry as that sets it up for free
  // mov   rCarry, \reg // Save new carry
   lsr   r0,rCarry, #1
   SET_FLAGS_NZ r0
   STORE_BYTE \inc r0
.endm

.macro ROR6 inc reg=r0
   // NB unsigned input register
   lsl   rCarry, rCarry, #8
   orr   \reg, \reg, rCarry
   LSR   \reg,\reg,#1
   SAVE_CARRY
   SET_FLAGS_NZ \reg
.ifc \reg,r0
   STORE_BYTE \inc
.else
   uxtb  \reg,\reg
   NEXT_INSTRUCTION 0
.endif
.endm

.macro CMP6 inc=1 reg=rAcc
.ifc \reg,rXreg
   mov   r1, rXreg
   sub   rflagsNZ, r1, r0
.else
   sub   rflagsNZ, \reg, r0
.endif
   SET_FLAGS_NZ rflagsNZ // setup N flag nb needed because 255-1
   SAVE_CARRY
   NEXT_INSTRUCTION \inc
.endm

.macro ADC6 inc=1 bounce=nobounce decbounce=nodecbounce
   mov  r1,rPbyteLo
   LSL  r1,#24+4
.ifc \decbounce,nodecbounce
   bmi  2f
.else
   bmi   \decbounce
.endif
   CLEARV_PART2
   mvn   r0, r0
   lsl   r0, r0, #24
   mvn   r0, r0  // set lower bits to 0xzzFFFFFF
   lsl   rAcc, rAcc, #24
   SETUP_CARRY rCarry
   adc   rAcc, rAcc, r0
   SAVE_VFLAG_FAST
   SAVE_CARRY
   lsr   rAcc, rAcc, #24
   SET_FLAGS_NZ
.ifc \bounce,nobounce
   NEXT_INSTRUCTION \inc noalign
.else
   b  \bounce
.endif

.ifc \decbounce,nodecbounce
2:
.ifc \inc,1
   BL adc_decimal
.else
   BL adc_decimal2
.endif

.endif
   INSTALIGN
.endm

.macro SBC6 inc=1 bounce=nobounce decbounce=nodecbounce
   mov  r1,rPbyteLo
   LSL  r1,#24+4
.ifc \decbounce,nodecbounce
   Bmi  2f
.else
   Bmi  \decbounce
.endif
   CLEARV_PART2
   lsl   r0, r0, #24
   lsl   rAcc, rAcc, #24
   SETUP_CARRY
   sbc   rAcc, rAcc, r0
   SAVE_VFLAG_FAST
   SAVE_CARRY
   lsr   rAcc, rAcc, #24
   SET_FLAGS_NZ
.ifc \bounce,nobounce
   NEXT_INSTRUCTION \inc noalign
.else
   b  \bounce
.endif

.ifc \decbounce,nodecbounce
2:
.ifc \inc,1
   BL sbc_decimal
.else
   BL sbc_decimal2
.endif

.endif
   INSTALIGN
.endm

// BIT
// flags N bit = bit7
//       v bit = bit 6
// flags Z bit = r0 & ACC

.macro BIT inc=1

   mov   r1, #pByteVflag
   mov   rflagsNZ, rPbyteLo
   bic   rflagsNZ, rflagsNZ, r1  // clear Vflag
   and   r1,r1,r0  // get bit 6
   orr   rflagsNZ, r1
   mov   rPbyteLo, rflagsNZ

   LSR   rflagsNZ, R0, #7 // set up N flag
   LSL   rflagsNZ, #8 // ( clear lower 8 bits)

   and   R0, r0, rAcc
   orr   rflagsNZ, r0 // if r0 is zero the rflagsNZ will be zero

   NEXT_INSTRUCTION \inc
.endm
