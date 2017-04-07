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

#define ENCODER_COUNT_CONSTANT 960000
#define WHEEL_COUNT_CONSTANT 468750
#define ENCODER_ID 5
#define MEASURE 1
#define SEND 2
#define TIME_CONSTANT 1.126953

#define FILTER_SHIFT 2
#define ENABLE_DIFF1 3
#define ENCODER_I_1	 0
#define ENCODER_A_1	 1
#define ENCODER_B_1  2
#define ENABLE_DIFF2 3
#define ENCODER_I_2	 0
#define ENCODER_A_2	 1
#define ENCODER_B_2  2
#define WHEEL_PIN	 4	

static CanMessage_t txFrame;

void pin_init(){
	//Encoder 1 pin init
	DDRF &= ~((1<<ENCODER_A_1)|(1<<ENCODER_B_1)|(1<<ENCODER_I_1));
	//Encoder 2 pin init
	DDRB &= ~((1<<ENCODER_A_2)|(1<<ENCODER_B_2)|(1<<ENCODER_I_2));
	//Enable Differential IC's
	PORTF |= (1<<ENABLE_DIFF1);
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
uint32_t m1_counts = 0;
uint32_t m2_counts = 0;
uint32_t w_counts = 0;
uint32_t m1_temp_count = 0;
uint32_t m2_temp_count = 0;
uint32_t w_temp_count = 0;

void low_pass_filter(uint32_t newValue, uint32_t *oldValue)
{
	 *oldValue = ((*oldValue - (*oldValue >> FILTER_SHIFT) + newValue) >> FILTER_SHIFT);
}

uint16_t calculate_rpm_motors(uint32_t count)
{
	uint32_t output = 0;
	if (count >= 200)
	{
		output = ENCODER_COUNT_CONSTANT/count;
	} else if (count > ENCODER_COUNT_CONSTANT)
	{
		output = 0;
	} else 
	{
		output = 0xFFFF;	
	}
	
	return output;
}

uint16_t calculate_rpm_wheel(uint32_t count)
{
	uint32_t output = 0;
	if (count >= 200)
	{
		output = WHEEL_COUNT_CONSTANT/count;
	} else if (count > WHEEL_COUNT_CONSTANT)
	{
		output = 0;
	} else 
	{
		output = 0xFFFF;	
	}
	
	return output;
}

void timers_conf()
{
	//Enabling Timer 1 with no prescaling and overflow interrupt
	TCCR1B |= (1<<CS10);
	TIMSK1 |= (1<<TOIE1);
	//Enabling Timer 3 with no prescaling and overflow interrupt
	TCCR3B |= (1<<CS30);
	TIMSK3 |= (1<<TOIE3);
	//Enabling Timer 2 with 1024 prescaling and overflow interrupt
	TCCR2A |= (1<<CS20)|(1<<CS22);
	TIMSK2 |= (1<<TOIE2);
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
	uint16_t time = 0;
	
	while (1) 
    {
		
		if (timer_elapsed_ms(TIMER0) > 100)
		{
			state = SEND;
		}
		
		switch (state)
		{
		case SEND:
			cli();
			
			m1_rpm = calculate_rpm_motors(m1_counts);
			txFrame.data[0] = m1_rpm >> 8;
			txFrame.data[1] = m1_rpm;
			printf("RPM: %u\n",m1_rpm);
			m1_counts = 0;
			
			m2_rpm = calculate_rpm_motors(m2_counts);
			txFrame.data[2] = m2_rpm >> 8;
			txFrame.data[3] = m2_rpm;
			
			w_rpm = calculate_rpm_wheel(w_counts);
			txFrame.data[4] = w_rpm;
			
			can_send_message(&txFrame);
			state = MEASURE;
			timer_start(TIMER0);
			
			sei();
			break;
			
		case MEASURE:
			//Encoder 1
			if (!(PINF&(1<<ENCODER_I_1)) && (m1_state == 0))
			{
				m1_temp_count = TCNT1;
				TCNT1 = 0;
				low_pass_filter(m1_temp_count,&m1_counts);
				m1_state = 1;
			} else if ((PINF &(1<<ENCODER_I_1)) && m1_state)
			{
				m1_state = 0;
			}
			
			//Encoder 2
			if ((PINB & (1<<ENCODER_I_2)) && !m1_state)
			{
				m2_temp_count = TCNT3;
				TCNT3 = 0;
				low_pass_filter(m2_temp_count,&m2_counts);
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
				low_pass_filter(w_temp_count,&w_counts);
				w_state = 1;
			} else if (!(PINB & (1<<WHEEL_PIN)) && m1_state)
			{
				w_state = 0;
			}
			break;
		}
	}
}

ISR(TIMER1_OVF_vect)
{
	m1_temp_count += 0xFFFF;
	TCNT1 = 0;
}

ISR(TIMER2_OVF_vect)
{
	w_temp_count += 0xFF;
	TCNT2 = 0;
}

ISR(TIMER3_OVF_vect)
{
	m2_temp_count += 0xFFFF;
	TCNT3 = 0;
}
