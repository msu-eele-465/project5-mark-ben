#ifndef TEMP_SENSOR_H
#define TEMP_SENSOR_H



extern volatile float mov_avg_buffer[100];
extern volatile int ADC_value;
extern volatile float temperature;
extern volatile int mov_avg_index;
extern volatile int count;
extern volatile int temp_update_flag;
extern volatile int window_size;     
extern volatile float moving_average;
extern volatile float running_sum;

void setup_ADC(void);
void setup_temp_timer(void);
void compute_temp(void);

#endif