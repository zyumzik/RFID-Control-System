#include <Arduino.h>

#define DFLT_MSG_VAL 0

// struct for exchanging data via EasyTransfer
struct Message
{
	uint_fast16_t	device_id = DFLT_MSG_VAL;	// reader device id
	uint32_t		card_id = DFLT_MSG_VAL;	    // read card id
	uint8_t			state_id = DFLT_MSG_VAL;	// message state
	uint8_t			other_id = DFLT_MSG_VAL;	// other information

	// set message data
	void set(uint_fast16_t device_id = DFLT_MSG_VAL,
			uint32_t card_id = DFLT_MSG_VAL,
			uint8_t state_id = DFLT_MSG_VAL,
			uint8_t other_id = DFLT_MSG_VAL)
	{
		this->device_id = device_id;
		this->card_id = card_id;
		this->state_id = state_id;
		this->other_id = other_id;
	}
	// clean message data
	void clean()
	{
		
	}
	// print message data
	void print()
	{
		Serial.println("message data:");
		Serial.println("device_id: " + String(device_id));
		Serial.println("card_id: " + String(card_id));
		Serial.println("state_id: " + String(state_id));
		Serial.println("other_id: " + String(other_id));
	}
};
