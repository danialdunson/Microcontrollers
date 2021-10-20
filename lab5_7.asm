;******************************************************************************
; File name: lab5_7.asm
; Author: Danial Dunson
; Last Modified On: 12 Mar 2021
; Purpose: write to the i/o port by writing to a specific range in memory
;******************************************************************************
                                                                                                                         */ 
;*********************************INCLUDES*************************************
.include "ATxmega128a1udef.inc"
;.include "Shepard.inc"
;******************************END OF INCLUDES*********************************
;******************************DEFINED SYMBOLS*********************************
;helpful clock symbols
.equ PRE1S = 64
.equ DELAY_PER_INV = 1 ;1/20= 50ms
.equ ADJUSTMENT_PER_INV = 1666 ; 1/.0006
.equ F_CPU = 2000000
.equ DEBOUCE_PER_INV = 5 ; 1/100=10ms
.equ StringLength = 10
.equ Prompt = '?'
.equ LF = 10 ; Line feed. Could also use '/n'
;**************************END OF DEFINED SYMBOLS******************************
;********************************MAIN PROGRAM**********************************
;.equ STRING_START = 0x2000
;******************************END OF EQUATES**********************************
;***************************MEMORY CONFIGURATION*******************************

.cseg

; configure the reset vector 
.org 0x0
	rjmp MAIN
.org USARTD0_RXC_vect
	rjmp USARTD0_RXC_ISR
;*************************END OF MEMORY CONFIGURATION**************************
;********************************MAIN PROGRAM**********************************
.cseg
; place main program after interrupt vectors 
.org 0x100
MAIN:

;initialize stack pointer
	ldi r16, 0xff
	sts CPU_SPL, R16
	LDI R16, 0X3F
	sts CPU_SPH, r16

;initialize USART
	rcall LED_INIT
	rcall INIT_USART_PINS
	rcall USART_INIT
;enable inturrupts
	ldi r16, 1
	sts PMIC_CTRL, R16
	ldi r16, 1<<7
	sts CPU_SREG, R16

;test loop
LED_LOOP:
	LDI R16, 1<<6
	STS PORTD_OUTTGL, R16
	rjmp LED_LOOP
	
/*TEST:
; Point Y to the INTERNAL_SRAM_START 
	rcall POINT2STARTSTRING
;get input
	rcall IN_STRING
;ECHO
	rcall POINT2STARTSTRING
	rcall OUT_STRING2
	rjmp TEST*/
DONE:
	rjmp DONE


;******************************INITIALIZATIONS**********************************
;******************************************************************************
; Name: LED_INIT 
; Purpose: SETS UP THE IO FOR THE BLUE LED. (ALOW)
; REGS USED: r16
;******************************************************************************
LED_INIT:
;preserve reg
	push r16
;PORTD OUTPUT off and ready
	ldi R16, 1<<6;BLUE
	STS PORTD_OUTCLR, R16
	sts portd_dirset, r16
;restore
	pop r16
	ret
;******************************************************************************
; Name: POINT2STARTSTRING 
; Purpose: Points Y pointer to start of string
; REGS USED: r16
;******************************************************************************
POINT2STARTSTRING:
;preserve reg
	push r16
; Point Y to the INTERNAL_SRAM_START 
	ldi r16, byte3(INTERNAL_SRAM_START)
	sts CPU_RAMPY, r16
	ldi YH, high(INTERNAL_SRAM_START)
	ldi YL, low(INTERNAL_SRAM_START)
;restore
	pop r16
	ret
;******************************************************************************
; Name: USART_INIT 
; Purpose: Initialize USARTD0'S TX and RX
;	250,000 bps (baudrate)
;	
; Config: odd parity, 8bit data, 1 start & 1 end bit, 
; REGS USED: USARTD0_BAUDCTRLA, USARTD0_BAUDCTRLB, USARTD0_CTRLA, USARTD0_CTRLC
; Input(s): N/A
; Output: N/A
;******************************************************************************
USART_INIT:
.EQU BSel = 0 
.EQU BScale = 0
;preserve registers
	push r16

;initialize baud rate
;set USARTD0_BAUDCTRLA w/ ls 8bits
	ldi r16, low(BSel)
	sts USARTD0_BAUDCTRLA, R16
;set USARTD0_BAUDCTRLB to BScale | BSel. nibbles are flipped
	ldi r16, high( (BScale<<4) | high(BSel) )
	sts USARTD0_BAUDCTRLB, R16
;set parity
	ldi r16, (	USART_CHSIZE_8BIT_gc | \
				USART_CMODE_ASYNCHRONOUS_gc | \
				USART_PMODE_ODD_gc  )
	sts USARTD0_CTRLC, R16

;enable RXCIF_INT (low level)
	ldi r16, USART_RXCINTLVL_LO_gc
	sts USARTD0_CTRLA, R16
;enable TX and RX
	ldi r16, 0b00011100
	sts USARTD0_CTRLB, r16

; recover relevant registers and return
	pop r16 ;restoring the r16 reg 
	ret

;******************************************************************************
; Name: INIT_USART_PINS 
; Purpose: To initialize the relevant USART PINS.
; Pins: [PD3] PortD0 TX, [PD2] PortD0 RX, 
; Input(s): N/A
; Output: N/A
;******************************************************************************
INIT_USART_PINS:
.equ BIT3 = 1<<3 ;putting equates here makes it modular!
.equ BIT2 = 1<<2

;preserve registers
	push r16
;set TX to '1' ('idle')-OUTPUT
	ldi r16, BIT3
	sts PORTD_OUTSET, r16
	sts PORTD_DIRSET, r16
;set PORTD_PIN2 = INPUT RX
	ldi r16, BIT2
	sts PORTD_OUTCLR, R16
	sts PORTD_DIRCLR, R16
;recover registers
	pop r16
;return
	ret
;******************************************************************************
; Name: OUTCHAR 
; Purpose: Outputs a single character to the transmit pin 
;			of the chosen USART module.
; Pins: [PD3] PortD0 TX, [PD2] PortD0 RX, 
; Input(s): N/A
; Output: N/A
;******************************************************************************
OUTCHAR:
	push r17
OUTCHARLOOP:
;check if there is a transmission in progress?
	lds r17, USARTD0_STATUS
	sbrs r17, USART_DREIF_bp
	rjmp OUTCHARLOOP
;recover r16(input parameter)
	pop r17
;Send the 
	STS USARTD0_DATA, R16
;return
	ret
;******************************************************************************
; Name: IN_CHAR 
; Purpose: To initialize the relevant USART PINS.
; Pins: [PD3] PortD0 TX, [PD2] PortD0 RX, 
; Input(s): N/A
; Output: R16
;******************************************************************************
IN_CHAR:
;preserve registers
	push r17
POLL_RX:
	ldS r17, USARTD0_STATUS
	sbrS r17, USART_RXCIF_bp
	rjmp POLL_RX
;get input
	lds r16, USARTD0_DATA

;recover registers
	pop r17
;return
	ret
;******************************************************************************
; Name: OUT_STRING 
; Purpose: OUTPUT THE STRING STARTING FROM Z MUST BE INITIALIZED PRIOR TO CALL
;	
; Input(s): N/A
; Output: N/A
;******************************************************************************
OUT_STRING:
;preserve registers
	push r16
STRING_LOOP:
;load value of reg
	ELPM r16, Z+
	cPI r16, 0x00
	breq STRING_DONE 
	rcall OUTCHAR
	rjmp STRING_LOOP

STRING_DONE:
; recover relevant registers
	pop r16
; return from subroutine
	ret
;******************************************************************************
; Name: OUT_STRING2 
; Purpose:OUTPUT THE STRING STARTING FROM Y MUST BE INITIALIZED PRIOR TO CALL
; RCALLS: OUTCHAR
; REGS USED: R16, See "OUTCHAR: "
; Input(s): N/A
; Output: N/A
;******************************************************************************
OUT_STRING2:
;preserve registers
	push r16
STRING_LOOP2:
;load characters
	ld r16, y+
	cPI r16, 0x00 ;Check4null
	breq STRING_DONE2
	
	rcall OUTCHAR
	rjmp STRING_LOOP2

STRING_DONE2:
; recover relevant registers
	pop r16
; return from subroutine
	ret
;******************************************************************************
; Name: IN_STRING 
; Purpose: Initialize USARTD0'S TX and RX
;	250,000 bps (baudrate)
;	
; Config: odd parity, 8bit data, 1 start & 1 end bit, 
; REGS USED: R16. see "IN_CHAR: "
; Input(s): N/A
; Output: N/A
;******************************************************************************
IN_STRING:
;preserve registers
	push r16
STRING_INCHAR:
;read data on buffer
	rcall IN_CHAR ;OUTPUT TO R16
	cpi r16, 0x0D ;CR
	breq CARRIAGE
;if DEL OR BS, don't save to string!
	cpi r16, 0x1b;DEL
	breq NOCHAR
	cpi r16, 0x08;BS
	breq NOCHAR
;else save string and inc y
	st y+, r16
;loop back to
	rjmp STRING_INCHAR
NOCHAR:
;decrement pointer
	ld r16, -y
	rjmp STRING_INCHAR
CARRIAGE:
;store null for enter
	ldi r16, 0
	st Y, r16
; recover relevant registers
	pop r16
; return from subroutine
	ret
;******************************************************************************
; Name: USARTD0_RXC_ISR 
; Purpose: Initialize USARTD0'S TX and RX
;	250,000 bps (baudrate)
;	
; Config: odd parity, 8bit data, 1 start & 1 end bit, 
; REGS USED: USARTD0_DATA
; Input(s): N/A
; Output: N/A
;******************************************************************************
USARTD0_RXC_ISR:
;preserve registers
	push r16
;read data (clears flag)
	lds r16, USARTD0_DATA
	rcall OUTCHAR
; recover relevant registers
	pop r16
; return from subroutine
	reti
;****************************** END INITIALIZATIONS*******************************

