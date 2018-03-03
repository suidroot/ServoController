#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>
#include "includes/USART.h"

#define PULSE_MIN		1000
#define PULSE_MAX		2000
#define PULSE_MID		1500

#define MOVEMAXPIN 		PB0
#define SERVOCONTROL 	PB1

#define ENCODERLEFT 	PD2
#define ENCODERRIGHT 	PD3
#define RESETPIN		PD4

#define DEBOUNCE_TIME  	1300                            /* microseconds */
#define MOVEINCREMENT	100

// uint16_t getNumber16(void);
void initPinChangeInterrupt(void);
void initTimer1Servo(void);
void showOff(void);
uint16_t testvalue(uint16_t servovalue);
void moveservo(uint16_t servovalue);


volatile uint8_t movedirection;

ISR(PCINT2_vect) {         /* Run every time there is a change on button */
	cli();
	if (bit_is_clear(PIND, ENCODERLEFT) && bit_is_set(PIND, ENCODERRIGHT)) {
		_delay_us(DEBOUNCE_TIME);
		if (bit_is_clear(PIND, ENCODERLEFT) && bit_is_set(PIND, ENCODERRIGHT))
			movedirection = 1;
	} else if (bit_is_clear(PIND, ENCODERRIGHT) && bit_is_set(PIND, ENCODERLEFT)) {
		_delay_us(DEBOUNCE_TIME);
		if (bit_is_clear(PIND, ENCODERRIGHT) && bit_is_set(PIND, ENCODERLEFT))
			movedirection = 2;
	} else if (bit_is_clear(PIND, RESETPIN)) {
		_delay_us(DEBOUNCE_TIME);
		if (bit_is_clear(PIND, RESETPIN))
			movedirection = 9;
	}
}

int main(void) {

	uint16_t servoPulseLength;
	uint16_t isgood;

	/* Set Port ot output */
	DDRB |= (1 << PB0) | (1 << PB1);

	initPinChangeInterrupt();
	initTimer1Servo();
	initUSART();

	printString("\r\nWelcome to the Servo Controller\r\n");
	showOff();

	servoPulseLength = PULSE_MID;

	while (1) {

		// 1 = left
		// 2 = right
		// 9 = reset
		// 0 = idle

		switch (movedirection) {
			case 1:
				servoPulseLength = servoPulseLength - MOVEINCREMENT;
				isgood = testvalue(servoPulseLength);
				if (isgood == 0) {
					printString("Move Left ");
					printWord(servoPulseLength);
					moveservo(servoPulseLength);
				} else {
					servoPulseLength = isgood;
				}
				break;
			case 2:
				servoPulseLength = servoPulseLength + MOVEINCREMENT;
				isgood = testvalue(servoPulseLength);

				if (isgood == 0) {
					printString("Move Right ");
					printWord(servoPulseLength);
					moveservo(servoPulseLength);
				} else {
					servoPulseLength = isgood;
				}
				break;
			// case 8:
			// 	printString("\r\nEnter a four-digit pulse length: ");
			// 	servoPulseLength = getNumber16();

			// 	printString("\r\nOn my way....\r\n");
			// 	OCR1A = servoPulseLength;
			// 	DDRB |= (1 << SERVOCONTROL);

			// 	_delay_ms(1000);
			// 	printString("Releasing...\r\n");
			// 	while (TCNT1 < 3000) {
			// 		;
			// 	}
			// 	DDRB &= ~(1 << SERVOCONTROL);
			// 	break;
			case 9:
				printString("Move to Center");
				servoPulseLength = PULSE_MID;
				moveservo(servoPulseLength);
				break;

		}
		movedirection = 0;
		sei();

	}
	return (0);
}

uint16_t testvalue (uint16_t servovalue) {

	uint16_t isgood = 0;

	// printString("Start\r\n");
	if (servovalue <= PULSE_MIN) {
		PORTB |= (1 << MOVEMAXPIN);
		printWord(servovalue);
		printString("is Below Min\r\n");
		isgood = PULSE_MIN;

	} else if (servovalue >= PULSE_MAX) {
		PORTB |= (1 << MOVEMAXPIN);
		printWord(servovalue);
		printString(" is Above Max\r\n");

		isgood = PULSE_MAX;
	} 

	return isgood;
}

void moveservo(uint16_t servovalue) {
		PORTB &= ~(1 << MOVEMAXPIN);
		DDRB |= (1 << SERVOCONTROL);
		OCR1A = servovalue;
		_delay_ms(100);
		// while (TCNT1 < 3000) {
		// 	;
		// }
		DDRB &= ~(1 << SERVOCONTROL);
		printString("... Done!\r\n");
}

void initPinChangeInterrupt(void) {
	/* set pin-change interrupt for D pins */
	PCICR |= (1 << PCIE2);
	/* set mask to look for PCINT18 / ENCODERLEFT */     
	PCMSK2 |= (1 << ENCODERLEFT) | (1 << ENCODERRIGHT) | (1 << RESETPIN);
	/* Enable pullups on input ports */
	PORTD |= (1 << ENCODERLEFT) | (1 << ENCODERRIGHT) | (1 << RESETPIN);

	/* set (global) interrupt enable bit */   
	sei();
}

void initTimer1Servo(void) {
	// Set Fast PWM
	TCCR1A |= (1 << WGM11);
	TCCR1B |= (1 << WGM12) | (1 << WGM13);

	// No Preescale
	TCCR1B |= (1 << CS11);

	ICR1 = 151100;
	TCCR1A |= (1 << COM1A1);
	DDRB |= (1 << SERVOCONTROL);

}

void showOff(void) {
	DDRB |= (1 << SERVOCONTROL);

	printString("Center\r\n");
	OCR1A = PULSE_MID;
	_delay_ms(500);

	printString("Clockwise Max\r\n");
	OCR1A = PULSE_MIN;
	_delay_ms(500);

	printString("Counterclokcwise Max\r\n");
	OCR1A = PULSE_MAX;
	_delay_ms(500);

	printString("Center\r\n");
	OCR1A = PULSE_MID;
	_delay_ms(500);

	DDRB &= ~(1 << SERVOCONTROL);

}


// uint16_t getNumber16(void) {
// 	char thousands = '0';
// 	char hundreds = '0';
// 	char tens = '0';
// 	char ones = '0';
// 	char thisChar = '0';

// 	do {
// 		thousands = hundreds;
// 		hundreds = tens;
// 		tens = ones;
// 		ones = thisChar;
// 		thisChar = receiveByte();
// 		transmitByte(thisChar);
// 	} while (thisChar != '\r');

// 	transmitByte('\n');
// 	return (1000 * (thousands - '0') + 100 * (hundreds - '0') +
// 		10 * (tens - '0') + ones - '0');
// }

