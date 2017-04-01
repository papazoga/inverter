#include <avr/io.h>
#include <avr/interrupt.h>

extern unsigned char sintab[N_ENTRIES];

static unsigned char pwm_curchan;
static unsigned char pwm_on;
static int pwm_position;

unsigned char pwm_deadtime;
unsigned char pwm_amplitude;

#define INITIAL_AMPLITUDE 50
#define INITIAL_TICKS     132  /* ((F_CPU / (7680 * N_ENTRIES))-1) */
#define INITIAL_DEADTIME  2    /* 1us deadtime */

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
	OCR2A = INITIAL_TICKS;
	TIMSK2 = (1<<OCIE2A);	        /* Interrupt when match occurs */

	pwm_curchan = 0;
	pwm_on = 0;
	pwm_position = 0;
	pwm_amplitude = INITIAL_AMPLITUDE;
	pwm_deadtime = INITIAL_DEADTIME;
}

static void enable_channel(int ch)
{
	switch (ch) {
	case 0:
		TCCR0A =  (1<<COM0A1)                 /* Clear OC0A on compare match going up, set on match going down */
			| (1<<COM0B0) | (1<<COM0B1)   /* Set OC0B on compare match going up, clear on match going down */
			| (1<<WGM00);	              /* Phase-correct PWM 0x00 to 0xff */
		TCCR0B = 2;
		TCNT0 = 0;
		break;
	case 1:
		TCCR1A =  (1<<COM1A1)                 /* Clear OC1A on compare match going up, set on match going down */
			| (1<<COM1B0) | (1<<COM1B1)   /* Set OC0B on compare match going up, clear on match going down */
			| (1<<WGM10);                 /* Phase-correct (8-bit) PWM 0x00 to 0xff */
		TCCR1B = 2;
		TCNT1 = 0;
		break;
	default:
		break;
	}
}

static void disable_channel(int ch)
{
	switch (ch) {
	case 0:
		TCCR0B = 0;
		PORTD |= (1<<6);
		PORTD &= ~(1<<5);
		break;
	case 1:
		TCCR1B = 0;
		PORTB |= (1<<1);
		PORTB &= ~(1<<2);
		break;
	default:
		break;
	}
}

void pwm_update()
{
	unsigned int sample = sintab[pwm_position] * pwm_amplitude;
	unsigned char val = sample >> 8;

	if (pwm_position == 0) {
		PORTD ^= (1<<7);
		pwm_curchan ^= 1;

		enable_channel(pwm_curchan);
		disable_channel(pwm_curchan^1);
	}

	OCR0A = val;
	OCR0B = val + pwm_deadtime;

	OCR1A = val;
	OCR1B = val + pwm_deadtime;

out:
	pwm_position = (pwm_position+1) % N_ENTRIES;
}

ISR(TIMER2_COMPA_vect)
{
	pwm_update();
}
