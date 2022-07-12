#include <Arduino.h>

class WiegnadSignal
{
public:
    WiegnadSignal(uint8_t led_pin, uint8_t zum_pin);

    // blink with led for count times (function uses delay)
    void blink(uint8_t blink_time, uint8_t blink_period, uint8_t count);
    // beep with zummer for count times (function uses delay)
    void beep(uint8_t zum_time, uint8_t zum_period, uint8_t count);

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