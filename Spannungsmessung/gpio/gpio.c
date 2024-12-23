#include <avr/io.h>
#include "gpio.h"

void gpio_init(){
		// Setzt die I/O-Pins als Ausg?nge
		PORTA.DIR |= 0x0F;

		// Setzt die I/O-Pins auf "0"
		PORTA.OUT &= ~0x0F;
}