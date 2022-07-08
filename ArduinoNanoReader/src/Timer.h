#include <Arduino.h>

class Timer
{
public:
	Timer();

	void begin(uint32_t period);

	// returns true if timer is ready (should do it's work)
	bool update(); 
private:
	uint32_t _timer = 0;
	uint32_t _period = 0;
};
