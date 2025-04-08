#include "msp430fr2355.h"
#include <msp430.h>
#include "i2c_master.h"


void i2c_master_setup(void) {
    //-- eUSCI_B0 --
    UCB0CTLW0 |= UCSWRST;

    UCB0CTLW0 |= UCSSEL__SMCLK;              // SMCLK
    UCB0BRW = 10;                       // Divider

    UCB0CTLW0 |= UCMODE_3;              // I2C Mode
    UCB0CTLW0 |= UCMST;                 // Master
    UCB0CTLW0 |= UCTR;                  // Tx
    UCB0CTLW1 |= UCASTP_2;
    //-- Configure GPIO --------
    P1SEL1 &= ~BIT3;           // eUSCI_B0
    P1SEL0 |= BIT3;

    P1SEL1 &= ~BIT2;
    P1SEL0 |= BIT2;

    
}

void update_LCD(int modeID, int temperature, int window_size) {
    UCB0CTLW0 |= UCTR;  // Transmit mode
    UCB0I2CSA = 0x02;   // Slave address

    UCB0TBCNT = 3;

    UCB0IFG &= ~UCSTPIFG;

// -- First Byte --

    send_buff = modeID;
    UCB0TXBUF = send_buff;

    UCB0CTLW0 |= UCTXSTT;

     unsigned int timeout = 10000;
    while (!(ready_to_send) && timeout--) {
    }
    if (timeout == 0) {
        return;
    } 
    ready_to_send = 0;

// -- Second Byte --

    send_buff = temperature;
    UCB0TXBUF = send_buff;

    UCB0CTLW0 |= UCTXSTT;

     unsigned int timeout = 10000;
    while (!(ready_to_send) && timeout--) {
    }
    if (timeout == 0) {
        return;
    } 
    ready_to_send = 0;

// -- Third Byte --
    send_buff = window_size;
    UCB0TXBUF = send_buff;

    UCB0CTLW0 |= UCTXSTT;

     unsigned int timeout = 10000;
    while (!(ready_to_send) && timeout--) {
    }
    if (timeout == 0) {
        return;
    } 
    ready_to_send = 0;

    timeout = 10000;
    while (!(UCB0IFG & UCSTPIFG) && timeout--) {
        ;
    } 
    UCB0IFG &= ~UCSTPIFG;
}

void i2c_write_lcd(unsigned int pattNum, char character) {
    UCB0CTLW0 |= UCSWRST;
    UCB0CTLW0 |= UCTR;
    UCB0I2CSA = 0x0002;
    UCB0TBCNT = 0x02;                   // Number of bytes
    UCB0CTLW0 &= ~UCSWRST;
    UCB0IE |= UCTXIE0;

    send_buff = pattNum;                // Send data byte
    UCB0CTLW0 |= UCTXSTT;               // Generate Start Condition

    
    while (!(ready_to_send));       // Wait for TX Buffer
    ready_to_send = 0;
    


    send_buff = (int)character;                // Send data byte
    while (!(ready_to_send));       // Wait for TX Buffer
    ready_to_send = 0;

    //while((UCB0IFG & UCSTPIFG) == 0){};
    //    UCB0IFG &= ~UCSTPIFG;
}

void i2c_write_led(unsigned int pattNum) {

    UCB0CTLW0 |= UCTR;  // Transmit mode
    UCB0I2CSA = 0x40;   // Slave address

    UCB0TBCNT = 1;

    UCB0IFG &= ~UCSTPIFG;

    send_buff = pattNum;
    UCB0TXBUF = send_buff;

    UCB0CTLW0 |= UCTXSTT;

     unsigned int timeout = 10000;
    while (!(ready_to_send) && timeout--) {
    }
    if (timeout == 0) {
        return;
    } 
    ready_to_send = 0;


    timeout = 10000;
    while (!(UCB0IFG & UCSTPIFG) && timeout--) {
        ;
    } 
    UCB0IFG &= ~UCSTPIFG;
}