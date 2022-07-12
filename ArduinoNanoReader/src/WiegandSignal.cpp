#include "WiegandSignal.h"

WiegandSignal::WiegandSignal(uint8_t led_pin, uint8_t zum_pin)
{
    this->led_pin = led_pin;
    this->zum_pin = zum_pin;

    pinMode(led_pin, OUTPUT);
    pinMode(zum_pin, OUTPUT);
}

void WiegandSignal::signal(SignalLength length, uint8_t count)
{
    for (size_t i = 0; i < count; i++)
    {
        digitalWrite(zum_pin, HIGH);
        digitalWrite(led_pin, HIGH);

        delay(length);

        digitalWrite(zum_pin, LOW);
        digitalWrite(led_pin, LOW);

        delay(length);
    }
}

void WiegandSignal::updateLed(uint8_t blink_time, uint8_t blink_period)
{
    if (led_state)
    {
        if (millis() >= led_timer + blink_time)
        {
            led_timer = millis();
            led_state = !led_state;
            digitalWrite(led_pin, led_state);
        }
    }
    else
    {
        if (millis() >= led_timer + blink_period)
        {
            led_timer = millis();
            led_state = !led_state;
            digitalWrite(led_pin, led_state);
        }
    }
}

void WiegandSignal::updateZum(uint8_t beep_time, uint8_t beep_period)
{
    if (zum_state)
    {
        if (millis() >= zum_timer + beep_time)
        {
            zum_timer = millis();
            zum_state = !zum_state;
            digitalWrite(zum_pin, zum_state);
        }
    }
    else
    {
        if (millis() >= zum_timer + beep_period)
        {
            zum_timer = millis();
            zum_state = !zum_state;
            digitalWrite(zum_pin, zum_state);
        }
    }
}
