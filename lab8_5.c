/*
 * lab8_5.c
 *
 * Created: 4/9/2021 1:56:18 PM
 *  Author: Danial Dunson
 */ 
#include <avr/io.h>
#include <avr/interrupt.h>
#include "coswave.h"
#include "usart.h"

extern void clock_init();
//function prototypes.
void dma_init(void);
void dac_init(void);
void tcc_init(void);
void event_init(void);
void analog_backpack_io_init(void);
void keypress(void);
void tc_start(void);
void dma_init_pointywave(void);
//global variables (for interrupts)
volatile char key;

#define key_c			255000*1.026
#define key_csharp		255000*1.095
#define key_d			255000*1.164
#define key_dsharp		255000*1.220
#define key_e			255000*1.300
#define key_f			255000*1.380
#define key_fsharp		255000*1.460
#define key_g			255000*1.56798
#define key_gsharp		255000*1.650
#define key_a			255000*1.750
#define key_asharp		255000*1.850
#define key_b			255000*1.955


uint8_t mode = 0;

int main(void)
{
	clock_init(); //set clock to 32MHz
	dma_init(); //initialize the DMA system.
	dac_init(); //initialize DAC
	tcc_init(); //initialize Timer.
	analog_backpack_io_init(); //Configure IO
	event_init();
	usartd0_init();
	
	PMIC.CTRL = PMIC_LOLVLEN_bm | PMIC_MEDLVLEN_bm;
	sei();
	
//prime DMA
	DMA.CH1.CTRLA |= DMA_CH_ENABLE_bm;
	DMA.CH1.CTRLA |= DMA_CH_TRFREQ_bm;
//Main
	while (1)
	{
	keypress();
	}
}

void dac_init(void){

	DACA.CTRLB = DAC_CHSEL_SINGLE1_gc | DAC_CH1TRIG_bm; 
	DACA.CTRLC = DAC_REFSEL_AREFB_gc; //RIGHT ADJUSTED BY DEFAULT....not setting the LEFTADJ bit. applies to both channels.
//dac_trg CFG
	DACA.EVCTRL = DAC_EVSEL_1_gc;
	
	DACA.CTRLA = DAC_CH1EN_bm;
	DACA.CTRLA |= DAC_ENABLE_bm; //Enabling the DAC.
}

void dma_init(void){
	DMA.CTRL |= DMA_RESET_bm;
	//set the burst length
	DMA.CH1.CTRLA = DMA_CH_SINGLE_bm | DMA_CH_BURSTLEN_2BYTE_gc  | DMA_CH_REPEAT_bm; //uart data reg empty being set is the
	
	//SOURCE RELOAD WHEN DOES THE SOURCE ADDRESS GO BACK TO THE BEGINNING. WE DO BLOCK FOR THE SHITL
	DMA.CH1.ADDRCTRL = DMA_CH_SRCRELOAD_BLOCK_gc | DMA_CH_SRCDIR_INC_gc |
					DMA_CH_DESTRELOAD_BURST_gc | DMA_CH_DESTDIR_INC_gc;
	//DMA.CH1.REPCNT = 127;				
	DMA.CH1.TRIGSRC = DMA_CH_TRIGSRC_DACA_CH1_gc;
	//config size of src
	DMA.CH1.TRFCNT = (uint16_t)( sizeof(mycoswave) );
	
	DMA.CH1.SRCADDR0 = (uint8_t)( (uintptr_t)mycoswave );
	DMA.CH1.SRCADDR1 = (uint8_t)( ((uintptr_t)mycoswave)>>8);
	DMA.CH1.SRCADDR2 = (uint8_t)( ((uint32_t)((uintptr_t)mycoswave))>>16);
	
	DMA.CH1.DESTADDR0 = (uint8_t)((uintptr_t)&DACA.CH1DATA);
	DMA.CH1.DESTADDR1 = (uint8_t)( ((uintptr_t)&DACA.CH1DATA)>>8);
	DMA.CH1.DESTADDR2 = (uint8_t)( ((uint32_t)((uintptr_t)&DACA.CH1DATA))>>16); 

	//enable DMA Channel 0 at the end.
	DMA.CH1.CTRLA |= DMA_CH_ENABLE_bm;	
	//enable DMA peripheral
	DMA.CTRL |= DMA_CH_ENABLE_bm;				
}

void dma_init_pointywave(void){
	DMA.CTRL |= DMA_RESET_bm;
	//set the burst length
	DMA.CH1.CTRLA = DMA_CH_SINGLE_bm | DMA_CH_BURSTLEN_2BYTE_gc  | DMA_CH_REPEAT_bm; //uart data reg empty being set is the
	
	//SOURCE RELOAD WHEN DOES THE SOURCE ADDRESS GO BACK TO THE BEGINNING. WE DO BLOCK FOR THE SHITL
	DMA.CH1.ADDRCTRL = DMA_CH_SRCRELOAD_BLOCK_gc | DMA_CH_SRCDIR_INC_gc |
					DMA_CH_DESTRELOAD_BURST_gc | DMA_CH_DESTDIR_INC_gc;
			
	DMA.CH1.TRIGSRC = DMA_CH_TRIGSRC_DACA_CH1_gc;
	//config size of src
	DMA.CH1.TRFCNT = (uint16_t)( sizeof(triangle) );
	
	DMA.CH1.SRCADDR0 = (uint8_t)( (uintptr_t)triangle );
	DMA.CH1.SRCADDR1 = (uint8_t)( ((uintptr_t)triangle)>>8);
	DMA.CH1.SRCADDR2 = (uint8_t)( ((uint32_t)((uintptr_t)triangle))>>16);
	
	DMA.CH1.DESTADDR0 = (uint8_t)((uintptr_t)&DACA.CH1DATA);
	DMA.CH1.DESTADDR1 = (uint8_t)( ((uintptr_t)&DACA.CH1DATA)>>8);
	DMA.CH1.DESTADDR2 = (uint8_t)( ((uint32_t)((uintptr_t)&DACA.CH1DATA))>>16); 

	//enable DMA Channel 0 at the end.
	DMA.CH1.CTRLA |= DMA_CH_ENABLE_bm;	
	//enable DMA peripheral
	DMA.CTRL |= DMA_CH_ENABLE_bm;				
}

void analog_backpack_io_init(){
	PORTC.OUTSET = PIN7_bm;
	PORTC.DIRSET = PIN7_bm; //for shutdown.	
}

void event_init(){
	EVSYS.CH1MUX = EVSYS_CHMUX_TCC0_OVF_gc;
	//ACTIVATE LENGTH.
	//LENGTH OVERFLOW -> DISABLE TCC0
}

void tcc_init(){
	#define denom 255000*1.56798
	TCC0.CNT = 0;
	TCC0.PER = (32E6) / (denom) ; //less than 2^16 so PRE=1
/*	TCC0.INTCTRLA = TC_OVFINTLVL_LO_gc;*/
/*	TCC0.CTRLD = TC_EVACT_RESTART_gc | TC_EVSEL_CH1_gc;*/ 

	//enable tcc0
	TCC0.CTRLA = TC_CLKSEL_DIV1_gc;
	/////////////////////////////TIMERCOUNTER1 TCC1
	TCC1.CNT = 0;
	TCC1.PER = (32E6)/ (64*10); //100ms OVERFLOW.
	TCC1.INTCTRLA = TC_OVFINTLVL_LO_gc;
/*	TCD1.CTRLD = TC_EVSEL_CH0_GC | TC_EVACT_RESTART_GC;*/
//	TCD1.CCA = (32E6)/ 10; //100MS COMPARE.
/*	TCD1.INTCTRLB = TC_CCAINTLVL_HI_GC;*/
	
	
	
}

ISR(TCC1_OVF_vect){
/*	asm "push r24";*/
	TCC0.PER = TC_CLKSEL_OFF_gc;  //stop dma.
	TCC1.CTRLA = TC_CLKSEL_OFF_gc; //stop length timer.
	
// 	asm "push r24";
// 	asm "reti";
}

ISR(USARTD0_RXC_vect){
/*	asm "push r24";*/
	key = (char) (USARTD0.DATA);
// 	asm "push r24";
// 	asm "reti";
}

void tc_start(void){

		TCC1.CTRLA = TC_CLKSEL_DIV64_gc; //start length timer.
		TCC1.PER = (32E6)/ (64*4);
		TCC1.CNT = 0;
}

void keypress(void){
	switch(key) {
		
		case 's' :
		if((mode++) & (0x01))dma_init_pointywave();
		else dma_init();
		key=0;
		break;
		
		case 'w'  :
		TCC0.PER =(32E6) / (key_c) ;
		tc_start();		
		key=0;
		break;
		
		case '3'  :
		TCC0.PER =(32E6) / (key_csharp) ;
		tc_start();
		key=0;
		break;
		
		case 'e'  :
		TCC0.PER =(32E6) / (key_d) ;
		tc_start();
		key=0;
		break;
		
		case '4'  :
		TCC0.PER =(32E6) / (key_dsharp) ;
		tc_start();
		key=0;
		break;
		
		case 'r'  :
		TCC0.PER =(32E6) / (key_e) ;
		tc_start();
		key=0;
		break;
		
		case 't'  :
		TCC0.PER =(32E6) / (key_f) ;
		tc_start();
		key=0;
		break;
		
		case '6'  :
		TCC0.PER =(32E6) / (key_fsharp) ;
		tc_start();
		key=0;
		break;
		
		case 'y'  :
		TCC0.PER =(32E6) / (key_g) ;
		tc_start();
		key=0;
		break;
		
		case '7'  :
		TCC0.PER =(32E6) / (key_gsharp) ;
		tc_start();
		key=0;
		break;
		
		case 'u'  :
		TCC0.PER =(32E6) / (key_a) ;
		tc_start();
		key=0;
		break;
		
		case '8'  :
		TCC0.PER =(32E6) / (key_asharp) ;
		tc_start();
		key=0;
		break;
		
		case 'i'  :
		TCC0.PER =(32E6) / (key_b) ;
		tc_start();
		key=0;
		break;
	}
}
