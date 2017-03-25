/*
 * Encoder_Test_1603.c
 *
 * Created: 16.03.2017 19:24:56
 * Author : Ultrawack
 */ 

#define F_CPU 8000000UL

#include <avr/io.h>
#include <util/delay.h>
#include <stdlib.h>
#include <string.h>
#include <avr/interrupt.h>
#include "usbdb.h"
#include "can.h"

#define COUNTSPERROUND 512
#define TIMECONSTANT_MS 100
#define COUNTCONSTANT 1.171875
#define ENCODER_ID 5


static uint8_t calculate = 0;
static CanMessage_t canMessage;

void pin_init(){
	DDRB &= ~(0x01); //setter PB0 til input
	DDRB |= (1<<PB5)|(1<<PB6)|(1<<PB7);
	PORTB |= (1<<PB5)|(1<<PB6)|(1<<PB7);
}

void timer_init(){
	TCCR1B |= (1<<CS11)|(1<<CS10);
	TCNT1 = 0;
	TIMSK1 |= (1<<OCIE1A);
	OCR1A = 12500;
}

int main(void)
{
	cli();
	pin_init();
	usbdbg_init();
	timer_init();
	can_init();
	sei();
	
	canMessage.id = ENCODER_ID;
	canMessage.length = 2;
	
	uint8_t printCount = 0;
	uint16_t count = 0;
	uint16_t rpm = 0;
	uint8_t state = 0;
    
	while (1) 
    {
		if (calculate == 1)
		{
			calculate = 0;
			printCount++;
			rpm = count * COUNTCONSTANT;
			canMessage.data[0] = (rpm >> 8);
			canMessage.data[1] = rpm;
			can_send_message(&canMessage);
			count = 0;
			TCNT1 = 0;
		}
		
		if (printCount == 10)
		{
			printf("RPM: %u\n\r", rpm);
			printCount = 0;
			count = 0;
			TCNT1 = 0;
		}
		
		if ((PINB & (1<<PB0)) && !state)
		{
			cli();
			count++;
			sei();	
			state = 1;
			
		} else if (!(PINB & (1<<PB0)) && state){

			state = 0;
		}
			
    }
}

ISR(TIMER1_COMPA_vect){
	calculate = 1;	
}