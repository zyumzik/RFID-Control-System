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
		set();
	}
	
	// print message data
	void print()
	{
		Serial.println("message:\n{");
		Serial.print("\tdevice_id: ");
		Serial.println(device_id);
		Serial.print("\tcard_id:   ");
		Serial.println(card_id);
		Serial.print("\tstate_id:  ");
		Serial.println(state_id);
		Serial.print("\tother_id:  ");
		Serial.println(other_id);
		Serial.println("}");
	}
};
