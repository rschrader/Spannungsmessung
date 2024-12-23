/* Bibliothek mit Funktionen für ein ST7735-Display mit I2C-Schnittstelle
   Achtung: x = 2..129, y = 0..63, 0 ist oben */

#include <avr/pgmspace.h>
#include <math.h>
#include "Font5x8udzz.h"

void old_init()
{
	i2c_start();
	i2c_write(0x78);
	i2c_write(0);		//last command
	i2c_write(0xAF);	//display on, RAM not cleared
	i2c_stop();
}	

void old_clear(uint8_t ptt)
{
	uint8_t x, y;
	for(y = 0; y < 8; y++)
	{
		i2c_start();
		i2c_write(0x78);
		i2c_write(0x80);		//not last control byte, command follows
		i2c_write(0xB0 + y);	//set page
		i2c_write(0x80);
		i2c_write(0x00);		//low nibble of column address = 0
		i2c_write(0x80);
		i2c_write(0x10);		//high nibble of column address = 0
		i2c_write(0x40);		//last control byte, RAM bytes follow
		for(x = 0; x < 132; x++)
		{
			i2c_write(ptt);
		}
		i2c_stop();
	}
}

void old_pix(uint8_t x, uint8_t y, uint8_t c)
{
	uint8_t m, pnt;
	pnt = 1 << (y % 8);
	i2c_start();
	i2c_write(0x78);			//write transaction
	i2c_write(0x80);			//not last control byte, command follows
	i2c_write(0xB0 + y / 8);	//set page
	i2c_write(0x80);
	i2c_write(x & 0x0F);		//low nibble of column address = 0
	i2c_write(0x80);
	i2c_write(0x10 + (x >> 4));	//high nibble of column address = 0
	i2c_write(0x80);
	i2c_write(0xE0);			//read-modify-write
	i2c_write(0x40);			//last control byte, RAM bytes follow
	i2c_stop();
	i2c_start();
	i2c_write(0x79);			//read transaction
	i2c_read();					//dummy read, due to last control byte from RAM
	i2c_m_ack();
	m = i2c_read();				//read RAM byte, column pointer unchanged
	i2c_m_nack();
	i2c_stop();
	if(c == 0)
		m = m & (~pnt);
	else
		m = m | pnt;
	i2c_start();
	i2c_write(0x78);			//write transaction
	i2c_write(0x40);			//last control byte, RAM byte follows
	i2c_write(m);				//write modified RAM byte
	i2c_stop();
	i2c_start();
	i2c_write(0x78);			//write transaction
	i2c_write(0);				//last control byte, command follows
	i2c_write(0xEE);			//end of read-modify-write
	i2c_stop();
}

void old_hline(uint8_t x0, uint8_t y, uint8_t x1, uint8_t c)
{
	uint8_t ptt, x;
	i2c_start();
	i2c_write(0x78);
	i2c_write(0x80);			//not last control byte, command follows
	i2c_write(0xB0 + y / 8);	//set page
	i2c_write(0x80);
	i2c_write(x0 & 0x0F);		//low nibble of column address = 0
	i2c_write(0x80);
	i2c_write(0x10 + (x0 >> 4));	//high nibble of column address = 0
	ptt = 1 << (y % 8);
	if(c == 0)
	    ptt = ptt ^ 0xFF;
	i2c_write(0x40);			//last control byte, RAM bytes follow
	for(x = x0; x <= x1; x++)
	{
		i2c_write(ptt);
	}
	i2c_stop();	
}

void old_vline(uint8_t x, uint8_t y0, uint8_t y1, uint8_t c)
{
	uint8_t p, p0, p1, pt0, pt1;
	p0 = y0 / 8;
	p1 = y1 / 8;
	pt0 = 0xFF << (y0 % 8);
	pt1 = 0xFF >> (7 - (y1 % 8));
	if(p0 == p1)
	{
		pt0 = pt0 & pt1;
	}
	i2c_start();
	i2c_write(0x78);
	i2c_write(0x80);			//not last command
	i2c_write(0xB0 + p0);		//set page
	i2c_write(0x80);
	i2c_write(x & 0x0F);		//low nibble of column address = 0
	i2c_write(0x80);
	i2c_write(0x10 + (x >> 4));	//high nibble of column address = 0
	i2c_write(0xC0);			//not last data
	i2c_write(pt0);
	if(p0 != p1)
	{
		for(p = p0 + 1; p < p1; p++)
		{
			i2c_write(0x80);			//not last command
			i2c_write(0xB0 + p);		//set page
			i2c_write(0x80);
			i2c_write(x & 0x0F);		//low nibble of column address = 0
			i2c_write(0x80);
			i2c_write(0x10 + (x >> 4));	//high nibble of column address = 0
			i2c_write(0xC0);			//not last data
			i2c_write(0xFF);
		}
		i2c_write(0x80);			//not last command
		i2c_write(0xB0 + p);		//set page
		i2c_write(0x80);
		i2c_write(x & 0x0F);		//low nibble of column address = 0
		i2c_write(0x80);
		i2c_write(0x10 + (x >> 4));	//high nibble of column address = 0
		i2c_write(0xC0);			//not last data
		i2c_write(pt1);
	}	
	i2c_write(0);				//last command
	i2c_write(0xE3);			//NOP
	i2c_stop();	
}

uint8_t old_char(uint8_t x, uint8_t p, uint8_t zchn)
{								//Zeichen in einfacher Größe
	uint8_t z[5];
	uint8_t w;
	int8_t i;
	w = pgm_read_byte(&font[6 * (zchn - 32) + 5]);
	for(i = 0; i < w; i++)
	{
		z[i] = pgm_read_byte(&font[6 * (zchn - 32) + i]);
	}
	i2c_start();
	i2c_write(0x78);
	i2c_write(0x80);			//not last command
	i2c_write(0xB0 + p);		//set page
	i2c_write(0x80);
	i2c_write(x & 0x0F);		//low nibble of column address
	i2c_write(0x80);
	i2c_write(0x10 + (x >> 4));	//high nibble of column address
	for(i = 0; i < w; i++)
	{
		i2c_write(0xC0);		//not last data
		i2c_write(z[i]);
	}		
	i2c_write(0);				//last command
	i2c_write(0xE3);			//NOP
	i2c_stop();	
	return(w);
}

uint8_t old_char2(uint8_t x, uint8_t p, uint8_t zchn)
{								//Zeichen in doppelter Größe
	uint8_t z[5], u[5], o[5];
	uint8_t k, m, w;
	int8_t i;
	w = pgm_read_byte(&font[6 * (zchn - 32) + 5]);
	for(i = 0; i < w; i++)
	{
		z[i] = pgm_read_byte(&font[6 * (zchn - 32) + i]);
	}
	for(i = 0; i < w; i++)
	{
		u[i] = 0;
		k = 3;
		for(m = 1; m < 12; m = m << 1)
		{
			if(z[i] & m)
			{
				u[i] = u[i] + k;
			}
			k = k << 2;
		}
		o[i] = 0;
		k = 3;
		for(m = 16; m > 0; m = m << 1)
		{
			if(z[i] & m)
			{
				o[i] = o[i] + k;
			}
			k = k << 2;
		}
	}		
	i2c_start();
	i2c_write(0x78);
	i2c_write(0x80);			//not last control byte, command follows
	i2c_write(0xB0 + p);		//set page
	i2c_write(0x80);
	i2c_write(x & 0x0F);		//low nibble of column address
	i2c_write(0x80);
	i2c_write(0x10 + (x >> 4));	//high nibble of column address
	for(i = 0; i < w; i++)
	{
		i2c_write(0xC0);		//not last control byte, data byte follows
		i2c_write(u[i]);
		i2c_write(0xC0);
		i2c_write(u[i]);
	}
	i2c_write(0x80);			//not last control byte, command follows
	i2c_write(0xB0 + p + 1);	//set page
	i2c_write(0x80);
	i2c_write(x & 0x0F);		//low nibble of column address
	i2c_write(0x80);
	i2c_write(0x10 + (x >> 4));	//high nibble of column address
	for(i = 0; i < w; i++)
	{
		i2c_write(0xC0);		//not last control byte, data byte follows
		i2c_write(o[i]);
		i2c_write(0xC0);
		i2c_write(o[i]);
	}
	i2c_write(0);				//last control byte, command follows
	i2c_write(0xE3);			//NOP
	i2c_stop();	
	return(2 * w);
}

uint8_t old_string2(uint8_t x, uint8_t p, uint8_t strng[])
{
	uint8_t i = 0;
	while(strng[i] != 0)
	{
		x = x + 2 + old_char2(x, p, strng[i]);
		i++;
	}
	return(x);
}	

uint8_t old_intdez2(uint8_t x, uint8_t p, int16_t n, uint8_t st)
{
	uint16_t d, s = 1;
	if(n < 0)
	{
		n = -n;
		x = x + 2 + old_char2(x, p, '-');
	}
	else
	{
		x = x + 2 + old_char2(x, p, ' ');
	}		
	for(d = 1; d < st; d++)
		s = 10 * s;
	while(s > 0)
	{
		d = n / s;
		x = x + 2 + old_char2(x, p, d + 48);
		n = n % s;
		s = s / 10;
	}	
	return(x);
}

uint8_t old_float2(uint8_t x, uint8_t p, float n)
{
	uint8_t d, nz = 0, s = 100;
	float nc;
	if(n < 0)
	{
		x = x + 2 + old_char2(x, p, '-');
		n = -n;
	}
	else
	{
		x = x + 2 + old_char2(x, p, ' ');
	}
	if(n < 999.9)
	{
		nc = n;
		while(s > 0)
		{
			d = (uint8_t)floor(nc / s);
			if((d == 0) && (nz == 0) && (s > 1))
			{
				x = x + 2 + old_char2(x, p, ' ');
			}
			else
			{
				x = x + 2 + old_char2(x, p, d + 48);
				if(d > 0)
					nz = 1;
			}
			nc = nc - d * s;
			s = s / 10;
		}
		if(n < 100.0)
		{
			x = x + 2 + old_char2(x, p, '.');
			nc = 10.0 * nc;
			d = (uint8_t)floor(nc);
			x = x + 2 + old_char2(x, p, d + 48);
		}
		else
		{
			x = x + 2 + old_char2(x, p, ' ');
			x = x + 2 + old_char2(x, p, ' ');
		}
	}
	else	//n >= 999.9 == ungültig
	{
		for(d = 0; d < 3; d++)
		{
			x = x + 2 + old_char2(x, p, '-');
		}
		x = x + 2 + old_char2(x, p, ' ');
		x = x + 2 + old_char2(x, p, ' ');
	}			
	return(x);
}