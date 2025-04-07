#include <msp430fr2355.h>

#define MAX_WINDOW_SIZE 99;

volatile int ADC_Value;
volatile float temperature;
volatile int mov_avg_index = 0;
volatile int count = 0;
volatile int update_mov_avg_flag = 0;
volatile int window_size = 3;

volatile float mov_avg_buffer[MAX_WIDNOW_SIZE];

void setup_ADC() {

    ADCCTL0 &= ~ADCSHT;                                     // Clear ADCSHT
    ADCCTL0 |= ADCSHT_2;                                    // Conversion Cycles = 16
    ADCCTL0 |= ADCON;                                       // Turn ADC on
    
    ADCCTL1 |= ADCCSEL_2;                                   // SMCLK Clock source
    ADCCTL1 |= ADCSHP;                                      // Sample signal source = timer

    ADCCTL2 &= ~ADCRES;                                     // Clear ADCRES
    ADCCTL2 |= ADCRES_2;                                    // Resolution = 12 Bit

    ADCMCTL0 |= ADCINCH_1;                                  // ADC input channel = A1 (P1.1)

    ADCIE |= ADCIE0;                                        // Enable ADC Conv Complete IRQ

}

void setup_temp_timer() {
    TB2R = 0;
    TB2CTL |= (TBSSEL__SMCLK | MC__UP);                     // Small clock, Up counter
    TB2CCR0 = 256;                                          // 0.5 sec timer
    TB2CCTL0 |= CCIE;                                       // Enable Interrupt
    TB2CCTL0 &= ~CCIFG;
}

#pragma vector=ADC_VECTOR
__interrupt void ADC_ISR(void){
    ADC_Value = ADCMEM0;                                    // Read ADC value

    float voltage = ((float)ADC_value * 3.3) / 4096;        // Voltage is ADC_value * reference voltage / adc max value

    temperature = (voltage / -0.01)                         // temperature is voltage / slope from datasheet

    mov_avg_buffer[mov_avg_index] = temperature;            // Update moving average
    mov_avg_index = (mov_avg_index + 1) % window_size;      // Update index until moving average length

    if (count < window_size) {
        count++;
    }

    update_mov_avg_flag = 1;
}

#pragma vector=TIMER2_B0_VECTOR
__interrupt void Timer2_B0_ISR(void) {
    TB2CCTL0 &= ~CCIFG:

    ADCCTL0 |= ADCENC | ADCSC;                              // Trigger new ADC conversion every 0.5 seconds


}