#include <msp430.h>
#include <stdbool.h>
#include "keypad.h"

//volatile int state_variable = 0;                        // 0 = Locked, 1 = Unlocked, 2 = Unlocking

volatile int status_led_count = 0;  // Current count in the PWM cycle
volatile int red_count = 0; // Red pulse width
volatile int green_count = 0; // Green pulse width
volatile int blue_count = 0; // Blue pulse width

//3.2 - Red 
//2.4 - Green
//3.7 - Blue

// This function will update the current status of the led
void update_led(void) {

    // Update the pulse widths based on the current state
    switch (state_variable) {
        case 0:
            red_count = 0xc4 / 8;
            green_count = 0x3e / 8;
            blue_count = 0x1d / 8;
            break;
        case 1:
            red_count = 0x1d / 8;
            green_count = 0xa2 / 8;
            blue_count = 0xc4 / 8;
            break;
        case 2:
            red_count = 0xc4 / 8;
            green_count = 0x92 / 8;
            blue_count = 0x1d / 8;
            break;
    }
}

#pragma vector=TIMER2_B0_VECTOR
__interrupt void Timer2_B0_ISR(void) {

    TB2CCTL0 &= ~CCIFG;

    status_led_count++;

    // Reset the PWM cycle
    if (status_led_count > 0xff / 8) {
        status_led_count = 0;
    }

    // RED PWM
    if (status_led_count <= red_count) {
        P3OUT |= BIT2;
    } else {
        P3OUT &= ~BIT2;
    }

    // GREEN PWM
    if (status_led_count <= green_count) {
        P2OUT |= BIT4;
    } else {
        P2OUT &= ~BIT4;
    }

    // BLUE PWM
    if (status_led_count <= blue_count) {
        P3OUT |= BIT7;
    } else {
        P3OUT &= ~BIT7;
    }
}