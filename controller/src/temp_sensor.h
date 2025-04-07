#ifndef TEMP_SENSOR_H
#define TEMP_SENSOR_H

#include <msp430fr2355.h>

#define MAX_WINDOW_SIZE;                             // Maximum moving average sample size
extern volatile float sampleBuffer[MAX_WINDOW_SIZE];
volatile int ADC_Value;
volatile float temperature;
volatile int mov_avg_index;
volatile int count;
volatile int update_mov_avg_flag;
volatile int window_size;     

void setup_ADC(void);
void setup_temp_timer(void);


#endif