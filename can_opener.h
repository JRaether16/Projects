#ifndef CAN_OPENER_H
#define CAN_OPENER_H

#include <stdint.h>

#define VERSION "0.4"
#define USE_OTA 0

/*
 * Switch numbers, switches are INDEXED
 */
#define SW_REDUCED 	0
#define SW_NORMAL 	1
#define SW_E_STOP	  2
#define SW_FORWARD	3
#define SW_REVERSE	4

/*
 * Pin numbers
 */
#define PIN_UP 15
#define PIN_NEXT 32
#define PIN_DOWN 14
#define PIN_BAT 35
#define PIN_EN 21

#define RTC_UP GPIO_NUM_15
#define RTC_NEXT GPIO_NUM_32
#define RTC_DOWN GPIO_NUM_14
#define RTC_BAT GPIO_NUM_35
#define RTC_EN GPIO_NUM_21

/*
 * Masks for buttons
 */
#define BTN_UP   0x01
#define BTN_NEXT 0x02
#define BTN_DOWN 0x04

/*
 * Menu indecies
 */
#define MENU_TOP_BAR    0
#define MENU_BOTTOM_BAR 1
#define MENU_LIVE_VIEW  2
#define MENU_SHUTDOWN   3


#define WIFI_NAME "RND"
#define WIFI_PASSWORD "v3rm33rrnd"
#define DEVICE_NAME "esp32_v0_3"
#define DEVICE_PASSWORD "VATH"

#define SCREEN_WIDTH  19
#define SCREEN_HEIGHT 17
#define SCREEN_PIXEL_WIDTH  240
#define SCREEN_PIXEL_HEIGHT 320

#define BAT_UPDATE_PERIOD 60000
#define TIMEOUT_PERIOD 300000
#define STANDBY_PERIOD 900000

#define VERMEER_YELLOW 0xFE24

// Avoids using delay because that messes with OTA
#define msleep(x)                 \
{                                 \
  long start = millis();          \
  while (millis() - start < x) {} \
}

/*
 * Variables
 */
// State names
enum State {STARTUP, MENU, STANDBY, BOTTOM_BAR, TOP_BAR_PREP, TOP_BAR, REPLACE_SW, DONE, LIVE_VIEW};

// One type for holding all application-relevent variables
typedef struct can_opener {
  State state, next_state; // Hold current and next state
  int faulty_switch;    // Which swich has been found to be faulty
  uint8_t prev_btns;  // Previous state of buttons
  uint8_t cur_btns;   // Current state of buttons
  uint8_t btns;       // Rising-edge sensitive version of buttons
  uint8_t sw_replaced;  // Bit-wise tracking of which switches have been replaced
  uint8_t sw_history;  // Bit-wise tracking of which switches have been on during a test
  uint8_t prev_sws;   // Previous state of switches
  uint8_t sws;        // Current state of switches
  bool new_state;   // True for one iteration after every state change
  const char* sw_names[5];    // Names of all switches
  const char* menu_items[4]; // Names of all menu items
  int menu_idx;     // Index of menu option to point to
  char prev_lines[SCREEN_HEIGHT][SCREEN_WIDTH + 1];   // Previous message (used for overwriting to erase)
  char lines[SCREEN_HEIGHT][SCREEN_WIDTH + 1];        // Current message to display
  int prev_battery_life;    // Previous battery life reading to avoid battery life increases caused by noise
  unsigned long prev_battery_update;      // Timestamp of last time the battery life was updated
  unsigned long prev_action;              // Timestamp of when the last action took place (used for timeout)
  bool timeout;     // Whether or not the device is in timeout mode
} can_opener_t;

/*
 * Function definitions in source file
 */
		
// Main functions
void co_init(can_opener_t* co_inst);
void co_run(can_opener_t* co_inst);

// State functions
void startup(can_opener_t* co_inst);
void menu(can_opener_t* co_inst);
void standby(can_opener_t* co_inst);
void bottom_bar(can_opener_t* co_inst);
void top_bar_prep(can_opener_t* co_inst);
void top_bar(can_opener_t* co_inst);
void replace_sw(can_opener_t* co_inst);
void done(can_opener_t* co_inst);
void live_view(can_opener_t* co_inst);


//Private functions
void co_OTA_init();
int get_battery_life();
bool check_can();
void read_btns(can_opener_t* co_inst);
void read_sws(can_opener_t* co_inst);
void co_timeout(can_opener_t* co_inst);
void co_shutdown(can_opener_t* co_inst);

#endif
