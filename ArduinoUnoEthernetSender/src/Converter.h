#include <Arduino.h>

static size_t digits(unsigned long l)
{
    if (l / 10 == 0)
        return 1;
    return 1 + digits(l / 10);
}

static String S(unsigned long l)
{
    String result = "";

    if (l <= 0)
        return result += "0";

    for (size_t i = digits(l); i >= 0; i--)
    {
        result.setCharAt(i, l % 10);
        l /= 10;
    }
    
}
