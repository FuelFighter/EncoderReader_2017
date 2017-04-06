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
#include "../UniversalModuleDrivers/timer.h"

#define COUNTCONSTANT 960000
#define ENCODER_ID 5
#define MEASURE 1
#define SEND 2

#define FILTER_SHIFT 4
#define ENABLE_DIFF1 PF3
#define ENCODER_I_1	 PF0
#define ENCODER_A_1	 PF1
#define ENCODER_B_1  PF2
#define ENABLE_DIFF2 PB3
#define ENCODER_I_2	 PB0
#define ENCODER_A_2	 PB1
#define ENCODER_B_2  PB2
#define WHEEL_PIN	 PB4	

static CanMessage_t txFrame;

void pin_init(){
	//Encoder 1 pin init
	DDRE &= ~((1<<ENCODER_A_1)|(1<<ENCODER_B_1)|(1<<ENCODER_I_1));
	//Encoder 2 pin init
	DDRB &= ~((1<<ENCODER_A_2)|(1<<ENCODER_B_2)|(1<<ENCODER_I_2));
	//Enable Differential IC's
	PORTE |= (1<<ENABLE_DIFF1);
	PORTB |= (1<<ENABLE_DIFF2);
	//Wheel Hall Sensor pin init
	// TBD
}

uint32_t m1_rpm = 0;
uint32_t m2_rpm = 0;
uint32_t w_rpm = 0;
uint8_t m1_state = 0;
uint8_t m2_state = 0;
uint8_t w_state = 0;
uint16_t m1_counts = 0;
uint16_t m2_counts = 0;
uint16_t w_counts = 0;
uint16_t m1_samples = 0;
uint16_t m2_samples = 0;
uint16_t w_samples = 0;

void low_pass_filter(uint16_t newValue, uint32_t *oldValue)
{
	 *oldValue = ((*oldValue - (*oldValue >> FILTER_SHIFT) + newValue) >> FILTER_SHIFT);
}

uint16_t calculate_rpm(uint16_t count)
{
	uint32_t output = 0;
	if (count >= 200)
	{
		output = COUNTCONSTANT/count;
	} else if (count == 65536)
	{
		output = 0;
	}
	return output;
}

void timers_conf()
{

}

int main(void)
{
	cli();
	pin_init();
	usbdbg_init();
	timer_init();
	can_init(0,0);
	timer_start(TIMER0);
	timers_conf();
	sei();
	
	uint8_t state = MEASURE;
    txFrame.id = ENCODER_ID;
	txFrame.length = 5;
	
	while (1) 
    {
		if (timer_elapsed_ms(TIMER0) > 100)
		{
			state = SEND;
		}
		
		switch (state)
		{
		case SEND:
			m1_counts = m1_counts/m1_samples;
			low_pass_filter(calculate_rpm(m1_counts),&m1_rpm);
			txFrame.data[0] = m1_rpm >> 8;
			txFrame.data[1] = m1_rpm;
			m1_samples = 0;
			m1_counts = 0;
			
			m2_counts = m2_counts/m2_samples;
			low_pass_filter(calculate_rpm(m2_counts),&m2_rpm);
			txFrame.data[2] = m2_rpm >> 8;
			txFrame.data[3] = m2_rpm;
			m2_samples = 0;
			m2_counts = 0;
			
			w_counts = w_counts/w_samples;
			low_pass_filter(calculate_rpm(w_counts),&w_rpm);
			txFrame.data[4] = w_rpm; 
			w_samples = 0;
			w_counts = 0;
			
			can_send_message(&txFrame);
			state = MEASURE;
			timer_start(TIMER0);
			break;
			
		case MEASURE:
			//Encoder 1
			if ((PINB & (1<<ENCODER_I_1)) && !m1_state)
			{
				m1_counts += TCNT1;
				TCNT1 = 0;
				m1_samples++;
				m1_state = 1;
			} else if (!(PINB & (1<<ENCODER_I_1)) && m1_state)
			{
				m1_state = 0;
			}
			
			//Encoder 2
			if ((PINF & (1<<ENCODER_I_2)) && !m1_state)
			{
				m2_counts = TCNT3;
				TCNT3 = 0;
				m2_samples++;
				m2_state = 1;
			} else if (!(PINB & (1<<ENCODER_I_1)) && m1_state)
			{
				m2_state = 0;
			}
			
			//Wheel Hall Sensor
			if ((PINB & (1<<WHEEL_PIN)) && !m1_state)
			{
				w_counts = TCNT2;
				TCNT2 = 0;
				w_samples++;
				w_state = 1;
			} else if (!(PINB & (1<<WHEEL_PIN)) && m1_state)
			{
				w_state = 0;
			}
			
			break;
		}
	}
}


