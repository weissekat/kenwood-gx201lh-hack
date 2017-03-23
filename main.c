#define CLOCK 9600000

/*
ATTINY13A


IAR compiler!
- PB0 (5) is connected to SDA via 330 ohm resistor while CPU via 1k resistor,
      such a voltage divider helps to set 1 on the line
- PB1 (6) is connected to SCL directly
- PB2 (7) is not connected, debug info
- PB3 (2) is connected to "NEWS" button, its wires are cut off
- PB4 (3) is connected to transistor 2n3906 used for Bluetooth module enable
      high level is OFF, low level is ON
*/

#define   ENABLE_BIT_DEFINITIONS
#include <inavr.h>
#include <intrinsics.h>
#include "iotiny13a.h"

// enable register vars in IAR settings
__regvar __no_init unsigned char mode @15;
__regvar __no_init unsigned char xxx @14;

unsigned char check1(void) {
    GIFR |= (1 << INTF0); while (!(GIFR & (1 << INTF0)));
    GIFR |= (1 << INTF0);
    if ((PINB & (1 << PB0)) == 0x00) {
        xxx = PINB;
        return 1;
    }
    return 0;
}

unsigned char check0(void) {
    GIFR |= (1 << INTF0); while (!(GIFR & (1 << INTF0)));
    GIFR |= (1 << INTF0);
    if ((PINB & (1 << PB0)) != 0x00) {
        xxx = PINB;
        return 1;
    }
    return 0;
}

void main(void) {
    mode = 0;

    DDRB = 0x00;
    PORTB = 0xFE;

    // interrupt on rising edge
    MCUCR |= 0x03;

    // INT0 (PB1, line SCL)
    GIMSK = (1 << INT0);

    xxx = PINB;

    DDRB |= (1 << PB2);
    PORTB &= ~(1 << PB2);

    DDRB |= (1 << PB4);
    PORTB |= (1 << PB4);

    while(1) {
        waiting_for_start:
            if ((PINB&(1<<PB3))==0x00) {
                __delay_cycles(CLOCK/1000*300);
                if (mode) {
                    mode = 0;
                    PORTB |= (1 << PB4);
                } else {
                    mode = 1;
                    PORTB &= ~(1 << PB4);
                }
            }

            if (mode) {
                if (xxx != PINB) {
                    if (((PINB & 0x03) == 0x02) && (xxx & 0x01)) {
                        goto capturing_data;
                    }
                    xxx = PINB;
                }
            }

            goto waiting_for_start;

        capturing_data:
            // catch 140 (8C)
            if (check1()) goto waiting_for_start; // 1
            if (check0()) goto waiting_for_start; // 0
            if (check0()) goto waiting_for_start; // 0
            if (check0()) goto waiting_for_start; // 0
            if (check1()) goto waiting_for_start; // 1
            if (check1()) goto waiting_for_start; // 1
            if (check0()) goto waiting_for_start; // 0
            if (check0()) goto waiting_for_start; // 0

            if (check0()) goto waiting_for_start; // 0, ACK

            // catch 80 (50)
            if (check0()) goto waiting_for_start; // 0
            if (check1()) goto waiting_for_start; // 1
            if (check0()) goto waiting_for_start; // 0
            if (check1()) goto waiting_for_start; // 1
            if (check0()) goto waiting_for_start; // 0
            if (check0()) goto waiting_for_start; // 0
            if (check0()) goto waiting_for_start; // 0
            if (check0()) goto waiting_for_start; // 0

            if (check0()) goto waiting_for_start; // 0, ACK

            // catch & set 209 (D1)
            if (check1()) goto waiting_for_start; // 1
            if (check1()) goto waiting_for_start; // 1
            if (check0()) goto waiting_for_start; // 0
            if (check1()) goto waiting_for_start; // 1
            if (check0()) goto waiting_for_start; // 0

            // set 0, enable output
            PORTB &= ~(1 << PB0);
            DDRB |= (1 << PB0);

            // wait for 1 rising edge
            GIFR |= (1 << INTF0); while (!(GIFR & (1 << INTF0)));

            // debug info: started
            PORTB ^= ( 1<<PB2 ); PORTB ^= ( 1<<PB2 );

            // interrupt on falling edge
            MCUCR &= ~0x03; MCUCR |= 0x02;

            // wait 2 tacts
            GIFR |= (1 << INTF0); while (!(GIFR & (1 << INTF0)));
            GIFR |= (1 << INTF0); while (!(GIFR & (1 << INTF0)));

            // set 1
            PORTB |= (1<<PB0);

            // wait 1 tact
            GIFR|=(1<<INTF0); while (!(GIFR&(1<<INTF0)));

            // debug info: finished
            PORTB ^= ( 1<<PB2 ); PORTB ^= ( 1<<PB2 );

            // interrupt on rising edge
            MCUCR|=0x03;

            // disable output
            DDRB&=~(1<<PB0);

            goto waiting_for_start;
    }
}
