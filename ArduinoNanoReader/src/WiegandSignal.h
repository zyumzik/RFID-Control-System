#include <Arduino.h>

class WiegandSignal
{
public:
    WiegandSignal(uint8_t led_pin, uint8_t zum_pin);

    // blink and beep for time with period for count times (function uses delay!)
    void signal(SignalLength length, uint8_t count);

    // blink with led (via timer) till you call this function
    void updateLed(uint8_t blink_time, uint8_t blink_period);
    // beep with zummer (via timer) till you call this function
    void updateZum(uint8_t beep_time, uint8_t beep_period);
private:
    bool led_state;
    uint32_t led_timer;
    uint8_t led_pin;
    uint8_t led_time;
    uint8_t led_period;

    bool zum_state;
    uint32_t zum_timer;
    uint8_t zum_pin;
    uint8_t zum_time;
    uint8_t zum_period;
};

enum SignalLength 
{
    s_short = 250,
    s_medium = 500,
    s_long = 1000
};