#include <avr/io.h>
#include <avr/interrupt.h>

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

static unsigned char adc_source = 0;
static unsigned int adc_value[2];

extern unsigned char pwm_amplitude;
extern unsigned char pwm_deadtime;

static void adc_start_conversion()
{
	ADMUX = (1<<REFS0) | adc_source;

	ADCSRA &= ~(1<<4);	   /* clear interrupt flag */
	ADCSRA |= (1<<ADPS0) | (1<<ADPS1) | (1<<ADPS2) | (1<<ADEN);
	ADCSRA |= (1<<ADSC);	/* start conversion */
}

static void adc_update()
{
	if (ADCSRA & (1<<6))
		return;

	adc_source ^= 1;
	adc_value[adc_source] = ADC;

	adc_start_conversion();
}

void main()
{
	unsigned int next_tick = 0;

	pwm_init();
	sei();

	ADCSRA = ADEN;
	ADCSRB = 0;

	adc_start_conversion();
	pwm_amplitude = 200;

	while (1) {
		unsigned int val = adc_value[0];

		adc_update();

//		pwm_amplitude = (val - 2*pwm_deadtime) >> 2;
	}

}
