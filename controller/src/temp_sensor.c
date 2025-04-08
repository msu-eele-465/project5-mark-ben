#include <msp430fr2355.h>

volatile int ADC_Value;
volatile float temperature;
volatile int mov_avg_index = 0;
volatile int count = 0;
volatile int temp_update_flag = 0;
volatile int window_size = 3;

volatile float mov_avg_buffer[99];

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
    TB3R = 0;
    TB3CTL |= (TBSSEL__SMCLK | MC__UP);                     // Small clock, Up counter
    TB3CCR0 = 256;                                          // 0.5 sec timer
    TB3CCTL0 |= CCIE;                                       // Enable Interrupt
    TB3CCTL0 &= ~CCIFG;
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

    temp_update_flag = 1;
}

#pragma vector=TIMER3_B0_VECTOR
__interrupt void Timer3_B0_ISR(void) {
    TB3CCTL0 &= ~CCIFG:

    ADCCTL0 |= ADCENC | ADCSC;                              // Trigger new ADC conversion every 0.5 seconds


}