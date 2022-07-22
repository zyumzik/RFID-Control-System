#include <Arduino.h>

#define DFLT_MSG_VAL 0

// struct for exchanging data via EasyTransfer
struct Message
{
	unsigned short	device_id 	= DFLT_MSG_VAL;	// reader device id
	unsigned long	card_id 	= DFLT_MSG_VAL;	    // read card id
	unsigned short	state_id 	= DFLT_MSG_VAL;	// message state 
	unsigned short	other_id 	= DFLT_MSG_VAL;	// other information

	// set message data
	void set(unsigned short device_id = DFLT_MSG_VAL, unsigned long card_id = DFLT_MSG_VAL,
			unsigned short state_id = DFLT_MSG_VAL, unsigned short other_id = DFLT_MSG_VAL)
	{
		this->device_id = device_id;
		this->card_id 	= card_id;
		this->state_id 	= state_id;
		this->other_id 	= other_id;
	}
	
	// clean message data
	void clean()
	{
		set();
	}
};
