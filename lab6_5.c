/************************************************************************
	lab6_5.c
	
  Description:
    Fetches new Data from the IMU the lsm module signals the relevant
	interrupt.
  
  Author: Danial Dunson                                                                  
************************************************************************/
#include <avr/interrupt.h>
#include <avr/io.h>
#include "spi.h"
#include "lsm6dsl.h"
#include "lsm6dsl_registers.h"
#include "usart.h"


volatile uint8_t accel_flag; //alerts that data is ready*/


int main(void){
	lsm6dsl_data_t lsm_data; 

	spi_init();
	usartd0_init();
	LSM6DSL_init();
	sei();
	while(1){
/************************************************************************/
/*		IF Statement: SOFTWARE FLAG FETCH NEW IMU DATA-- & SEND IT BROTHER                                                                   */
/************************************************************************/
		if ( (accel_flag=1) )
		{
			accel_flag = 0;
			uint8_t ADDREQ;
			ADDREQ = (0x22)|LSM6DSL_SPI_READ_bm;

			PORTF.OUTCLR = SPI_SS; //enable slave
			volatile uint8_t IMURAW[13];
//fetch that IMU data.
			uint8_t i;
			for (i=0;i<14;i++) //function call = slower though
			{
				if (i<2) { //send first 2 address requests
					while( !(SPIF.STATUS& 1<<7) ); //Wait to start next read cycle
					SPIF.DATA = ADDREQ;
					ADDREQ++;
					continue; 
				}
				if ((i<12)){
					if( (SPIF.STATUS& 1<<7) ) //Wait to start next read cycle
					{
						IMURAW[i-2] = SPIF.DATA; //read x-1 b4 tx of x+1
						SPIF.DATA = ADDREQ;

						ADDREQ++;}
					else i--;
					
				}
				else{
					if( (SPIF.STATUS& 1<<7) ) //Wait to start next read cycle
					{
						IMURAW[i-2] = SPIF.DATA; //read x-1 b4 tx of x+				
						SPIF.DATA = 0x8F;
						ADDREQ++;}
					else i--;	
				}	
			}
					 	
			PORTF.OUTSET = SPI_SS; //disable slave
/************************************************************************/
/*                     SEND THAT DATA TO THE MOON BROTHER*/                                               
/************************************************************************/

			
			usartd0_out_char(IMURAW[6]);
			usartd0_out_char(IMURAW[7]);
			usartd0_out_char(IMURAW[8]);
			usartd0_out_char(IMURAW[9]);
			usartd0_out_char(IMURAW[10]);
			usartd0_out_char(IMURAW[11]);
//gyro data. will enable in demo.

// 			usartd0_out_char(IMURAW[0]);
// 			usartd0_out_char(IMURAW[1]);
// 			usartd0_out_char(IMURAW[2]);
// 			usartd0_out_char(IMURAW[3]);
// 			usartd0_out_char(IMURAW[4]);
// 			usartd0_out_char(IMURAW[5]);

 			PORTC.INTCTRL = PORT_INT0LVL_LO_gc; //re-enable NEWDATA interrupt.
			} //END IF SOFTWARE FLAG
		else ;	
		}//END OF WHILE LOOP.
			
}

ISR(PORTC_INT0_vect){
	accel_flag = 1;
	PORTC.INTCTRL = 0; //disable int.
	//PORTC.INTFLAGS=0b00000001;	
}
/************************************************************************/
/*								END OF MAIN                                                                     */
/************************************************************************/