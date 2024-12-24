#ifndef SYNCTRIGGER_H_
#define SYNCTRIGGER_H_

#include "common.h"
#include <avr/io.h>
#include <avr/interrupt.h>


// configure TCD0 to output 200Hz rect wave on PA4
void sync_trigger_send_init () {
	PORTA.DIR |= PIN4_bm;					// set PA4 as output for trigger signal
	CPU_CCP = CCP_IOREG_gc;					// enable write protected register
	TCD0.FAULTCTRL = TCD_CMPAEN_bm;			// enable channel A
	
	
	TCD0_CTRLB |= TCD_WGMODE_DS_gc;			// use one ramp wave generation mode
	
	TCD0_CMPBCLR |= 390;					// max value -> 200.32 Hz
	TCD0_CMPASET |= 195;					// 50 % duty cycle on CMPA
	TCD0_CMPBSET |= 196;			 		// 50 % duty cycle on CMPA			
	
	while(!(TCD0.STATUS & TCD_ENRDY_bm))	// wait until ENRDY flag is ready
	{
		;
	}
	
	TCD0_CTRLA |= TCD_CLKSEL_20MHZ_gc		// use 20 Mhz clock
			   | TCD_CNTPRES_DIV32_gc 
			   | TCD_SYNCPRES_DIV8_gc		// set prescalers to 32*8 = 256
	           | TCD_ENABLE_bm;				// enable timer
	
};

void sync_trigger_rcv_init() {
	//configure PA5
	PORTA.DIR &= ~PIN5_bm;					// set PA5 input
	PORTA.PIN5CTRL |= PORT_PULLUPEN_bm;		// enable pullup
	PORTA.PIN5CTRL |= PORT_ISC_RISING_gc;	// enable interrupt on rising edge
}

#endif /* SYNCTRIGGER_H_ */