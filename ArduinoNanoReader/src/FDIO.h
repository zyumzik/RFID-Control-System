#ifndef FDIO_H
#define FDIO_H

#include <Arduino.h>

// DANGEROUS FUNCTION!!!
inline void dWrite(uint8_t pin, uint8_t val)
{
    uint8_t bit = digitalPinToBitMask(pin);
    uint8_t port = digitalPinToPort(pin);
    volatile uint8_t* out = portOutputRegister(port);
	if (val == LOW)
		*out &= ~bit;
    else
		*out |= bit;
}

// DANGEROUS FUNCTION
inline bool dRead(uint8_t pin)
{
    uint8_t bit = digitalPinToBitMask(pin);
	uint8_t port = digitalPinToPort(pin);
    if (*portInputRegister(port) & bit) return 1;
	return 0;
}

// DANGEROUS FUNCTION
inline void pMode(uint8_t pin, uint8_t mode)
{
    uint8_t bit = digitalPinToBitMask(pin);
	uint8_t port = digitalPinToPort(pin);
	volatile uint8_t *reg = portModeRegister(port);
    volatile uint8_t *out = portOutputRegister(port);

	if (mode == INPUT) 
    {
		*reg &= ~bit;
		*out &= ~bit;
	} 
    else if (mode == INPUT_PULLUP) 
    {
		*reg &= ~bit;
		*out |= bit;
	} 
    else 
    {
		*reg |= bit;
	}
}

#endif