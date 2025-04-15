#include <msp430.h>
#include <math.h>

volatile unsigned int ADC_value;
volatile int temperature;
volatile unsigned int mov_avg_index = 0;
volatile unsigned int count = 0;
volatile unsigned int temp_update_flag = 0;
volatile unsigned int window_size = 3;
volatile float mov_avg_buffer[100];
volatile float numerator;
volatile float denominator;
volatile float add;
volatile float base;
volatile float square;
volatile float running_sum = 0.0f;
volatile float moving_average = 0.0f;
volatile int sample_ready = 0;

void setup_ADC() {

    P1SEL0 |= BIT1;
    P1SEL1 |= BIT1;

    ADCCTL0 &= ~ADCSHT;                                     // Clear ADCSHT
    ADCCTL0 |= ADCSHT_2;                                    // Conversion Cycles = 16
    ADCCTL0 |= ADCON;                                       // Turn ADC on
    
    ADCCTL1 |= ADCSSEL_2;                                   // SMCLK Clock source
    ADCCTL1 |= ADCSHP;                                      // Sample signal source = timer

    ADCCTL2 &= ~ADCRES;                                     // Clear ADCRES
    ADCCTL2 |= ADCRES_2;                                    // Resolution = 12 Bit

    ADCMCTL0 |= ADCINCH_1;                                  // ADC input channel = A1 (P1.1)

    ADCIE |= ADCIE0;                                        // Enable ADC Conv Complete IRQ

}

void setup_temp_timer() {
    TB3R = 0;
    TB3CTL |= (TBSSEL__ACLK | MC__UP);                     // Small clock, Up counter
    TB3CCR0 = 16000;                                          // 0.5 sec timer
    TB3CCTL0 |= CCIE;                                       // Enable Interrupt
    TB3CCTL0 &= ~CCIFG;
}

void compute_temp() {
    if (sample_ready) {
        __disable_interrupt();
        sample_ready = 0;
        float voltage = ADC_value * 0.000805f;

        numerator = 1.8639f - voltage;
        denominator = 3.88f * .00001f;
        add = 2.1962f * 1000000;
        base = (numerator/denominator) + add;

        square = sqrt(base);
        //square = powf(base,0.5f);
        float new_sample = (-1481.96f + square);
        new_sample = 10 * new_sample;
        if (count < window_size) {
            mov_avg_buffer[mov_avg_index] = new_sample;
            running_sum += new_sample;
            count++;
            mov_avg_index = (mov_avg_index + 1) % window_size;
        } else {
            running_sum -= mov_avg_buffer[mov_avg_index];
            mov_avg_buffer[mov_avg_index] = new_sample;
            running_sum += new_sample;
            mov_avg_index = (mov_avg_index + 1) % window_size;
            
            moving_average = running_sum / window_size;
            moving_average = 10 * moving_average;
            temp_update_flag = 1;
        } 
        __enable_interrupt();
    }
    
}

#pragma vector=ADC_VECTOR
__interrupt void ADC_ISR(void){    

    ADC_value = ADCMEM0;                                    // Read ADC value
    sample_ready = 1;
    
}

#pragma vector=TIMER3_B0_VECTOR
__interrupt void Timer3_B0_ISR(void) {
    TB3CCTL0 &= ~CCIFG;

    ADCCTL0 |= ADCENC | ADCSC;                              // Trigger new ADC conversion every 0.5 seconds
}

