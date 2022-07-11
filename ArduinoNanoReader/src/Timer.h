#include <Arduino.h>

class Timer
{
public:
	Timer();

	void begin(uint32_t period);

	// returns true if timer is ready (should do it's work)
	bool update(); 

	void stop();
private:
	bool 	 _alive = false;	
	uint32_t _timer = 0;
	uint32_t _period = 0;
};
