#include <avr/io.h>
#include <avr/interrupt.h>

extern unsigned char sintab[N_ENTRIES];

static unsigned char pwm_dir;
static unsigned char pwm_on;
static int pwm_position;

unsigned char pwm_deadtime;
unsigned char pwm_amplitude;

#define INITIAL_AMPLITUDE 20
#define INITIAL_TICKS     132  /* ((F_CPU / (7680 * N_ENTRIES))-1) */
#define INITIAL_DEADTIME  17    /* In 60ns increments */

#ifdef SLOW_WAVEFORM
#define TIMER_PRESCALE 3
#else
#define TIMER_PRESCALE 1
#endif

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
	TCCR2A = 0;
	TCCR2B = 0;
	TCNT0 = 0;
	TCNT2 = 0;

	TCCR2A = (1<<WGM21);		/* Clear on compare match */
	TCCR2B = 3;			/* Prescale by 32. We are CPU limited. */
	OCR2A = INITIAL_TICKS;
	TIMSK2 = (1<<OCIE2A);	        /* Interrupt when match occurs */

	pwm_dir = 0;
	pwm_on = 0;
	pwm_position = 0;
	pwm_amplitude = INITIAL_AMPLITUDE;
	pwm_deadtime = INITIAL_DEADTIME;
}

/*
 * Positive (dir=0) or negative (dir=1) half-wave
 */
static void set_channel(int dir)
{
	switch (dir) {
	case 0:
		TCCR0A =  (1<<COM0A1)                 /* Clear OC0A on compare match going up, set on match going down */
			| (1<<COM0B0) | (1<<COM0B1)   /* Set OC0B on compare match going up, clear on match going down */
			| (1<<WGM00);	              /* Phase-correct PWM 0x00 to 0xff */
		TCCR0B = TIMER_PRESCALE;
		TCNT0 = 0;
		break;
	case 1:
		TCCR0A =  (1<<COM0A0) | (1<<COM0A1)   /* Set OC0A on compare match going up, clear on match going down */
			| (1<<COM0B1)                 /* Clear OC0B on compare match going up, set on match going down */
			| (1<<WGM00);	              /* Phase-correct PWM 0x00 to 0xff */
		TCCR0B = TIMER_PRESCALE;
		TCNT0 = 0;
		break;
	default:
		break;
	}
}

#define PORTMASK  ((1<<1) | (1<<2))
void pwm_update()
{
	unsigned int sample = sintab[pwm_position] * pwm_amplitude;
	unsigned char val = sample >> 8;

	if (pwm_position == 0) {
		unsigned char portval = PORTB & ~PORTMASK;
		PORTD ^= (pwm_dir<<7);

		set_channel(pwm_dir);

		PORTB = portval | ((pwm_dir^1)<<2) | (pwm_dir<<1);
		pwm_dir = 1;
	}

	if (val) {
		if (pwm_dir) {
			OCR0A = val + pwm_deadtime;
			OCR0B = val;
		} else {
			OCR0A = val;
			OCR0B = val + pwm_deadtime;
		}
	} else {
		OCR0A = 0;
		OCR0B = 0;
	}

out:
	pwm_position = (pwm_position+1) % N_ENTRIES;
}

ISR(TIMER2_COMPA_vect)
{
	pwm_update();
}
