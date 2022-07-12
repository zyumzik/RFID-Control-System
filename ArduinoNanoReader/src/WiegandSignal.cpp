#include "WiegandSignal.h"

WiegnadSignal::WiegnadSignal(uint8_t led_pin, uint8_t zum_pin)
{
    this->led_pin = led_pin;
    this->zum_pin = zum_pin;

    pinMode(led_pin, OUTPUT);
    pinMode(zum_pin, OUTPUT);
}

void WiegnadSignal::blink(uint8_t blink_time, uint8_t blink_period, uint8_t count)
{
    for (size_t i = 0; i < count; i++)
    {
        digitalWrite(led_pin, HIGH);
        delay(blink_time);
        digitalWrite(led_pin, LOW);
        delay(blink_period);
    }
}

void WiegnadSignal::beep(uint8_t beep_time, uint8_t beep_period, uint8_t count)
{
    for (size_t i = 0; i < count; i++)
    {
        digitalWrite(zum_pin, HIGH);
        delay(beep_time);
        digitalWrite(zum_pin, LOW);
        delay(beep_period);
    }
}

void WiegnadSignal::updateLed(uint8_t blink_time, uint8_t blink_period)
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

void WiegnadSignal::updateZum(uint8_t beep_time, uint8_t beep_period)
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
