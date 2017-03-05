#include <avr/io.h>
#include <avr/interrupt.h>

extern unsigned char sintab[N_ENTRIES];

static unsigned char pwm_side;
static unsigned char pwm_on;
static int pwm_position;

unsigned char pwm_deadtime;
unsigned char pwm_amplitude;

#define INITIAL_AMPLITUDE 50
#define INITIAL_TICKS     132  /* ((F_CPU / (7680 * N_ENTRIES))-1) */

enum {
	TOP_OFF = 0,
	TOP_ON,
	BOTTOM_OFF,
	BOTTOM_ON,
};

void pwm_init()
{
	/* Set OC0A/B to output */
	DDRD |= (1<<5) | (1<<6);
	
	DDRD |= (1<<7);
	PORTD &= ~(1<<7);
	
	/* Set OC1A/B to output */
	DDRB |= (1<<1) | (1<<2);

	/* Start in a known state */
	PORTD &= ~((1<<5) | (1<<6));
	PORTB &= ~((1<<1) | (1<<2));

	TCCR0A = 0;
	TCCR0B = 0;
	TCCR1A = 0;
	TCCR1B = 0;
	TCCR2A = 0;
	TCCR2B = 0;
	TCNT0 = 0;
	TCNT1 = 0;
	TCNT2 = 0;

	TCCR2A = (1<<WGM21);		/* Clear on compare match */
	TCCR2B = 3;			/* Prescale by 32. We are CPU limited. */
	OCR2A = INITIAL_TICKS;		/* 16MHz/4 = 4MHz */
	TIMSK2 = (1<<OCIE2A);	        /* Interrupt when match occurs */

	pwm_side = 0;
	pwm_on = 0;
	pwm_position = 0;
	pwm_amplitude = INITIAL_AMPLITUDE;
	pwm_deadtime = 1;

	TCCR0A =  (1<<COM0A1)                 /* Clear OC0A on compare match going up, set on match going down */
		| (1<<COM0B0) | (1<<COM0B1)   /* Set OC0B on compare match going up, clear on match going down */
		| (1<<WGM00);	              /* Phase-correct PWM 0x00 to 0xff */
	TCCR0B = 2;

	TCCR1A =  (1<<COM1A1)                 /* Clear OC1A on compare match going up, set on match going down */
		| (1<<COM1B0) | (1<<COM1B1)   /* Set OC0B on compare match going up, clear on match going down */
		| (1<<WGM10);                 /* Phase-correct (8-bit) PWM 0x00 to 0xff */
	TCCR1B = 2;
}

#if 0
void pwm_disable()
{
	PORTD &= ~((1<<3) | (1<<5) | (1<<6));
	PORTB &= ~((1<<1) | (1<<2) | (1<<3));

	TCCR0A = 0;
	TCCR1A = 0;

	TCCR0B = 0;
	TCCR1B = 0;

	TIMSK0 &= ~1;
}
#endif

#define PWM_CUTOFF 10

void pwm_update()
{
	unsigned int sample = sintab[pwm_position] * pwm_amplitude;
	unsigned char val = sample >> 8;

	if (pwm_position == 0) {
		PORTD ^= (1<<7);
		pwm_side ^= 1;

		TCNT0 = 0;
		TCNT1 = 0;
		if (pwm_side) {
			DDRD &= ~((1<<5) | (1<<6));
			DDRB |= (1<<1) | (1<<2);
		} else {
			DDRD |= (1<<5) | (1<<6);
			DDRB &= !((1<<1) | (1<<2));
		}
	
	}

	OCR0A = val + pwm_deadtime;
	OCR0B = val;

	OCR1A = val + pwm_deadtime;
	OCR1B = val;

out:
	pwm_position = (pwm_position+1) % N_ENTRIES;
}

ISR(TIMER2_COMPA_vect)
{
	pwm_update();
}
