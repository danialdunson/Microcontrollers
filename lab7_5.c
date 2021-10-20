#include <avr/io.h>
#include <avr/interrupt.h>
#include "usart.h"

//function declarations
void adc_init(void);
void tcc0_init(void);
char num2char(int16_t num);
void mag2higherror(uint8_t *str);
void displayhex(int16_t result_s);
void newline(void);

//Global Variables
volatile int16_t result;
volatile uint16_t tempresult;
volatile uint8_t newconversion;
volatile uint8_t serialplot_in;
int16_t notplaying;

int main(void){
	int16_t adc;
	uint16_t base10;
	char outchar;
	volatile prevnumber;
	volatile uint16_t uresult;
	adc_init();
	tcc0_init();
	usartd0_init();
	PORTH.DIRSET = PIN0_bm; //TRIGGER FOR WAVEFORMS OCSCILLISCOPE VIA DAD.
	PORTD.DIRSET = PIN5_bm;
	//ENABLE INTERRUPTS
	PMIC.CTRL = PMIC_LOLVLEN_bm | PMIC_MEDLVLEN_bm;
	sei();

	while(1){
		if( (newconversion) ) {
			char outchar;
			PORTH.OUTTGL = PIN0_bm; //Confirming the output
			
			usartd0_out_char((result & 0x00ff) );
			usartd0_out_char((result & 0xff00)>>8 );
			newconversion = 0;
		}
	}
}




void adc_init(void)
{
	//CH0
	ADCA.CH0.CTRL = (ADC_CH_GAIN_1X_gc | ADC_CH_INPUTMODE_DIFFWGAIN_gc);
	ADCA.CH0.MUXCTRL = ( ADC_CH_MUXNEG_PIN6_gc | ADC_CH_MUXPOS_PIN1_gc );
	//using ADCA WITH GAIN=1.
	ADCA.CTRLA = (ADC_DMASEL_OFF_gc | ADC_ENABLE_bm);
	ADCA.CTRLB = ( ADC_RESOLUTION_12BIT_gc | ADC_CONMODE_bm);
	ADCA.REFCTRL = (ADC_REFSEL_AREFB_gc);
	//enabling the interrupt
	ADCA.EVCTRL = ADC_EVACT_CH0_gc | ADC_SWEEP0_bm | ADC_EVACT_CH0_gc;
	ADCA.CH0.INTCTRL = ADC_CH_INTLVL_LO_gc | ADC_CH_INTMODE_COMPLETE_gc;
	
}

void tcc0_init(void)
{	//change here for easier demo.
	#define clkf 2000000; //NEED A PRESCALER OF 8
	#define prescale 8
	#define invperiod 150 //Section3 = 4Hz
	
	
	uint16_t denom = prescale*invperiod;
	
	TCC0.PER = 2000000/denom;
	TCC0.CNT = 0;
	TCC0.CTRLA = TC_CLKSEL_DIV8_gc;
	/*	TCC0.CTRLD = */
	//SETUP THE EVENT SYSTEM
	EVSYS.CH0MUX = EVSYS_CHMUX_TCC0_OVF_gc;
	
}

char num2char(int16_t num){
	char digitsbro [16] = {0x30,0x31,0x32,0x33,0x34,0x35,0x36,0x37, \
							0x38,0x39,0x41,0x42,0x43,0x44,0x45,0x46};
	char *digitsptr = &digitsbro;
	return *(digitsptr+num);
}


void mag2higherror(uint8_t *str){
	usartd0_out_string2(str);
}

void displayhex(int16_t result_s){
	usartd0_out_char('(');
	usartd0_out_char('0');
	usartd0_out_char('x');
	
	usartd0_out_char(num2char( (result_s & 0x0f00)>>8 ) );
	usartd0_out_char(num2char( (result_s & 0x00f0)>>4 ) );
	usartd0_out_char(num2char( (result_s & 0x000f) ) );
	usartd0_out_char(')');
}

void newline(void){
	usartd0_out_char('\n');
	usartd0_out_char('\r');
}

ISR(ADCA_CH0_vect)
{
	result = ADCA.CH0RES;
	PORTD.OUTTGL = PIN5_bm;
	newconversion=1;
}

ISR(USARTD0_RXC_vect)
{
	serialplot_in = USARTD0.DATA;
	if(serialplot_in == 'E') {ADCA.CH0.MUXCTRL = ( ADC_CH_MUXNEG_PIN5_gc | ADC_CH_MUXPOS_PIN4_gc );}
	if(serialplot_in == 'S') {ADCA.CH0.MUXCTRL = ( ADC_CH_MUXNEG_PIN6_gc | ADC_CH_MUXPOS_PIN1_gc );}
	
}
