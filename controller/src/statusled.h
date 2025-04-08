#ifndef STATUSLED_H
#define STATUSLED_H


// Global state variable: 0 = Locked, 1 = Unlocked, 2 = Unlocking
extern volatile int state_variable;

extern volatile int status_led_count;  
extern volatile int red_count;
extern volatile int green_count;
extern volatile int blue_count;

void update_led(void);

#endif