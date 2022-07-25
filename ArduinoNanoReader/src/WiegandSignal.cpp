#include "WiegandSignal.h"

WiegandSignal::WiegandSignal(uint8_t led_pin, uint8_t zum_pin)
{
    this->led_pin = led_pin;
    this->zum_pin = zum_pin;

    pMode(led_pin, OUTPUT);
    pMode(zum_pin, OUTPUT);
}

void WiegandSignal::invoke(Length length, uint8_t count)
{
    is_invoke = true;
    current_length = length;
    invoke_counter = count * 2;
    state = false;
    dWrite(led_pin, state);
    dWrite(zum_pin, state);
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
                dWrite(led_pin, state);
            if (zum)
                dWrite(zum_pin, state);
        }
    }

    // simple update (using counter)
    if (length == s_none && zum && led && is_invoke)
    {
        if (millis() >= timer + current_length)
        {
            timer = millis();
            state = !state;
            dWrite(led_pin, state);
            dWrite(zum_pin, state);
            invoke_counter--;
            if (invoke_counter <= 0)
            {
                is_invoke = false;
            }
        }
    }
}
