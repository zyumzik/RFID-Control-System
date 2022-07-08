#include "Timer.h"

Timer::Timer()
{
}

void Timer::begin(uint32_t period)
{
	_timer = millis();
	_period = period;
}

bool Timer::update()
{
	if (millis() >= _timer + _period)
	{
		_timer = millis();
		return true;
	}
	return false;
}
