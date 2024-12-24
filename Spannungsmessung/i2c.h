/* I2C-Bibliothek für AVRtiny3216, erlaubt CLK-Stretching */

#ifndef I2CLIB
#define I2CLIB

#define i2c_ddrclr PORTB_DIRCLR
#define i2c_ddrset PORTB_DIRSET
#define i2c_outclr PORTB_OUTCLR
#define i2c_in PORTB_IN
#define SCLn 0
#define SDAn 1
#define SCL (1<<SCLn)
#define SDA (1<<SDAn)
#define SCL0 i2c_ddrset = SCL
#define SCL1 i2c_ddrclr = SCL; while((i2c_in & SCL) == 0){}
#define SDA0 i2c_ddrset = SDA
#define SDA1 i2c_ddrclr = SDA

void i2c_warte()
{
	volatile uint8_t i;
	for(i = 0; i < 6; i++)			//@ 20 MHz
	{}
}

void i2c_start()
{
	i2c_outclr = SCL | SDA;			//Ausgänge = 0
	SDA1;							//SDA = 1
	SCL1;							//SCL = 1
	i2c_warte();
	SDA0;							//SDA = 0, während SCL == 1
	i2c_warte();
	SCL0;							//SCL = 0
	i2c_warte();
}

void i2c_stop()
{
	SDA0;							//SDA = 0
	SCL1;							//SCL = 1
	i2c_warte();
	SDA1;							//SDA = 1, während SCL == 1
	i2c_warte();
}

uint8_t i2c_write(uint8_t wert)
{
	uint8_t maske, ack;
	for(maske = 0x80; maske > 0; maske = maske >> 1)  //8 Bit schreiben, MSB zuerst
	{
		if((wert & maske) == 0)		//wenn aktuelles Bit in wert == 0 ...
			SDA0;					//... dann SDA = 0 ...
		else						//... sonst ...
			SDA1;					//... SDA = 1
		SCL1;						//Beginn Taktimpuls
		i2c_warte();
		SCL0;						//Ende Taktimpuls
		i2c_warte();
	}
	SDA1;							//SDA freigeben zum ACK-Lesen
	SCL1;							//Beginn 9. Taktimpuls
	i2c_warte();
	ack = (i2c_in & SDA) >> SDAn;	//Bit 0 von ack = ACK/NACK
	SCL0;							//Ende 9. Taktimpuls
	i2c_warte();
	return(ack);					//ACK/NACK zurückgeben
}

uint8_t i2c_read()
{
	uint8_t stelle, wert = 0;
	SDA1;							//SDA freigeben zum Lesen
	for(stelle = 0x80; stelle > 0; stelle = stelle >> 1)  //8 Bit lesen, MSB zuerst
	{
		SCL1;						//Beginn Taktimpuls
		i2c_warte();
		if((i2c_in & SDA) != 0)		//wenn SDA == 1 ...
			wert = wert + stelle;	//... dann entsprechende Stelle in wert = 1
		SCL0;						//Ende Taktimpuls
		i2c_warte();
	}
	return(wert);					//gelesenes Byte zurückgeben
}

void i2c_m_ack()
 {
	SDA0;							//SDA = ACK
	SCL1;							//Beginn 9. Taktimpuls
	i2c_warte();
	SCL0;							//Ende 9. Taktimpuls
	i2c_warte(); 
 }
 
 void i2c_m_nack()
 {
	SDA1;							//SDA = NACK
	SCL1;							//Beginn 9. Taktimpuls
	i2c_warte();
	SCL0;							//Ende 9. Taktimpuls
	i2c_warte(); 
 }
 
#endif
 
