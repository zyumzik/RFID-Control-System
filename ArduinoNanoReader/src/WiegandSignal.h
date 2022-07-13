#include <Arduino.h>

class WiegandSignal
{
public:
    enum Length
    {
        s_none = 0,
        s_short = 100,
        s_medium = 300,
        s_long = 750
    };

    bool is_invoke;

    WiegandSignal(uint8_t led_pin, uint8_t zum_pin);

    // start async blinking and beeping. use update() function in loop(). returns time of signaling in ms
    void invoke(Length length, uint8_t count);

    // updates timer and makes signal if period passed.
    void update(Length length = s_none, bool zum = true, bool led = true);
private:
    bool        state;
    Length      current_length;
    uint32_t    timer;
    uint8_t     invoke_counter;
    uint8_t     led_pin;
    uint8_t     zum_pin;
};

