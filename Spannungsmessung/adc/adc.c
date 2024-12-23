#include <avr/io.h>
#include "adc.h"


void adc_init(void) {
	// Referenzspannung auf 5 V setzen
	ADC1.CTRLC = 0x53;

	// ADC aktivieren und auf 10-Bit-Aufl?sung einstellen
	ADC1.CTRLA = 0x10;

	// ADC-Prescaler auf 16 einstellen
	ADC1.CTRLC = 0x03;

	// Kanal AIN6 (PC0) als ADC-Eingang w?hlen
	ADC1.MUXPOS = 0x06;
	
	// Interrupts aktivieren
	ADC1.INTCTRL = 0x01;
	
	// ADC aktivieren
	ADC1.CTRLA = 0x01;
}

uint16_t adc_lese_synchron() {
	// Starte eine ADC-Konvertierung
	ADC1.COMMAND = 0x01;
	
	// Warte, bis die Konvertierung abgeschlossen ist
	while (!(ADC1.INTFLAGS & 0x01));
	
	return ADC1.RES;
}

void adc_trigger_asynchron() {
	// Starte eine ADC-Konvertierung
	ADC1.COMMAND = 0x01;
}

int16_t adc_lese_asynchron(){
	if (ADC1.INTFLAGS & 0x01)
		return ADC1.RES;
	else
		return -1;
}

