#ifndef USART_LIB
#define USART_LIB

#include <avr/io.h>
#include <string.h>

#include "usart.h"
#include "common.h"

void USART0_init(uint16_t baudrate)
{
	uint16_t baudrate_reg = (float)(F_CPU * 64 / (16 * (float) baudrate)) + 0.5;
	
	PORTB.DIR &= ~PIN3_bm;  // set PB3 as input
	PORTB.DIR |= PIN2_bm;   // set PB2 as output
	
	USART0.BAUD = baudrate_reg;
	USART0.CTRLB |= USART_TXEN_bm;
}

void USART0_sendChar(char c)
{
	while (!(USART0.STATUS & USART_DREIF_bm))
	{
		;
	}
	USART0.TXDATAL = c;
}

void USART0_sendString(char *str)
{
	for(size_t i = 0; i < strlen(str); i++)
	{
		USART0_sendChar(str[i]);
	}
}

#endif