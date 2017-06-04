	
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
#include "../UniversalModuleDrivers/rgbled.h"
#include "../UniversalModuleDrivers/usbdb.h"
#include "../UniversalModuleDrivers/can.h"
#include "../UniversalModuleDrivers/adc.h"
#include "../UniversalModuleDrivers/timer.h"

#define COUNTCONSTANT 1.171875
#define WHEEL_COUNT_CONSTANT 60000
#define ENCODER_A_1	 5 //PE5
#define ENCODER_A_2	 2 //PD2
#define WHEEL_PIN 1 //PD1
#define BIT_TO_TORQUE 20.346

#define ENCODER_1_REG PINE
#define ENCODER_1_PIN 5
#define ENCODER_1_INTERRUPT INT5
#define ENCODER_1_INTERRUPT_VECT INT5_vect
#define ENCODER_2_REG PIND
#define ENCODER_2_PIN 2
#define ENCODER_2_INTERRUPT INT2
#define ENCODER_2_INTERRUPT_VECT INT2_vect

static volatile uint32_t encoder_count_1 = 0;
static volatile uint32_t encoder_count_2 = 0;


//ADC for M1 = ADC0
//ADC for M2 = ADC3

static CanMessage_t canMessage;
uint32_t countTime = 0;

void pin_init(){
	//Encoder 1 pin init
	DDRD &= ~((1<<ENCODER_A_2)|(1<<WHEEL_PIN));
	DDRE &= ~((1<<ENCODER_A_1));
}

/*
void interrupt_timer_init(){
	//Calculating speed 10 times a second.
	// ClkIO/64
	TCCR1B |= (1<<CS11)|(1<<CS10);
	//Enable Interrupt
	TIMSK1 |= (1<<TOIE1);
	//Resetting Timer register
	TCNT1 = 0;
}*/

int main(void)
{
	cli();
	pin_init();
	usbdbg_init();
	//interrupt_timer_init();
	timer_init();
	can_init(0,0);
	adc_init();
	rgbled_init();

	// Enable interrupts with rising edge
	EIMSK |= (1 << 2) | (1 << 5);
	EICRA |= 0b00110000;
	EICRB |= 0b00001100;

	sei();
	
	timer_start(TIMER1);
	timer_start(TIMER2);
	
	canMessage.id = ENCODER_CAN_ID;
	canMessage.length = 8;

	uint16_t rpm1 = 0;
	
	uint16_t rpm2 = 0;
	
	//uint16_t countWheel = 0;
	uint32_t rpmWheel = 0;
	
	uint8_t stateWheel = 0;
	
	while (1) 
    {
		rgbled_toggle(LED_GREEN);
		
		if (timer_elapsed_ms(TIMER1) >= 100)
		{
			rgbled_toggle(LED_BLUE);
			
			rpm1 = encoder_count_1 * COUNTCONSTANT;
			canMessage.data.u16[0] = rpm1;
			
			rpm2 = encoder_count_2 * COUNTCONSTANT;
			canMessage.data.u16[1] = rpm2;
			
			if (timer_elapsed_ms(TIMER2) > 5000)
			{
				rpmWheel = 0;
			}
			
			canMessage.data.u16[2] = rpmWheel;

			canMessage.data.u16[3] = encoder_count_1;
			can_send_message(&canMessage);
			
			encoder_count_1 = 0;
			encoder_count_2 = 0;
			timer_start(TIMER1);
		}
		
		if ((PIND & (1<<WHEEL_PIN)) && !stateWheel)
		{
			rgbled_turn_on(LED_RED);
			countTime = timer_elapsed_ms(TIMER2);
			timer_start(TIMER2);
			rpmWheel = WHEEL_COUNT_CONSTANT/countTime;
			stateWheel = 1;
		}
		else if (!(PIND & (1<<WHEEL_PIN)) && stateWheel)
		{
			rgbled_turn_off(LED_RED);
			stateWheel = 0;	
		}
    }
}

ISR(ENCODER_1_INTERRUPT_VECT) { // Motor 1
	encoder_count_1++;
}

ISR(ENCODER_2_INTERRUPT_VECT) { // Motor 2
	encoder_count_2++;
}