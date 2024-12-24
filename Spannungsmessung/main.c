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
#include "sync_trigger.h"
#include "oled.h"

#define SAMPLES 10

enum CurrentType{
	AC = 0,
	DC = 1
} ;

static const char *CurrentTypeString[] = {
	"AC", "DC",
};

struct Messung{
	int16_t buffer[SAMPLES];
	int16_t spannung_mv;
	enum CurrentType current_type;
	uint16_t last_zero_crossing;
	int16_t tick;
};

volatile struct Messung messung;

void setup(){
	// CPU-Takt auf 20 MHz einstellen
	CPU_CCP = 0xD8;
	CLKCTRL_MCLKCTRLB = 0x00;
	
	// Peripherie Initialisieren
	adc_init();
	USART0_init(11520);
	sync_trigger_send_init();
	sync_trigger_rcv_init();
	old_init();
	
	// enable interrupts
	sei();
};

void print_status() {
	// gebe aktuellen status auf uart aus
	char buffer[30];
	sprintf(buffer, "%d: %d mV (%s, %d ms) - [", messung.tick, messung.spannung_mv, CurrentTypeString[messung.current_type], messung.last_zero_crossing);
	USART0_sendString(buffer);
	for(int i = 0; i < SAMPLES; i++){
		sprintf(buffer, "%d ", messung.buffer[i]);
		USART0_sendString(buffer);
	}
	USART0_sendString("] \r\n");
}

void basic_oled_frame() {
	uint8_t x = 2;
	
	// clear frame
	old_clear(0);
	
	
	old_hline(2, 10, 129, 128);
	old_vline(17 ,0, 63, 128);
	old_vline(64 ,0, 63, 128);
	old_vline(95 ,0, 63, 128);
	
	// write table
	x += old_string(4,8, "L");
	x += old_string(21,8, "U(V)");
	x += old_string(68,8, "Typ");
	x += old_string(99,8, "0(ms)");
}

void update_oled_frame() {
		// write px to clear registers
		old_pix(2,0,0);
		char buffer[10];
		
		// update L1
		old_string(4,10, "1");		// L
		old_float(21,10, (float)messung.spannung_mv / 1000);	// U
		old_string(68,10, CurrentTypeString[messung.current_type]);	// Type
		old_intdez(99,10, messung.last_zero_crossing ,3);	// 0
}

int main(void) {
	setup();
	basic_oled_frame();

	// main loop
	while (1) {
		cli();	// deaktiviere interrupts um konsistente werte zu gewaehrleisten
		
		print_status();
		update_oled_frame();
		
		sei(); // aktivere interrupts wieder
		
		// Pause 1s
		_delay_ms(100);
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
		// increment tick
		messung.tick++;
		
		float spannung = ((float)adc_wert * ADC1_VREF / 1024) - ADC1_OFFSET;
		int16_t spannung_min = INT16_MAX;
		int16_t spannung_max = INT16_MIN;
		
		// shift historic measurements
		for(int i = 1; i < SAMPLES; i++){
			messung.buffer[i -1] = messung.buffer[i];
		}
		
		// store latest mesaurement at end of array
		messung.buffer[SAMPLES-1] = 1000 * spannung;
		
		// get minum value and maximum value from buffer
		for(int i = 0; i < SAMPLES; i++){
			if (messung.buffer[i] > spannung_max) {
				spannung_max = messung.buffer[i];
			}
			if (messung.buffer[i] < spannung_min) {
				spannung_min = messung.buffer[i];
			}
		}
		
		// check if AC or DC
		if (spannung_max > 0 && spannung_min < 0) {
			messung.current_type = AC;
		} else {
			messung.current_type = DC;
		}
		
		// check for polarity switch negative to positive
		if (messung.buffer[SAMPLES -2] < 0 && messung.buffer[SAMPLES-1] >=0){
			messung.last_zero_crossing = 0;
		} 
		else {
			messung.last_zero_crossing += 5; // 200Hz -> 5 ms per tick
			if (messung.last_zero_crossing > 999) {
				messung.last_zero_crossing = 999;
			}
		}
		
		// calculate voltage
		messung.spannung_mv = spannung_max;
	}
}

// interrupt when sync trigger is received on PA5
ISR(PORTA_PORT_vect){
	// clear interrupt flag
	PORTA.INTFLAGS |= PORT_INT5_bm;
	
	// starte adc
	adc_trigger_asynchron();
}