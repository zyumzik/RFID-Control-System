#include <Arduino.h>

class WiegnadSignal
{
public:
    WiegnadSignal(uint8_t led_pin, uint8_t zum_pin);

    void blink(uint8_t blink_time, uint8_t blink_period, uint8_t count);
    void beep(uint8_t zum_time, uint8_t zum_period, uint8_t count);

    void updateLed(uint8_t blink_time, uint8_t blink_period);
    void updateZum(uint8_t beep_time, uint8_t beep_period);
private:
    //void setSignal(uint8_t pin, uint8_t value)

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