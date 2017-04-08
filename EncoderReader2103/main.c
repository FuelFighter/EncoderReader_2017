
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
#include "../UniversalModuleDrivers/usbdb.h"
#include "../UniversalModuleDrivers/can.h"

#define COUNTSPERROUND 512
#define TIMECONSTANT_MS 100
#define COUNTCONSTANT 1.171875
#define ENCODER_ID 5

#define ENABLE_DIFF1 PF0
#define ENCODER_I_1	 PF3
#define ENCODER_A_1	 PF2
#define ENCODER_B_1  PF1
#define ENABLE_DIFF2 PB0	
#define ENCODER_I_2	 PB3
#define ENCODER_A_2	 PB2
#define ENCODER_B_2  PB1
#define WHEEL_PIN PB4

static uint8_t calculate = 0;
static CanMessage_t canMessage;

void pin_init(){
	//Encoder 1 pin init
	DDRF &= ~((1<<ENCODER_A_1)|(1<<ENCODER_B_1)|(1<<ENCODER_I_1));
	//Encoder 2 pin init and Wheel Pin
	DDRB &= ~((1<<ENCODER_I_2)|(1<<ENCODER_A_2)|(ENCODER_B_2)|(1<<WHEEL_PIN));
	//Enable Differential IC's
	PORTF |= (1<<ENABLE_DIFF1);
	PORTB |= (1<<ENABLE_DIFF2);
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
	can_init(0,0);
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
	
	uint8_t state1 = 0;
	uint8_t state2 = 0;
	uint8_t stateWheel = 0;
    
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
			cli();
			printf("RPM 1: %u\n", rpm1);
			printf("RPM 2: %u\n", rpm2);
			printf("RPM Wheel: %u\n", rpmWheel);
			printCount = 0;
			TCNT1 = 0;
			sei();
		}
		
		if ((PINF & (1<<ENCODER_I_1)) && !state1)
		{
			cli();
			count1++;
			sei();	
			state1 = 1;
			
		} else if (!(PINF & (1<<ENCODER_I_1)) && state1){
			state1 = 0;
		}
		
		if ((PINB & (1<<ENCODER_I_2)) && !state2)
		{
			cli();
			count2++;
			sei();
			state2 = 1;
			
			} else if (!(PINB & (1<<ENCODER_I_2)) && state2){
			state2 = 0;
		}
		if ((PINB & (1<<WHEEL_PIN)) && !stateWheel)
		{
			cli();
			countWheel++;
			sei();
			stateWheel = 1;
			
			} else if (!(PINB & (1<<WHEEL_PIN)) && stateWheel){
			stateWheel = 0;
		}			
    }
}

ISR(TIMER1_COMPA_vect){
	calculate = 1;	
}