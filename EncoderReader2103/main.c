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

#define ENABLE_DIFF1 PF3
#define ENCODER_I_1	 PF0
#define ENCODER_A_1	 PF1
#define ENCODER_B_1  PF2
#define ENABLE_DIFF2 PB3
#define ENCODER_I_2	 PB0
#define ENCODER_A_2	 PB1
#define ENCODER_B_2  PB2	

static uint8_t calculate = 0;
static CanMessage_t canMessage;

void pin_init(){
	//Encoder 1 pin init
	DDRE &= ~((1<<ENCODER_A_1)|(1<<ENCODER_B_1)|(1<<ENCODER_I_1))
	//Encoder 2 pin init
	DDRB &= ~((1<<ENCODER_A_2)|(1<<ENCODER_B_2)|(1<<ENCODER_I_2));
	//Enable Differential IC's
	PORTE |= (1<<ENABLE_DIFF1);
	PORTB |= (1<<ENABLE_DIFF2);
	//Wheel Hall Sensor pin init
	// TBD
}

void timer_init(){
	//Calculating speed 10 times a second.
	// ClkIO/64
	TCCR1B |= (1<<CS11)|(1<<CS10);
	//Enable Interrupt
	TIMSK1 |= (1<<OCIE1A);
	//Setting Compare register
	OCR1A = 12500;
	//Resetting Timer register
	TCNT1 = 0;
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
	canMessage.length = 6;
	
	uint8_t printCount = 0;
	uint16_t count1 = 0;
	uint16_t rpm1 = 0;
	
	uint16_t count2 = 0;
	uint16_t rpm2 = 0;
	
	uint16_t countWheel = 0;
	uint16_t rpmWheel = 0;
	
	uint8_t state = 0;
    
	while (1) 
    {
		if (calculate == 1)
		{
			calculate = 0;
			printCount++;
			rpm1 = count1 * COUNTCONSTANT;
			canMessage.data[0] = (rpm1 >> 8);
			canMessage.data[1] = rpm1;
			
			rpm2 = count2 * COUNTCONSTANT;
			canMessage.data[2] = (rpm2 >> 8);
			canMessage.data[3] = rpm2;
			
			rpmWheel = countWheel * COUNTCONSTANT;
			canMessage.data[4] = (rpmWheel >> 8);
			canMessage.data[5] = rpmWheel;
			
			can_send_message(&canMessage);
			
			count1 = 0;
			count2 = 0;
			countWheel = 0;
			TCNT1 = 0;
		}
		
		if (printCount == 10)
		{
			printf("RPM 1: %u\n", rpm1);
			printf("RPM 2: %u\n", rpm2);
			printf("RPM Wheel: %u\n", rpmWheel);
			printCount = 0;
			TCNT1 = 0;
		}
		
		if ((PINB & (1<<PB0)) && !state)
		{
			cli();
			count1++;
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