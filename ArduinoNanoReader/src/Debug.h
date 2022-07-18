#include <Arduino.h>

class Debug
{
public:
    static void begin(unsigned long baud = 9600, bool time_log = false)
    {
        Serial.begin(baud);
        while(!Serial);
        debug = true;
        show_time = time_log;
    }
    static void stop()
    {
        debug = false;
    }

    static void print(const Printable& p)
    {
        if (!debug)
            return;

        Serial.print(p);
    }
    static void print(const String& s)
    {
        if (!debug)
            return;
        
        Serial.print(s);
    }
    static void print(int i)
    
    {
        if (!debug)
            return;

        Serial.print(i);
    }
    static void print(unsigned int i)
    {
        if (!debug)
            return;

        Serial.print(i);
    }
    static void print(long l)
    {
        if (!debug)
            return;

        Serial.print(l);
    }
    static void print(unsigned long l)
    {
        if (!debug)
            return;

        Serial.print(l);
    }
    static void print(unsigned char c)
    {
        if (!debug)
            return;

        Serial.print(c);
    }
    static void print(const char* c)
    {
        if (!debug)
            return;

        Serial.print(c);
    }
    
    static void log()
    {
        if (!debug)
            return;

        Serial.println();
    }
    static void log(const Printable& p)
    {
        if (!debug)
            return;
            
        if (show_time)
        {
            Serial.println(time());
        }

        Serial.println(p);
        Serial.println();
    }
    static void log(const String& s)
    {
        if (!debug)
            return;
            
        if (show_time)
        {
            Serial.println(time());
        }

        Serial.println(s);
        Serial.println();
    }
    static void log(const char* s)
    {
        if (!debug)
            return;
            
        if (show_time)
        {
            Serial.println(time());
        }

        Serial.println(s);
        Serial.println();
    }
    static void log(long l)
    {
        if (!debug)
            return;
            
        if (show_time)
        {
            Serial.println(time());
        }

        Serial.println(l);
        Serial.println();
    }
    static void log(unsigned long l)
    {
        if (!debug)
            return;
            
        if (show_time)
        {
            Serial.println(time());
        }

        Serial.println(l);
        Serial.println();
    }
    static void log(unsigned char c)
    {
        if (!debug)
            return;

        if (show_time)
        {
            Serial.println(time());
        }

        Serial.println(c);
        Serial.println();
    }

private:
    static bool debug;
    static bool show_time;

    static String time()
    {
        String result = "[";
        
        unsigned long milliseconds = millis();

        unsigned long seconds = milliseconds / 1000;
        milliseconds -= seconds * 1000;

        unsigned long minutes = seconds / 60;
        seconds -= minutes * 60;

        unsigned long hours = minutes / 60;
        minutes -= hours * 60;

        //

        if (hours < 10)
            result.concat("0");
        result.concat(hours);
        result.concat(":");

        if (minutes < 10)
            result.concat("0");
        result.concat(minutes);
        result.concat(":");

        if (seconds < 10)
            result.concat("0");
        result.concat(seconds);
        result.concat(":");

        if (milliseconds < 100)
            result.concat("0");
        else if (milliseconds < 10)
            result.concat("00");
        result.concat(milliseconds);

        result.concat("]");

        return result;
    }
};

bool Debug::debug = false;
bool Debug::show_time = false;
