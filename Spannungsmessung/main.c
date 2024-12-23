#include "common.h"

#include <avr/io.h>
#include <avr/interrupt.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <util/delay.h>
#include <string.h>

#include "usart.h"
#include "adc.h"
#include "i2c.h"
#include "oled.h"
#include "sync_trigger.h"

struct Messung{
	uint16_t spannung_mv;
	uint16_t frequenz_hz;
};

volatile struct Messung messung;

void setup(){
	// CPU-Takt auf 20 MHz einstellen
	CPU_CCP = 0xD8;
	CLKCTRL_MCLKCTRLB = 0x00;
	
	// Peripherie Initialisieren
	adc_init();
	old_init();
	USART0_init(9600);
	sync_trigger_send_init();
	sync_trigger_rcv_init();
	
	// enable interrupts
	sei();
	
	// Display einmal l?schen
	old_clear(0x00);
};

int main(void) {
	setup();

	// main loop
	while (1) {
		
		// gebe aktuellen status auf uart aus
		cli();	// deaktiviere interrupts um konsistente werte zu gewaehrleisten
		char adc1_ergebnis_string[30];
		sprintf(adc1_ergebnis_string, "Channel 1: %d mV / %d Hz \r\n", messung.spannung_mv, messung.frequenz_hz);
		sei(); // aktivere interrupts wieder 
		USART0_sendString(adc1_ergebnis_string);
		
		// Pause 1s
		_delay_ms(1000);
		
		
	}
}

/************************************************************************/
/* Interrupt Handling                                                   */
/************************************************************************/

#define ADC1_OFFSET 2.5
#define ADC1_VREF 5

// interrupt that is triggered when ADC sampling finished
ISR(ADC1_RESRDY_vect){
	uint16_t adc_wert = adc_lese_asynchron();
	if (adc_wert != -1) {
		float spannung = ((float)adc_wert * ADC1_VREF / 1024) - ADC1_OFFSET;
		messung.spannung_mv = 1000 * spannung;
	}
}

// interrupt when sync trigger is received on PA5
ISR(PORTA_PORT_vect){
	// clear interrupt flag
	PORTA.INTFLAGS |= PORT_INT5_bm;
	
	// starte adc
	adc_trigger_asynchron();
}