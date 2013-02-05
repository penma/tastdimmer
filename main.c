#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <stdlib.h>

#include "taster.h"

#define TABLE_TOP 31
#define TABLE_BOT 0

static const __flash uint8_t ramp[32] = {
	0, 1, 2, 2, 2, 3, 3, 4, 5, 6, 7, 8, 10, 11, 13, 16, 19, 23,
	27, 32, 38, 45, 54, 64, 76, 91, 108, 128, 152, 181, 215, 255
};

static uint8_t brightness = TABLE_TOP;
static int enabled = 1;

static void update_light() {
	if (enabled) {
		TCCR0A |= (1 << COM0A1) | (1 << COM0B1);
		OCR0A = OCR0B = ramp[brightness];
	} else {
		TCCR0A &= ~((1 << COM0A1) | (1 << COM0B1));
	}
}


#define DIMMER_TAST_MIN 2 /* 50ms */
#define DIMMER_START_DELAY 20 /* 500ms */
#define DIMMER_DIRECTION_DELAY 40 /* 1000ms */

ISR (TIMER1_COMPA_vect) {
	static uint16_t hold_step = 0;
	static uint16_t save_step;

	if (PINB & (1 << PB3)) {
		save_step = hold_step;
		hold_step = 0;
	} else {
		if (hold_step != 0xffff) {
			hold_step++;
		}
	}

	static int dir = 0;
	static int wait = 0;

	if (hold_step == DIMMER_START_DELAY) {
		wait = 0;
		if (brightness == TABLE_TOP) {
			dir = 0;
		} else if (brightness == TABLE_BOT) {
			dir = 1;
		} else {
			dir = !dir;
		}
	} else if (hold_step > DIMMER_START_DELAY) {
		enabled = 1;
		if (!wait && dir) {
			brightness++;
			if (brightness == TABLE_TOP) {
				wait = 1;
				save_step = hold_step;
			}
		} else if (!wait && !dir) {
			brightness--;
			if (brightness == TABLE_BOT) {
				wait = 1;
				dir = !dir;
				save_step = hold_step;
			}
		} else if (wait) {
			if (hold_step > save_step + DIMMER_DIRECTION_DELAY) {
				wait = 0;
				if (brightness == TABLE_TOP) {
					dir = 0;
				} else {
					dir = 1;
				}
			}
		}
		update_light();
	} else if (hold_step == 0 && save_step > DIMMER_TAST_MIN && save_step <= DIMMER_START_DELAY) {
		/* 50-500ms: taster. licht togglen. */
		enabled = !enabled;
		update_light();
	}
}

int main (void) {
//	init();
	DDRD |= (1 << PD5);
	DDRB |= (1 << PB2);

	/* fast pwm, 9-bit, toggle OC1A, prescaler 1 */
	TCCR0A = (1 << COM0A1) | (1 << COM0B1) | (1 << WGM01) | (1 << WGM00);
	TCCR0B = (1 << CS00);
	OCR0A = OCR0B = 0xff;

	/* tasterfoo */
	DDRB &= ~(1 << PB3);
	PORTB |= (1 << PB3);

	TCCR1A = 0;
	TCCR1B = (1 << WGM12) | (1 << CS12) | (1 << CS10); /* prescaler 1/1024, CTC */
	OCR1A = 194; /* (F_CPU/1024 * 25ms) - 1 */
	TIMSK |= 1 << OCIE1A;
	sei();

	while (1) {
	}
}

