#include <msp430fr2355.h>
#include "temp_sensor.h"
#include "keypad.h"
#include "statusled.h"
#include <string.h>
#include <stdint.h>

volatile int state_variable = 0;
char keypad_input[4] = {};
volatile int input_index = 0;
volatile int send_i2c_update = 0;
volatile int pattern = -1; // Current pattern
volatile int step[4] = {0, 0, 0, 0}; // Current step in each pattern
volatile float base_tp = 0.5;    // Default 1.0s

const uint8_t pattern_0 = 0b10101010;
const int pattern_1[4] = {0b10101010, 0b10101010, 0b01010101, 0b01010101};  // Pattern 1
const int pattern_3[6] = {0b00011000, 0b00100100,   // Pattern 3
                          0b01000010, 0b10000001,
                          0b01000010, 0b00100100};

void setup_heartbeat() {
    // --    LED   --
    
    P6DIR |= BIT6;                                                      // P6.6 as OUTPUT
    P6OUT |= BIT6;                                                      // Start LED off

    // -- Timer B0 --
    TB0R = 0;
    TB0CCTL0 = CCIE;                                                    // Enable Interrupt
    TB0CCR0 = 32820;                                                    // 1 sec timer
    TB0EX0 = TBIDEX__8;                                                 // D8
    TB0CTL = TBSSEL__SMCLK | MC__UP | ID__4;                            // Small clock, Up counter,  D4
    TB0CCTL0 &= ~CCIFG;
}

void setup_ledbar_update_timer() {
    TB1CTL = TBSSEL__ACLK | MC__UP | ID__4;                             // Use ACLK, up mode, divider 4
    TB1CCR0 = (int)((32768 * base_tp) / 4.0);                           // Set update interval based on base_tp
    TB1CCTL0 = CCIE;                                                    // Enable interrupt for TB1 CCR0
}

void rgb_timer_setup() {
    P3DIR |= (BIT2 | BIT7);                                 // Set as OUTPUTS
    P2DIR |= BIT4;
    P3OUT |= (BIT2 | BIT7);                                 // Start HIGH
    P2OUT |= BIT4;

    TB2R = 0;
    TB2CTL |= (TBSSEL__SMCLK | MC__UP);                     // Small clock, Up counter
    TB2CCR0 = 512;                                          // 1 sec timer
    TB2CCTL0 |= CCIE;                                       // Enable Interrupt
    TB2CCTL0 &= ~CCIFG;
}

uint8_t compute_ledbar() {
    uint8_t led_pins = 0;
    switch (pattern) {
        case 0:
            led_pins = pattern_0;
            break;
        case 1:
            led_pins = pattern_1[step[pattern]];
            step[pattern] = (step[pattern] + 1) % 4;
            break;
        case 2:
            led_pins = step[pattern];
            step[pattern] = (step[pattern] + 1) % 255;
            break;
        case 3:
            led_pins = pattern_3[step[pattern]]; // Pattern 3
            step[pattern] = (step[pattern] + 1) % 6; // advance to the next step
            break;
        case -1: 
            led_pins = 0;
        default:
            break; 
    }
    return led_pins;
}

void change_led_pattern(int new_pattern) {
    if (new_pattern == pattern) {
        step[pattern] = 0;  // Just reset the step count if the same pattern is selected
    }

    pattern = new_pattern;
}

void update_slave_ledbar() {
    volatile int ledbar_pattern = compute_ledbar();
    i2c_write_led(ledbar_pattern);
}

int main(void)
{
    WDTCTL = WDTPW | WDTHOLD;               // Stop watchdog timer
    
    P1DIR |= BIT0;
    P1OUT &= ~BIT0;
    i2c_master_setup();
    setup_keypad();
    setup_heartbeat();
    setup_ledbar_update_timer();
    rgb_timer_setup();
    setup_temp_timer();
    setup_ADC();

    send_buff = 0;
    ready_to_send = 0;

    PM5CTL0 &= ~LOCKLPM5;                   // Disable the GPIO power-on default high-impedance mode
                                            // to activate previously configured port settings
    UCB0CTLW0 &= ~UCSWRST;                // Take out of reset
    UCB0IE |= UCTXIE0;
    UCB0IE |= UCRXIE0;

    __enable_interrupt();

    while(1)
    {
// ----------------------------------------------------------------------------------------------------------------------------------------
// ------------- LOCKED -------------------------------------------------------------------------------------------------------------------
// ----------------------------------------------------------------------------------------------------------------------------------------
        char key = pressed_key();
        if (state_variable == 0 || state_variable == 2) {                      // Locked
            change_led_pattern(-1);
            if (key != '\0') {                                                 // Check for key
                
                state_variable = 2;                                            // if key, unlocking
                if (input_index < 3) {                                         
                    keypad_input[input_index] = key;
                    input_index++;
                } else if (input_index == 3) {                                 // if 4 keys, check unlock
                    
                    check_key();
                }
            }   
// ----------------------------------------------------------------------------------------------------------------------------------------
// ------------- UNLOCKED -----------------------------------------------------------------------------------------------------------------
// ----------------------------------------------------------------------------------------------------------------------------------------
        } else if(state_variable == 1) {
            // --------------------------------------------
            // ------------ MODE SELECT -------------------
            // --------------------------------------------
            if (key != '\0') {                                                  // LOCK
                if(key == 'D') {                                            
                    change_led_pattern(-1);
                    state_variable = 0;
                    input_index = 0;
                    send_i2c_update = 0;                
                    memset(keypad_input, 0, sizeof(keypad_input));              // Clear input
                    //update_lcd(0xFE, 0xFE, 0xFE);
                }

                else if (key == '*') {                                          // LED pattern update mode
                    //update_LCD();
                    state_variable = 3;
                    input_index = 0;
                    memset(keypad_input, 0, sizeof(keypad_input));
                    //update_LCD(10, 0xFF, 0xFF);
                }
                else if (key == '#') {                                          // Window size select mode
                    //update_LCD();                                          
                    state_variable = 4;
                    input_index = 0;
                    memset(keypad_input, 0 sizeof(keypad_input));
                    //update_LCD(11, 0xFF, 0xFF);
                }
                else if (key == 'A') {
                    if (base_tp > 0.25) {
                        base_tp -= 0.25;
                    }
                    input_variable = 0;
                    memset(keypad_input, 0, sizeof(keypad_input));
                    break;
                }
                else if (key == 'B') {
                    base_tp += 0.25;
                    input_variable = 0;
                    memset(keypad_input, 0, sizeof(keypad_input));
                    break;
                }
            }
            if(temp_update_flag) {
                temp_update_flag = 0;
                if (count >= window_size) {
                    float sum = 0.0f;
                    for(int i = 0; i < window_size; i++) {
                        sum += mov_avg_buffer[i];
                    }
                    int moving_average = (int) 10*(sum/window_size);
                    //update_LCD(0xFF, moving_average, 0xFF);
                }
            }
        }
        // --------------------------------------------
        // ---------- LED PATTERN SELECTION -----------
        // --------------------------------------------
        else if (state_variable == 3) {
            char key = pressed_key();
            if(key != '\0') {
                send_i2c_update = 1;
                case '0':
                    //update_LCD(0, 0xFF, 0xFF);
                    change_led_pattern(0);
                    state_variable = 1;
                    input_index = 0;
                    memset(keypad_input, 0, sizeof(keypad_input));  // Clear input
                    break;
                case '1':
                    //update_LCD(1, 0xFF, 0xFF);
                    change_led_pattern(1);
                    state_variable = 1;
                    input_index = 0;
                    memset(keypad_input, 0, sizeof(keypad_input));  // Clear input
                    break;
                case '2':
                    //update_LCD(2, 0xFF, 0xFF);
                    change_led_pattern(2);
                    state_variable = 1;
                    input_index = 0;
                    memset(keypad_input, 0, sizeof(keypad_input));  // Clear input
                    break;
                case '3':
                    //update_LCD(3, 0xFF, 0xFF);
                    change_led_pattern(3);
                    state_variable = 1;
                    input_index = 0;
                    memset(keypad_input, 0, sizeof(keypad_input));  // Clear input
                    break;
                case '4':
                    //update_LCD(4, 0xFF, 0xFF);
                    change_led_pattern(4);
                    state_variable = 1;
                    input_index = 0;
                    memset(keypad_input, 0, sizeof(keypad_input));  // Clear input
                    break;
                case '5':
                    //update_LCD(5, 0xFF, 0xFF);
                    change_led_pattern(5);
                    state_variable = 1;
                    input_index = 0;
                    memset(keypad_input, 0, sizeof(keypad_input));  // Clear input
                    break;
                case '6':
                    //update_LCD(6, 0xFF, 0xFF);
                    change_led_pattern(6);
                    state_variable = 1;
                    input_index = 0;
                    memset(keypad_input, 0, sizeof(keypad_input));  // Clear input
                    break;
                case '7':
                    //update_LCD(7, 0xFF, 0xFF);
                    change_led_pattern(7);
                    state_variable = 1;
                    input_index = 0;
                    memset(keypad_input, 0, sizeof(keypad_input));  // Clear input
                    break;
                case '8':
                    //update_LCD(8, 0xFF, 0xFF);
                    change_led_pattern(8);
                    state_variable = 1;
                    input_index = 0;
                    memset(keypad_input, 0, sizeof(keypad_input));  // Clear input
                    break;
                case '9':
                    //update_LCD(9, 0xFF, 0xFF);
                    change_led_pattern(9);
                    state_variable = 1;
                    input_index = 0;
                    memset(keypad_input, 0, sizeof(keypad_input));  // Clear input
                    break;               
            }
        }
        // --------------------------------------------
        // ---------- WINDOW SIZE SELECTION -----------
        // --------------------------------------------
        else if (state_variable == 4) {
            char key = pressed_key();
            if(key != '\0') {
                keypad_input[input_index] = key;
                input_index++;
            }
            else if (key == 'C') {
                int new_window = atoi(keypad_input);
                if (new_window > 0 && new_window < 100) {
                    window_size = new_window;
                    count = 0;
                }
                state_variable = 1;
                input_input = 0;
                memset(keypad_input, 0, sizeof(keypad_input));
                //update_LCD(0xFF, 0xFF, window_input);
            }
        }
    }
}

// ----------------------------------------------------------------------------------------------------------------------------------------
// ------------- INTERRUPTS ---------------------------------------------------------------------------------------------------------------
// ----------------------------------------------------------------------------------------------------------------------------------------

#pragma vector=TIMER0_B0_VECTOR
__interrupt void Timer_B0_ISR(void) {
    TB0CCTL0 &= ~CCIFG;
    P6OUT ^= BIT6;
}

#pragma vector = TIMER1_B0_VECTOR
__interrupt void Timer_B1_ISR(void) {
    TB1CCTL0 &= ~CCIFG;

    if (send_i2c_update) {
        update_slave_ledbar();    
    }
    
    TB1CCR0 = (int)((32768 * base_tp) / 4.0);
}

#pragma vector=EUSCI_B0_VECTOR
__interrupt void EUSCI_B0_ISR(void){
    P1OUT |= BIT0;
    int current = UCB0IV;
    switch(current) {
        case 0x02:  // TXIFG
            UCB0TXBUF = send_buff;
            ready_to_send = 1;
            break;
        default:
            break;
    }
}
