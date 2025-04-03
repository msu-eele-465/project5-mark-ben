#include "intrinsics.h"
#include <msp430fr2310.h>
#include <stdbool.h>

// ------- Global Variables ------------
volatile char button_pressed = ' ';
volatile int curr_pattern = -1;
volatile int state = 0;
int cursor = 0;
int blink = 0;
const int slave_addr = 0x0002;
// -------------------------------------


void lcd_raw_send(int send_data, int num) {
    int send_data_temp;
    int nibble;
    int i = 0;
    int busy = 1;
    send_data_temp = send_data;

    while (i < num) {

        // Get top nibble
        if (num == 2) {
            nibble = send_data_temp & 0b11110000;
            send_data_temp = send_data_temp << 4;
        } else {
            nibble = send_data_temp & 0b00001111;
            send_data_temp = send_data_temp >> 4;
            nibble = nibble << 4;
        }
   
        // Output nibble
        P1OUT &= ~(BIT4 | BIT5 | BIT6 | BIT7);
        P1OUT |= nibble;

        // Set enable high
        P1OUT |= BIT1;
        _delay_cycles(1000);

        // Wait and drop enable
        _delay_cycles(1000);
        P1OUT &= ~BIT1;
        _delay_cycles(1000);

        i++;
    }
    
    P1DIR &= ~(BIT4 | BIT5 | BIT6 | BIT7); // Set input
    P1OUT |= BIT0;
    P2OUT &= ~BIT6;

    // Check busy flag
    while (busy != 0) {
        _delay_cycles(1000);
        P1OUT |= BIT1;       // Enable high
        _delay_cycles(1000);

        busy = P1IN & BIT7;  // Read busy

        _delay_cycles(1000);
        P1OUT &= ~BIT1;     // Enable low

        _delay_cycles(1000);
        P1OUT |= BIT1;     // Enable high
        _delay_cycles(2000);
        P1OUT &= ~BIT1;    // Enable low
    }
    _delay_cycles(1000);

    P1DIR |= (BIT4 | BIT5 | BIT6 | BIT7);  // Set output
    P1OUT &= ~BIT0;
}


void lcd_string_write(char* string) {
    int i = 0;

    while (string[i] != '\0') {
        P2OUT |= BIT6;
        lcd_raw_send((int)string[i], 2);
        i++;
    }
}

void update_pattern(char* string) {
    lcd_raw_send(0b00000010, 2);

    lcd_string_write(string);
}

void update_key(char c) {
    char string[] = {'\0', '\0'};
    string[0] = c;

    lcd_raw_send(0xCF, 2);

    lcd_string_write(string); 
}

int main(void)
{
    char last_pressed;
    int last_pattern;

    // Stop watchdog timer
    
    WDTCTL = WDTPW | WDTHOLD;

    last_pressed = button_pressed;
    last_pattern = curr_pattern;

    P2OUT &= ~BIT0;
    P2DIR |= BIT0;

    P1OUT &= ~BIT0; // P1.0 is R/W
    P1DIR |= (BIT0 | BIT1 | BIT4 | BIT5 | BIT6 | BIT7);

    P2OUT &= ~BIT6; // RS PIN
    P2DIR |= BIT6;

    P1SEL0 |= BIT2;  // Confiure I2C
    P1SEL0 |= BIT3;
    P1SEL1 &= ~BIT2;
    P1SEL1 &= ~BIT3;

    // Init I2C
    UCB0CTLW0 |= UCSWRST;  // Software reset
    UCB0CTLW0 &= ~UCSSEL_3;  // Input clock
    UCB0CTLW0 |= UCMODE_3;  // I2C mode
    UCB0I2COA0 = slave_addr | UCOAEN; // My slave address
    //UCB0CTLW1 |= UCSWACK;
    UCB0CTLW0 &= ~UCSWRST;  // Disable software reset
    UCB0IE |= (UCRXIE0 | UCSTPIE); // Enable interrupts
    //UCB0CTLW0 |= UCTXACK;

    // Disable low-power mode / GPIO high-impedance
    PM5CTL0 &= ~LOCKLPM5;


    // Init LCD
    lcd_raw_send(0b110000100010, 3); // Turn on LCD in 2-line mode
    lcd_raw_send(0b00001100, 2); // Display on, cursor off, blink off
    lcd_raw_send(0b00000001, 2); // Clear display
    lcd_raw_send(0b00000110, 2); // Increment mode, entire shift off

    __enable_interrupt();

    //int i = 0;
    //while (true) {
    //    char c[] = {'\0', '\0'};
    //    c[0] = (char)i;
    //    lcd_string_write(&c);
    //    i++;
    //    i = i % 256;
    //}


    while (true)
    {

        // Delay for 100000*(1/MCLK)=0.1s
        __delay_cycles(100000);

        if (curr_pattern == 11) {
            curr_pattern = last_pattern;
            if (button_pressed == 'C') {
                cursor ^= BIT0;
                lcd_raw_send(0b00001100 | (cursor << 1) | (blink), 2);
            } else if (button_pressed == '9') {
                blink ^= BIT0;
                lcd_raw_send(0b00001100 | (cursor << 1) | (blink), 2);
            }
        }

        if (last_pressed != button_pressed) {
            update_key(button_pressed);
            last_pressed = button_pressed;
        }

        if (last_pattern != curr_pattern) {
            switch (curr_pattern) {
                case 0:
                    update_pattern("static        ");
                    break;
                case 1:
                    update_pattern("toggle        ");
                    break;
                case 2:
                    update_pattern("up counter    ");
                    break;
                case 3:
                    update_pattern("in and out    ");
                    break;
                case 4:
                    update_pattern("down counter  ");
                    break;
                case 5:
                    update_pattern("rotate 1 left ");
                    break;
                case 6:
                    update_pattern("rotate 7 right");
                    break;
                case 7:
                    update_pattern("fill left     ");
                    break;
                case 10:
                    update_pattern("              ");
            }
            last_pattern = curr_pattern;
        }
    }
}

#pragma vector=EUSCI_B0_VECTOR
__interrupt void EUSCI_B0_I2C_ISR(void){
    int current = UCB0IV;
    int read_data;

    switch(current) {
    case 0x08: // Rx stop condition
        state = 0;
        P2OUT ^= BIT0;
        //UCB0CTLW0 |= UCTXACK;
        break;
    case 0x16: // Rx data
        read_data = UCB0RXBUF;
        switch(state) {
        case 0: // Rx pattern
            curr_pattern = read_data;
            break;
        case 1: // Rx character
            button_pressed = (char)(read_data);
            break;
        }
        state++;
        break;
    }
}