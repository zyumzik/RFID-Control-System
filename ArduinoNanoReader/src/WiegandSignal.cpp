#include "WiegandSignal.h"

WiegandSignal::WiegandSignal(uint8_t led_pin, uint8_t zum_pin)
{
    this->led_pin = led_pin;
    this->zum_pin = zum_pin;

    pinMode(led_pin, OUTPUT);
    pinMode(zum_pin, OUTPUT);
}

void WiegandSignal::invoke(Length length, uint8_t count)
{
    is_invoke = true;
    current_length = length;
    invoke_counter = count * 2;
    state = false;
    digitalWrite(led_pin, state);
    digitalWrite(zum_pin, state);
}

void WiegandSignal::update(Length length = s_none, bool zum = true, bool led = true)
{
    // update with periodic signal (without counter)
    if (length != s_none)
    {
        if (millis() >= timer + length)
        {
            timer = millis();
            state = !state;
            if (led)
                digitalWrite(led_pin, state);
            if (zum)
                digitalWrite(zum_pin, state);
        }
    }

    // simple update (using counter)
    if (length == s_none && zum && led && is_invoke)
    {
        if (millis() >= timer + current_length)
        {
            timer = millis();
            state = !state;
            digitalWrite(led_pin, state);
            digitalWrite(zum_pin, state);
            invoke_counter--;
            if (invoke_counter <= 0)
            {
                is_invoke = false;
            }
        }
    }
}
