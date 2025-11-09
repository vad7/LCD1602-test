/*
 * Counter.c
 *
 * Created: 07.11.2025
 *  Author: Vadim Kulakov, vad7@yahoo.com
 *
 * ATtiny44A
 */ 
// Fuses: BODLEVEL = 1V8
#define F_CPU	8000000UL

#include <stdlib.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/sleep.h>
#include <avr/wdt.h>
#include <avr/pgmspace.h>
#include <avr/eeprom.h>
#include <util/delay.h>
#include <util/atomic.h>

#include "I2C.h"
#include "LCDHD44780.h"

#define KEY_IN					PINB
#define KEY_LEFT				(1<<PORTB0)	// LCD 2004
#define KEY_2004_PRESSING		!(KEY_IN & KEY_LEFT)
#define KEY_OK					(1<<PORTB1)
#define KEY_RUS_PRESSING		!(KEY_IN & KEY_OK)
#define KEY_RIGHT				(1<<PORTB2)
#define KEY_SEC_PRESSING		!(KEY_IN & KEY_RIGHT)
#define KEY_INIT				PORTB = KEY_LEFT | KEY_OK | KEY_RIGHT; // pullup
volatile uint8_t KeysPressed = 0;
uint8_t KeysPressedTimeOut = 0;
#define KEY_LONG_PRESSING_TIME	12 // *0.125 sec, entering setup = *2
volatile uint8_t Timer = 0;		// sec
uint8_t sec_cnt = 0;

#define EPROM_DISPLAY_PERIOD		0x00

void Delay10us(uint8_t ms) {
	while(ms-- > 0) _delay_us(10); //wdt_reset();
}
void Delay1ms(uint8_t ms) {
	while(ms-- > 0) {
		_delay_ms(1); wdt_reset();
	}
}
void Delay100ms(unsigned int ms) {
	while(ms-- > 0) {
		_delay_ms(100); wdt_reset();
	}
}

uint8_t EEPROM_read(uint16_t ucAddress) // ATtiny84A only!
{
	while(EECR & (1<<EEPE)) ; // EEWE
	EEAR = ucAddress;
	EECR |= (1<<EERE);
	return EEDR;
}
void EEPROM_write(uint16_t ucAddress, uint8_t ucData) // ATtiny84A only!
{
	while(EECR & (1<<EEPE)) ; // EEWE
	cli();
	EECR = (0<<EEPM1)|(0<<EEPM0);
	EEAR = ucAddress;
	EEDR = ucData;
	EECR |= (1<<EEMPE); //(1<<EEMWE);
	EECR |= (1<<EEPE); //(1<<EEWE);
	sei();
}

#define SETUP_WATCHDOG WDTCSR = (1<<WDCE) | (1<<WDE); WDTCSR = (1<<WDE) | (1<<WDIE) | (0<<WDP3) | (1<<WDP2) | (1<<WDP1) | (0<<WDP0) //  Watchdog 1 s
ISR(TIM1_COMPA_vect, ISR_NOBLOCK) { // 0.125s
	if(++sec_cnt == 8) {
		sec_cnt = 0;
		if(Timer) Timer--;
	}
	//if(DisplayRefresh > 1) DisplayRefresh--;
	//if(KeysPressedTimeOut && --KeysPressedTimeOut == 0) {
	//	KeysPressed = (KEY_IN & (KEY_LEFT | KEY_RIGHT | KEY_OK)) ^ (KEY_LEFT | KEY_RIGHT | KEY_OK);
	//}
}

int main(void)
{
	CLKPR = (1<<CLKPCE); CLKPR = (0<<CLKPS3) | (0<<CLKPS2) | (0<<CLKPS1) | (0<<CLKPS0); // Clock prescaler division factor: 1
	MCUCR = (1<<SE) | (0<<SM1) | (0<<SM0); // Idle sleep enable
	KEY_INIT;
	//DDRA = ; // Out
	//PORTA = StopSwitch | (1<<PORTA5) | (1<<PORTA2) | (1<<PORTA1) | (1<<PORTA0); // pullup: Cnt1, Cnt2, LDR1, not used pins
	//PORTB = ; // pullup
	SETUP_WATCHDOG;
	//GIMSK = (1<<PCIE1) | (1<<PCIE0); // Pin Change Interrupt Enable 0, 1
	//PCMSK0 = (1<<PCINT4) | (1<<PCINT3); // Pin Change Mask Register 0 - reed switch, power sensor
	//PCMSK1 = (1<<PCINT10) | (1<<PCINT9) | (1<<PCINT8); // Pin Change Mask Register 1 - Keys
	uint8_t display_preiod = EEPROM_read(EPROM_DISPLAY_PERIOD);
	if(display_preiod == 0xFF) EEPROM_write(EPROM_DISPLAY_PERIOD, display_preiod = 3);
	//
	OCR1A = F_CPU / (4 * 2 * 64); // 0.125 sec; = F_CPU / (FREQ * 2 * Prescaller); min = 61Hz;  FREQ = F_CPU / (2 * Prescaller * (1 + OCR1A)); 
	TIMSK1 = (1<<OCIE1A); // overflow interrupt
	//DDRA |= (1<<PORTA5); TCCR1A = (1<<COM1B0); // FREQ OUT OC1B
	TCCR1B = (1<<WGM12) | (0 << CS12) | (1 << CS11) | (1 << CS10); // CTC mode, Timer0 prescaller: 64
	//
	sei();
	if(KEY_RUS_PRESSING) {
		LCDCH_Init(LCDCH_2LINES | LCDCH_RUS_FONT);
		LCDCH_WriteStringPGM(PSTR("Rus 5x8"));
	} else if(KEY_SEC_PRESSING) {
		LCDCH_Init(LCDCH_2LINES | LCDCH_SECOND_FONT);
		LCDCH_WriteStringPGM(PSTR("Sec 5x10"));
	} else {
		LCDCH_Init(LCDCH_2LINES); // | LCDCH_SECOND_FONT));
		LCDCH_WriteStringPGM(PSTR("Std 5x8"));
	}
	uint8_t cols, rows;
	if(KEY_2004_PRESSING) {
		LCDCH_WriteStringPGM(PSTR(",LCD2004"));
		cols = 16;
		rows = 4;
	} else {
		LCDCH_WriteStringPGM(PSTR(",LCD1602"));
		cols = 12;
		rows = 2;
	}
	LCDCH_SetCursor(2, 1);
	if(cols >= 16) LCDCH_WriteStringPGM(PSTR("p2-2004,p3-RU,p4-SEC")); 
	else LCDCH_WriteStringPGM(PSTR("p2-2004,3-RU,4-S")); 
	//Delay100ms(15);
	uint8_t c = 0;
	uint8_t loop1 = 0;
	Timer = 5;
	
    while(1)
    {	__asm__ volatile ("" ::: "memory"); // Need memory barrier
	    sleep_cpu();
		wdt_reset();
		if(Timer == 0) {
			LCDCH_ClearDisplay();
			for(uint8_t i = 1; i <= rows; i++) {
				LCDCH_SetCursor(i, 1);
				FormatNumberHEX(c);
				LCDCH_WriteString(buffer);
				LCDCH_WriteByte(':');
				LCDCH_WriteByte(' ');
				for(uint8_t j = 1; j <= cols; j++) {
					LCDCH_WriteByte(c++);
					if(c == 0) {
						loop1 = 1;
						break;
					}
				}
			}
			if(loop1) {
				uint8_t pressed = KEY_RUS_PRESSING;
				while(KEY_RUS_PRESSING) {
				    sleep_cpu();
				    wdt_reset();
				}
				if(pressed) Delay100ms(3); else Timer = EEPROM_read(EPROM_DISPLAY_PERIOD);
			} else Timer = EEPROM_read(EPROM_DISPLAY_PERIOD);
		}
	}
}


