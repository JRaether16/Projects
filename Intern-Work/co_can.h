#ifndef CO_CAN_H
#define CO_CAN_H

#include <stdint.h>

#ifndef ARDUINO_ARCH_ESP32
#define ARDUINO_ARCH_ESP32
#endif


//CAN Message structure
class CanMessage
{
	/*
	 * Based on email from Ryan: Searching for [pf = 0xFF] and [ps = 0x79].
	 * pf = PDU Format, ps = PDU Specific
	 * Important bits and bytes are as follows (byte, bit, length): 
	 *(2,7,2)[Reduced] -> SW0
	 *(3,1,2)[Normal]  -> SW1
	 *(3,3,8)[Stop]    -> SW2
	 *(4,5,2)[Forward] -> SW3
	 *(5,1,2)[Reverse] -> SW4
	 */	
public:
	bool valid;
	uint32_t id;
	uint8_t data[8]; //Store data bytes
  CanMessage();

};

void co_can_init();
bool co_can_check_valid();
CanMessage co_can_read();

#endif
