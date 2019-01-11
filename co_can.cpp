#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>

#include <CAN.h>
#include <CAN_config.h>
#include <unistd.h>
#include "co_can.h"


// CAN configuration object used by CAN driver
CAN_device_t CAN_cfg;

// CAN message object for passing data back to main program
CanMessage::CanMessage()
{
  this->valid = false;
  this->id = 0x00000000;
  for (int i = 0; i < 8; i++)
  {
    this->data[i] = 0x00;
  }
}

/*
 * Initialize CAN
 */
void co_can_init()
{
  CAN_cfg.speed = CAN_SPEED_250KBPS;
  //CAN_cfg.tx_pin_id = GPIO_NUM_12;
  //CAN_cfg.rx_pin_id = GPIO_NUM_27;
  CAN_cfg.tx_pin_id = GPIO_NUM_17;
  CAN_cfg.rx_pin_id = GPIO_NUM_16;
  
  // Create a queue for receiving CAN messages
  CAN_cfg.rx_queue = xQueueCreate(10,sizeof(CAN_frame_t));
  
  // Initialize CAN Module
  CAN_init();

  // For setting filters and masks see lines 226 - 233 in CAN.c from the ESP32-CAN-Driver library
}

/*
 * Checks if a valid message has been received. 
 * Returns true if it has, else false
 */
bool co_can_check_valid()
{	
  // Check for a message in the queue
  return (uxQueueMessagesWaiting(CAN_cfg.rx_queue) == 0) ? false : true;
}

/*
 * Reads a can message off the bus and returns a 
 * CanMessage object with data and validity
 */
CanMessage co_can_read()
{	
	CanMessage msg = CanMessage();
  CAN_frame_t rx_frame;
  
  // If invalid message, just return an empty CanMessage marked as invalid
	if(!co_can_check_valid()){
		msg.valid = false;
		return msg;
	}
	else
	{
    // Read in the new frame (returning an empty CanMessage marked as invalid)
    if(xQueueReceive(CAN_cfg.rx_queue, &rx_frame, 3*portTICK_PERIOD_MS) == pdFALSE)
    {
      msg.valid = false;
      return msg;
    }

    // Fill the CanMessage object and return it
    msg.id = rx_frame.MsgID;

    for(int i = 0; i < 8; i++){
      msg.data[i] = rx_frame.data.u8[i];
    }

    msg.valid = true;

    return msg;
	}	
}


