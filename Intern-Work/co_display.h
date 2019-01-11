#ifndef CO_DISPLAY_H
#define CO_DISPLAY_H

#define TFT_CS   13
#define TFT_DC   33
#define TFT_MOSI 18
#define TFT_CLK  5
#define TFT_RST  27
#define TFT_MISO 19

void co_display_init();
void co_display_sleep();
void co_display_startup();
void co_display_header_bar();
void co_update_battery_life(int battery_life);
void co_display_menu(can_opener_t* co_inst);
void co_display_print(can_opener_t* co_inst);
void co_set_message(can_opener_t* co_inst, const char* msg);

#endif
