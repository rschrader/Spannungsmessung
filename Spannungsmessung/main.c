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


/************************************************************************/
/* DATATYPES                                                            */
/************************************************************************/
#define SAMPLES 10

enum CurrentType{
	AC = 0,
	DC = 1
} ;

static const char *CurrentTypeString[] = {
	"AC", "DC",
};

struct Measurement{
	int16_t buffer[SAMPLES];
	int16_t voltage_mv;
	enum CurrentType current_type;
	uint16_t last_zero_crossing;
	int16_t tick;
};

/************************************************************************/
/* Global Variables                                                     */
/************************************************************************/

volatile struct Measurement measurement;


/************************************************************************/
/* Init Code                                                            */
/************************************************************************/
void setup(){
	// configure CPU to 20Mhz
	CPU_CCP = 0xD8;
	CLKCTRL_MCLKCTRLB = 0x00;
	
	// intialize peripherals
	adc_init();
	USART0_init(11520);
	sync_trigger_send_init();
	sync_trigger_rcv_init();
	old_init();
	
	// enable interrupts
	sei();
};

/*
 * Draw basic oled frame. This is static content and have to be drawn only once.
 */
void basic_oled_frame() {
	uint8_t x = 2;
	
	// clear frame
	old_clear(0);
	
	// draw table
	old_hline(2, 10, 129, 128);
	old_vline(17 ,0, 63, 128);
	old_vline(64 ,0, 63, 128);
	old_vline(95 ,0, 63, 128);
	
	// write table headings
	x += old_string(4,8, (uint8_t*) "L");
	x += old_string(21,8, (uint8_t*) "U(V)");
	x += old_string(68,8, (uint8_t*) "Typ");
	x += old_string(99,8, (uint8_t*) "0(ms)");
}

/************************************************************************/
/* Main Loop Code                                                       */
/************************************************************************/

/*
 * Udpate current values from measurements on serial console. Expected output:
 * 27850: 2495 mV (DC, 999 ms) - [2495 2495 2495 2495 2495 2495 2495 2495 2495 2495 ]
 */
void print_status() {
	char buffer[30]; // char buffer for sprintf
	sprintf(buffer, "%d: %d mV (%s, %d ms) - [", measurement.tick, measurement.voltage_mv, CurrentTypeString[measurement.current_type], measurement.last_zero_crossing);
	USART0_sendString(buffer);
	for(int i = 0; i < SAMPLES; i++){
		sprintf(buffer, "%d ", measurement.buffer[i]);
		USART0_sendString(buffer);
	}
	USART0_sendString("] \r\n");
}

/*
 * Udpate current values from measurements on oled screen
 */
void update_oled_frame() {
	// update L1 table entries
	old_string(4,10, (uint8_t*) "1");										// L
	old_float(21,10, (float)measurement.voltage_mv / 1000);		// U
	old_string(68,10, (uint8_t*) CurrentTypeString[measurement.current_type]);	// Type
	old_intdez(99,10, measurement.last_zero_crossing ,3);			// 0
}

int main(void) {
	setup();
	basic_oled_frame();

	// main loop
	while (1) {
		cli();	// deactivate interrupts to provide consistency
		
		print_status();
		update_oled_frame();
		
		sei(); // activate interrupts after updating
		
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
	uint16_t adc_wert = adc_read_async();
	if (adc_wert != -1) {
		// increment tick
		measurement.tick++;
		
		// read adc measurement
		float spannung = ((float)adc_wert * ADC1_VREF / 1024) - ADC1_OFFSET;
		
		// shift historic measurements
		for(int i = 1; i < SAMPLES; i++){
			measurement.buffer[i -1] = measurement.buffer[i];
		}
		
		// store latest mesaurement at end of array
		measurement.buffer[SAMPLES-1] = 1000 * spannung;
		
		// get minum value and maximum value from buffer
		int16_t spannung_min = INT16_MAX;
		int16_t spannung_max = INT16_MIN;
		for(int i = 0; i < SAMPLES; i++){
			if (measurement.buffer[i] > spannung_max) {
				spannung_max = measurement.buffer[i];
			}
			if (measurement.buffer[i] < spannung_min) {
				spannung_min = measurement.buffer[i];
			}
		}
		
		// check if AC or DC
		if (spannung_max > 0 && spannung_min < 0) {
			measurement.current_type = AC;
		} else {
			measurement.current_type = DC;
		}
		
		// check for polarity switch negative to positive
		if (measurement.buffer[SAMPLES -2] < 0 && measurement.buffer[SAMPLES-1] >=0){
			measurement.last_zero_crossing = 0;
		} 
		else {
			measurement.last_zero_crossing += 5; // ADC Interrupt is expected every 200Hz -> 5 ms increment per tick
			
			// limit last zero crossing to 999ms
			if (measurement.last_zero_crossing > 999) {
				measurement.last_zero_crossing = 999;
			}
		}
		
		// calculate voltage
		measurement.voltage_mv = spannung_max;
	}
}

// interrupt when sync trigger is received on PA5
ISR(PORTA_PORT_vect){
	// clear interrupt flag
	PORTA.INTFLAGS |= PORT_INT5_bm;
	
	// starte adc
	adc_trigger_async();
}