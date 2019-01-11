#include <unistd.h>
#include <stdio.h>
#include <Arduino.h>
#include <WiFi.h>
#include <ESPmDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
#include <driver/adc.h>
#include <driver/rtc_io.h>
#include "can_opener.h"
#include "co_can.h"
#include "co_display.h"
 
////////////////////////////////////////////////////////////
/*
 * Startup
 */
////////////////////////////////////////////////////////////
void co_init(can_opener_t* co_inst)
{
  Serial.begin(115200);
  while(!Serial);

  // Turn off bluetooth
  btStop();

  // Initialize OTA
  if (USE_OTA == 1)
  {
    co_OTA_init();
  }

  // Initialize co_inst
  co_inst->state = STARTUP; // Go to startup first
	co_inst->next_state = MENU; // Then go to menu
  memset(co_inst->prev_lines[0], 0, (SCREEN_WIDTH+1) * SCREEN_HEIGHT); //Init empty lines
  memset(co_inst->lines[0], 0, (SCREEN_WIDTH+1) * SCREEN_HEIGHT); //Init empty lines
  co_inst->new_state = true;
  co_inst->prev_btns = 0x00;
  co_inst->cur_btns = 0x00;
  co_inst->btns = 0x00;
  co_inst->sw_replaced = 0x00; // No switches checked yet or replaced
  co_inst->sw_history = 0x00;
  co_inst->prev_sws = 0x00;
  co_inst->sws = 0x00;
	co_inst->faulty_switch = -1; 
	co_inst->sw_names[SW_REDUCED] = "Reduced sensitivity";
	co_inst->sw_names[SW_NORMAL] = "Normal sensitivity";
	co_inst->sw_names[SW_E_STOP] = "Emergency stop";
	co_inst->sw_names[SW_FORWARD] = "Feed forward";
	co_inst->sw_names[SW_REVERSE] = "Feed reverse";
  co_inst->menu_items[MENU_LIVE_VIEW] = "Live Switch Status";
  co_inst->menu_items[MENU_BOTTOM_BAR] = "Test Bottom Bar";
  co_inst->menu_items[MENU_TOP_BAR] = "Test Top Bar";
  co_inst->menu_items[MENU_SHUTDOWN] = "Shutdown";
  co_inst->menu_idx = MENU_TOP_BAR;
  co_inst->prev_battery_life = 101;
  co_inst->prev_battery_update = 0L;
  co_inst->prev_action - 0L;
  co_inst->timeout = false;

  // Change the button pins' RTC statuses
  rtc_gpio_deinit(RTC_UP);
  rtc_gpio_deinit(RTC_NEXT);
  rtc_gpio_deinit(RTC_DOWN);

  // Set pin directions
  pinMode(PIN_UP, INPUT);
  pinMode(PIN_DOWN, INPUT);
  pinMode(PIN_NEXT, INPUT);
  pinMode(PIN_BAT, INPUT);
  pinMode(PIN_EN, OUTPUT);

  // Enable peripherals
  digitalWrite(PIN_EN, HIGH);

  // Initialize diplay
  co_display_init();

  // Initialize CAN
  co_can_init();
}

/*
 * Set up OTA (Over The Air) programming using the given parameters
 */
void co_OTA_init()
{
  // Set up OTA
  /* connect to wifi */
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_NAME, WIFI_PASSWORD);
  /* Wait for connection */
  while (WiFi.status() != WL_CONNECTED) {
      msleep(500);
      Serial.print(".");
  }
  
  /* create a connection at port 3232 */
  ArduinoOTA.setPort(3232);
  /* we use mDNS instead of IP of ESP32 directly */
  ArduinoOTA.setHostname(DEVICE_NAME);

  /* we set password for updating */
  ArduinoOTA.setPassword(DEVICE_PASSWORD);

  /* this callback function will be invoked when updating start */
  ArduinoOTA.onStart([]() {
    Serial.println("Start updating");
  });
  /* this callback function will be invoked when updating end */
  ArduinoOTA.onEnd([]() {
    Serial.println("\nEnd updating");
  });
  /* this callback function will be invoked when a number of chunks of software was flashed
  so we can use it to calculate the progress of flashing */
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  });

  /* this callback function will be invoked when updating error */
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
    else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
    else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
    else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
    else if (error == OTA_END_ERROR) Serial.println("End Failed");
  });
  /* start updating */
  ArduinoOTA.begin();
  Serial.print("ESP IP address: ");
  Serial.println(WiFi.localIP());
}


////////////////////////////////////////////////////////////
/*f
 * Main body of program called bay Arduino's loop() function
 */
////////////////////////////////////////////////////////////
void co_run(can_opener_t* co_inst)
{
  // Only do these things when in normal operation
  if(!co_inst->timeout)
  {
    // Handle any new OTA data
    if (USE_OTA == 1)
    {
      ArduinoOTA.handle();
    }

    // Update battery life if necessary
    int battery_life = get_battery_life();
    if (millis() - co_inst->prev_battery_update >= BAT_UPDATE_PERIOD && battery_life < co_inst->prev_battery_life)
    {
      co_update_battery_life(battery_life);
      co_inst->prev_battery_life = battery_life;
      co_inst->prev_battery_update = millis();
    }

    // Read machine switches (via CAN)
    read_sws(co_inst);
  }

  // Read buttons
	read_btns(co_inst);

  // Check to see if it's time to enter timeout mode
  if((!co_inst->timeout) && (millis() - co_inst->prev_action >= TIMEOUT_PERIOD))
  {
    co_inst->new_state = true;
    co_inst->timeout = true;
  }

  // Check to see if it's time to auto-shutdown
  if(co_inst->timeout)
  {
    if(millis() - co_inst->prev_action >= STANDBY_PERIOD)
    {
      co_shutdown(co_inst);
    }
    else{
      co_timeout(co_inst);
      return;
    }
  }
  
	// Call the appropriate function based on the current state and set new_state accordingly
	switch(co_inst->state)
	{
  case STARTUP:
    startup(co_inst);
    co_inst->new_state = (co_inst->state == STARTUP) ? false : true;
    break;
  case MENU:
    menu(co_inst);
    co_inst->new_state = (co_inst->state == MENU) ? false : true;
    break;
	case STANDBY:
		standby(co_inst);
		co_inst->new_state = (co_inst->state == STANDBY) ? false : true;
		break;
	case BOTTOM_BAR:
		bottom_bar(co_inst);
		co_inst->new_state = (co_inst->state == BOTTOM_BAR) ? false : true;
		break;
	case TOP_BAR_PREP:
		top_bar_prep(co_inst);
		co_inst->new_state = (co_inst->state == TOP_BAR_PREP) ? false : true;
		break;
	case TOP_BAR:
		top_bar(co_inst);
		co_inst->new_state = (co_inst->state == TOP_BAR) ? false : true;
		break;
	case REPLACE_SW:
		replace_sw(co_inst);
		co_inst->new_state = (co_inst->state == REPLACE_SW) ? false : true;
		break;
	case DONE:
    done(co_inst);
    co_inst->new_state = (co_inst->state == DONE) ? false : true;
		break;
  case LIVE_VIEW:
    live_view(co_inst);
    co_inst->new_state = (co_inst->state == LIVE_VIEW) ? false : true;
    break;
	default:
		break;
	}
}


////////////////////////////////////////////////////////////
/*
 * Functions to implement our states
 */
////////////////////////////////////////////////////////////


////////////////////////////////////////////////////////////
/*
 * Startup state - Displays splash screen
 */
////////////////////////////////////////////////////////////
void startup(can_opener_t* co_inst)
{
    // Display splash screen for 3 seconds
    co_display_startup();
    msleep(3000);

    // Go to menu next
    co_inst->state = MENU;
}


////////////////////////////////////////////////////////////
/*
 * Menu state - Displays the menu and handles navigation
 */
////////////////////////////////////////////////////////////
void menu(can_opener_t* co_inst)
{
  // Keep track of if anything has changed
  bool refresh_screen = false;
  
  // Initialize state (only runs the first time)
  if (co_inst->new_state)
  {
    // Draw header bar (with battery reading)
    co_display_header_bar();
    int battery_life = get_battery_life();
    co_update_battery_life(battery_life);
    co_inst->prev_battery_life = battery_life;
    co_inst->prev_battery_update = millis();
    co_inst->prev_action = millis();
    refresh_screen = true;
  }

  // Handle button presses
  if (((co_inst->btns & BTN_UP) == BTN_UP) && (co_inst->menu_idx > 0))
  {
    // Move cursor up
    co_inst->menu_idx--;
    refresh_screen = true;
  }
  if (((co_inst->btns & BTN_DOWN) == BTN_DOWN) && (co_inst->menu_idx < MENU_SHUTDOWN))
  {
    // Move cursor down
    co_inst->menu_idx++;
    refresh_screen = true;
  }
  if ((co_inst->btns & BTN_NEXT) == BTN_NEXT)
  {
    // Clear the screen by drawing header bar (with battery reading)
    co_display_header_bar();
    int battery_life = get_battery_life();
    co_update_battery_life(battery_life);
    co_inst->prev_battery_life = battery_life;
    
    // Select menu item
    switch (co_inst->menu_idx)
    {
    case MENU_LIVE_VIEW:
      co_inst->state = STANDBY;
      co_inst->next_state = LIVE_VIEW;
      break;
    case MENU_BOTTOM_BAR:
      co_inst->state = STANDBY;
      co_inst->next_state = BOTTOM_BAR;
      break;
    case MENU_TOP_BAR:
      co_inst->state = STANDBY;
      co_inst->next_state = TOP_BAR_PREP;
      break;
    case MENU_SHUTDOWN:
      co_inst->state = DONE;
      break;
    default:
      break;
    }
  }

  // Refresh screen if necessary
  if (refresh_screen)
  {
    co_display_menu(co_inst);
  }
}


////////////////////////////////////////////////////////////
/*
 * Standby state - Waits for valid CAN messages
 */
////////////////////////////////////////////////////////////
void standby(can_opener_t* co_inst)
{
  // Check CAN several times before displaying prompt to plug in
  // This is in an attempt to avoid showing prompt when already plugged in
  for (int i = 0; i < 128000; i++)
  {
    // Wait for CAN message validation
    if (check_can()) 
    {
      co_inst->state = co_inst->next_state;
      return;
    }
  }
  
  // Initialize state (only runs the first time)
  if (co_inst->new_state)
  {
	  co_set_message(co_inst, "Plug into the diagnostics port of your BC1000 or press next to return to the menu.");
	  co_display_print(co_inst);
  }

  // Check for NEXT button being pressed
  if (co_inst->btns & BTN_NEXT)
  {
    // Return to menu
    co_inst->state = MENU;
  }
}


////////////////////////////////////////////////////////////
/*
 * Diagnose switch failures on bottom feed safety bar
 */
////////////////////////////////////////////////////////////
void bottom_bar(can_opener_t* co_inst)
{
	// Initialize state (only runs the first time)
	if(co_inst->new_state)
	{
		co_set_message(co_inst, "Push the bottom feed safety bar completely in and out once. Press next when completed.");
		co_display_print(co_inst);

    // Reset switches and history (in other words, start a new diagnostic process)
		co_inst->sws = 0x00;
		co_inst->sw_history = 0x00;
	}	

  // Track the history of each switch
  // 0 = Was never tuned on
  // 1 = Was turned on at some point
  co_inst->sw_history |= co_inst->sws;

  // Check for NEXT button being pressed
	if (co_inst->btns & BTN_NEXT)
	{
    // For normal functionality, all switches should currently be off
    // and should have turned on at some point
		if (((co_inst->sw_history & 0x03) == 0x03) && ((co_inst->sws & 0x03) == 0x00))
		{
      // Normal functionality
			co_set_message(co_inst, "Bottom feed safety bar functioning normally.");
			co_display_print(co_inst);
			msleep(3000);
			co_inst->state = MENU;
		}
		else
		{
      // Find which switch was faulty
			if (((co_inst->sw_history & 0x01) == 0x00) || ((co_inst->sws & 0x01) == 0x01))
			{
				co_inst->state = REPLACE_SW;
				co_inst->faulty_switch = SW_REDUCED;
			}
			else if (((co_inst->sw_history & 0x02) == 0x00) || ((co_inst->sws & 0x02) == 0x02))
			{
				co_inst->state = REPLACE_SW;
				co_inst->faulty_switch = SW_NORMAL;
			}
			else {
				co_inst->state = REPLACE_SW;
				co_inst->faulty_switch = -1;
			}
		}
	}
}


////////////////////////////////////////////////////////////
/*
 * Prep the user for the top bar test by putting the bar
 * into the FORWARD position
 */
////////////////////////////////////////////////////////////
void top_bar_prep(can_opener_t* co_inst)
{
  // Initialize state (only happens the first time)
	if (co_inst->new_state) {
		co_set_message(co_inst, "Put the upper feed control bar into the FORWARD position. Press next when completed.");
		co_display_print(co_inst);
	}

  // Check for NEXT button being pressed
	if (co_inst->btns & BTN_NEXT) {
    // Move on to diagnosing the top bar
		co_inst->state = TOP_BAR;
	}
}


////////////////////////////////////////////////////////////
/*
 * Diagnose switch fauilures on upper feed control bar
 */
////////////////////////////////////////////////////////////
void top_bar(can_opener_t* co_inst)
{
  // Initialize state (only runs the first time)
	if (co_inst->new_state) {
		co_set_message(co_inst, "Slowly move the upper feed control bar to E-STOP, then to REVERSE, then allow it to rest in NEUTRAL. Press next when completed.");
		co_display_print(co_inst);

    // Reset switches and history (in other words, start a new diagnostic process)
		co_inst->sws = 0x00;
		co_inst->sw_history = 0x00;
	}
 
	// Track the history of each switch
  // 0 = Was never tuned on
  // 1 = Was turned on at some point
  co_inst->sw_history |= co_inst->sws;

  // Check for NEXT button being pressed
	if (co_inst->btns & BTN_NEXT)
	{
    // For normal functionality, all switches should currently be off
    // and should have turned on at some point
		if (((co_inst->sw_history & 0x1C) == 0x1C) && ((co_inst->sws & 0x01C) == 0x00))
		{
			co_set_message(co_inst, "Upper feed control bar functioning normally.");
			co_display_print(co_inst);
			msleep(3000);
			co_inst->state = MENU;
		}
		else
		{
      // Find which switch was faulty
			if (((co_inst->sw_history & 0x04) == 0x00) || ((co_inst->sws & 0x04) == 0x04))
			{
				co_inst->state = REPLACE_SW;
				co_inst->faulty_switch = SW_E_STOP;
			}
			else if (((co_inst->sw_history & 0x08) == 0x00) || ((co_inst->sws & 0x08) == 0x08))
			{
				co_inst->state = REPLACE_SW;
				co_inst->faulty_switch = SW_FORWARD;
			}
			else if (((co_inst->sw_history & 0x10) == 0x00) || ((co_inst->sws & 0x10) == 0x10))
			{
				co_inst->state = REPLACE_SW;
				co_inst->faulty_switch = SW_REVERSE;
			}
			else {
				co_inst->state = REPLACE_SW;
				co_inst->faulty_switch = -1;
			}
		}
	}
}


////////////////////////////////////////////////////////////
/*
 * Instructs user to replace any faulty switches
 */
////////////////////////////////////////////////////////////
void replace_sw(can_opener_t* co_inst)
{
  // Safety check, should never happen
	if (co_inst->faulty_switch == -1)
	{
		// No switches are faulty
		co_set_message(co_inst, "Error: No faulty switch");
		co_display_print(co_inst);
		msleep(3000);
		co_inst->state = MENU;
	}
	else if ((co_inst->sw_replaced & (0x01 << co_inst->faulty_switch)) == 0x00)
	{
    // This is the first time this switch has been visited
   
    // Initialize state (only runs the first time)
    if (co_inst->new_state)
    {
      char str[200];
      sprintf(str, "%s switch seems faulty. Replace it, then press next to return to the menu.", co_inst->sw_names[co_inst->faulty_switch]); 
      co_set_message(co_inst, str);
      co_display_print(co_inst);
    }

    // Check for NEXT button being pressed
    if (co_inst->btns & BTN_NEXT)
    {
      // Remember that this switch has been replaces
      co_inst->sw_replaced |= 0x01 << co_inst->faulty_switch;
      co_inst->faulty_switch = -1;

      // Return to menu
      co_inst->state = MENU;
    }
	}
	else
	{
   // This switch has already been replaced
   
    // Initialize state (only runs the first time)
    if (co_inst->new_state)
    {
      char str[200];
      sprintf(str, "%s switch still seems faulty. Please call your Vermeer dealer for further assistance. Press next to return to menu.", co_inst->sw_names[co_inst->faulty_switch]); 
      co_set_message(co_inst, str);
      co_display_print(co_inst);
    }
    
    // Check for NEXT button being pressed
    if (co_inst->btns & BTN_NEXT)
    {
      // Return to menu
      co_inst->state = MENU;
    }
	}
}


////////////////////////////////////////////////////////////
/*
 * Done state - Puts the device into sleep mode
 */
////////////////////////////////////////////////////////////
void done(can_opener_t* co_inst)
{
  // Display message for 3 seconds
  co_set_message(co_inst, "Powering off device.");
  co_display_print(co_inst);
  msleep(3000);

  // Shut down the device
  co_shutdown(co_inst);

}

////////////////////////////////////////////////////////////
/*
 * Live View state - Shows real-time status of switches
 */
////////////////////////////////////////////////////////////
void live_view(can_opener_t* co_inst)
{
  // Keep track of if anything has changed
  bool refresh_screen = false;
  
  // Initialize state (only runs the first time)
  if (co_inst->new_state)
  {
    refresh_screen = true;
  }

  // Check for a change in switch status
  if (co_inst->sws != co_inst->prev_sws)
  {
    refresh_screen = true;
  }

  // Update the display if necessary
  if (refresh_screen)
  {
    char on_off[5][4];
    for (int i = 0; i < 5; i++)
    {
      if ((co_inst->sws & (0x01 << i)))
      {
        sprintf(on_off[i], "ON");
      }
      else
      {
        sprintf(on_off[i], "OFF");
      }
    }
      
  
    char str[200];
    sprintf(str, "Reduced %s\nNormal  %s\nE-Stop  %s\nForward %s\nReverse %s\n\n\n\nPress next to return to menu.", on_off[0], on_off[1], on_off[2], on_off[3], on_off[4]);
    co_set_message(co_inst, str);
    co_display_print(co_inst);
  }

  // Check for NEXT button being pressed
  if ((co_inst->btns & BTN_NEXT) == BTN_NEXT)
  {
    // Return to menu
    co_inst->state = MENU;
  }
}




////////////////////////////////////////////////////////////
/*
 * Helper functions
 */
////////////////////////////////////////////////////////////


////////////////////////////////////////////////////////////
/*
 * Determines battery life
 */
////////////////////////////////////////////////////////////
int get_battery_life()
{
  float bat_level = (float) analogRead(PIN_BAT);

  // Convert from a 12-bit integer to a percentage
  // 1744.0 and 721.0 were calculated to make 0% be at 3.3V and 100% be at 4.5V
  float percentage = ((bat_level - 1744.0) / 721.0) * 100.0;

  // Make sure the percentage is between 0% and 100%
  if (percentage < 0.0)
  {
    percentage = 0.0;
  }

  if (percentage > 100.0)
  {
    percentage = 100.0;
  }
  
	return (int) percentage;
}


////////////////////////////////////////////////////////////
/*
 * Confirm whether port and data is valid
 */
////////////////////////////////////////////////////////////
bool check_can()
{
	return co_can_check_valid();
}

////////////////////////////////////////////////////////////
/*
 *  Read user input buttons
 */
////////////////////////////////////////////////////////////
void read_btns(can_opener_t* co_inst)
{
	// Copy co_inst->cur_btns to co_inst->prev_btns
	co_inst->prev_btns = co_inst->cur_btns;
  co_inst->cur_btns = 0x00;
  co_inst->btns = 0x00;

  // Read in new values for co_inst->cur_btns
  if (digitalRead(PIN_UP) == HIGH)
  {
    co_inst->cur_btns |= BTN_UP;

    // Check for rising edge
    if ((co_inst->prev_btns & BTN_UP) == 0x00)
    {
      co_inst->btns |= BTN_UP;
      
      // Reset timeout counter
      co_inst->prev_action = millis();
    }
  }
  if (digitalRead(PIN_NEXT) == HIGH)
  {
    co_inst->cur_btns |= BTN_NEXT;

    // Check for rising edge
    if ((co_inst->prev_btns & BTN_NEXT) == 0x00)
    {
      co_inst->btns |= BTN_NEXT;
      
      // Reset timeout counter
      co_inst->prev_action = millis();
    }
  }
  if (digitalRead(PIN_DOWN) == HIGH)
  {
    co_inst->cur_btns |= BTN_DOWN;

    // Check for rising edge
    if ((co_inst->prev_btns & BTN_DOWN) == 0x00)
    {
      co_inst->btns |= BTN_DOWN;
      
      // Reset timeout counter
      co_inst->prev_action = millis();
    }
  }
  
  // Debouncing
	if (co_inst->cur_btns != co_inst->prev_btns)
	{
		msleep(10);
	}
}


////////////////////////////////////////////////////////////
/*
 * Read incoming CAN data to figure out switch status
 */
////////////////////////////////////////////////////////////
void read_sws(can_opener_t* co_inst)
{
  // Object for storing a new CAN message
	CanMessage msg = co_can_read();

  // Only use message if valid
	if (msg.valid)
  {
    // Copy co_inst->sws into co_inst->prev_sws, then read in new switch data
    co_inst->prev_sws = co_inst->sws;
    co_inst->sws = 0x00;
	  if ((~msg.data[1]) & 0x40)
	  {
		  co_inst->sws |= (0x01 << SW_REDUCED);
	  }
  	if ((~msg.data[2]) & 0x01)
  	{
  		co_inst->sws |= (0x01 << SW_NORMAL);
  	}
  	if ((~msg.data[2]) & 0x04)
  	{
  		co_inst->sws |= (0x01 << SW_E_STOP);
  	}
  	if (msg.data[3] & 0x10)
  	{
  		co_inst->sws |= (0x01 << SW_FORWARD);
  	}
  	if (msg.data[4] & 0x01)
  	{
  		co_inst->sws |= (0x01 << SW_REVERSE);
  	}
  }
}

////////////////////////////////////////////////////////////
/*
 * Puts the device into timeout mode, then checks to see
 * when to wake up
 */
////////////////////////////////////////////////////////////
void co_timeout(can_opener_t* co_inst)
{
  // Initialize state (only runs the first time)
  if(co_inst->new_state)
  {
    // Turn off WiFi
    WiFi.mode(WIFI_OFF);

    // Put the display to sleep
    co_display_sleep();

    // Shut off peripherals (display, CAN transceiver, and battery level voltage divider)
    digitalWrite(PIN_EN, LOW);
  
    msleep(100); // Wait 0.1 seconds for graceful shutdown

    co_inst->new_state = false;
  }  

  // Check for a button press, indicating wake up
  if (co_inst->btns & (BTN_NEXT | BTN_UP | BTN_DOWN))
  {
    co_inst->new_state = true;
    co_inst->timeout = false;

    // Enable preipherals
    digitalWrite(PIN_EN, HIGH);

    // Initialize OTA
    if (USE_OTA == 1)
    {
      co_OTA_init();
    }

    // Initialize display
    co_display_init();

    // Initialize CAN
    co_can_init();

    // Display header bar (with battery life)
    co_display_header_bar();
    int battery_life = get_battery_life();
    co_update_battery_life(battery_life);
    co_inst->prev_action = millis();
  }
  
}

////////////////////////////////////////////////////////////
/*
 * Handle shutdown
 */
////////////////////////////////////////////////////////////
void co_shutdown(can_opener_t* co_inst)
{
  // Turn off WiFi
  WiFi.mode(WIFI_OFF);

  // Put the display to sleep
	co_display_sleep();

  // Shut off peripherals (display, CAN transceiver, and battery level voltage divider)
  digitalWrite(PIN_EN, LOW);
  
  msleep(100); // Wait 0.1 seconds for graceful shutdown

  // Configure navigation buttons to trigger wake up
  uint64_t mask = ((uint64_t)1 << 14) | ((uint64_t)1 << 15) | ((uint64_t)1 << 32);
  esp_sleep_enable_ext1_wakeup(mask, ESP_EXT1_WAKEUP_ANY_HIGH);

  // Put ESP32 into deep sleep
  esp_deep_sleep_start();
}
