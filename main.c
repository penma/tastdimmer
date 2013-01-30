#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <stdlib.h>

#include "taster.h"

static uint8_t brightness = 80;
static int enabled = 1;

static void update_light() {
	if (enabled) {
		TCCR0A |= (1 << COM0A1) | (1 << COM0B1);
		OCR0A = OCR0B = brightness;
	} else {
		TCCR0A &= ~((1 << COM0A1) | (1 << COM0B1));
	}
}

#define TABLE_TOP 0xff
#define TABLE_BOT 0

#define DIMMER_TAST_MIN 10
#define DIMMER_START_DELAY 100
#define DIMMER_DIRECTION_DELAY 200

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
	OCR1A = 38; /* (F_CPU/1024 * 5ms) - 1 */
	TIMSK |= 1 << OCIE1A;
	sei();

	while (1) {
	}
}

