#include <Arduino.h>

class Timer
{
public:
	// begin timer count
	void begin(uint32_t period)
	{
		_alive = true;
		_timer = millis();
		_period = period;
	}

	// returns true if timer is ready (should do it's work)
	bool update()
	{
		if (millis() >= _timer + _period
			&& _alive)
		{
			_timer = millis();
			return true;
		}
		return false;
	}

	// stop timer (update always return false)
	void stop()
	{
		_alive = false;
	}
	
private:
	bool 	 _alive = false;	
	uint32_t _timer = 0;
	uint32_t _period = 0;
};
