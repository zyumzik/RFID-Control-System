#include "Timer.h"

Timer::Timer()
{
}

void Timer::begin(uint32_t period)
{
	_alive = true;
	_timer = millis();
	_period = period;
}

bool Timer::update()
{
	if (millis() >= _timer + _period
		&& _alive)
	{
		_timer = millis();
		return true;
	}
	return false;
}

void Timer::stop()
{
	_alive = false;
}
