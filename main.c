#include <avr/io.h>
#include <avr/interrupt.h>

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "serial.h"

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

static void process_command()
{
	int n;
	int error = 0;

	if (strcmp(serial_argv[0], "dt") == 0) {
		n = atoi(serial_argv[1]);
		if (n>=0 && n<256)
			pwm_deadtime = n;
		else
			error = 1;
	} else if (strcmp(serial_argv[0], "amp") == 0) {
		n = atoi(serial_argv[1]);
		if (n>=0 && n<256)
			pwm_amplitude = n;
		else
			error = 1;
	} else {
		error = 1;
	}

	if (error) {
		serial_write("ERROR\r\n", 7);
	} else {
		serial_write("OK\r\n", 4);
	}

	serial_argc = 0;
}

void main()
{
	unsigned int next_tick = 0;

	pwm_init();
	serial_init();

	sei();

	ADCSRA = ADEN;
	ADCSRB = 0;

	adc_start_conversion();
	serial_rx_enable();
	serial_argc = 0;
//	pwm_amplitude = 200;

	pwm_amplitude = 50;

	while (1) {
		unsigned int val = adc_value[0];

		if (serial_argc) {
			process_command();
			serial_rx_enable();
		}

		adc_update();
	}

}
