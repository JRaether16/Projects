// This project contains the software of version 0.4 for can_opener
#include "can_opener.h"

// Instance of a can_opener_t object to avoid other global variables
can_opener_t co_inst;

/*
 * Runs once at the beginning of the program
 */
void setup()
{
  co_init(&co_inst);
}

/*
 * Runs repeatedly indefinitely
 */
void loop()
{
  co_run(&co_inst);
}

