#include <stdint.h>
#include <unistd.h>
#include <Arduino.h>
#include "Adafruit_GFX.h"
#include "Adafruit_ILI9341.h"
#include "can_opener.h"
#include "co_display.h"
#include "co_graphics.h"

// TFT object
Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC, TFT_RST);

/*
 * This function initializes all things necessary for using the TFT (Thin Flexible Transistor) LCD display
 */
void co_display_init()
{
  // Start the TFT
  tft.begin(); // Optional to include spi frequency

  // Set screen rotation
  uint8_t rotation = 0; //Portrait mode 0 or 2, landscape 1 or 3
  tft.setRotation(rotation);
}

/*
 * This function sends a command telling the display to turn off
 * Shouldn't be necessary since power to the display is being cut off anyway
 */
void co_display_sleep()
{
  tft.startWrite();

  // Turn off display
  tft.writeCommand(ILI9341_DISPOFF);
  msleep(120);
  Serial.println("Display turned off");

  // Enter sleep mode
  tft.writeCommand(ILI9341_SLPIN);
  msleep(120);
  Serial.println("Sleep mode entered");
  
  tft.endWrite();
}

/* 
 * Displays the splash screen for 3 soconds
 */
void co_display_startup()
{
  // Make screen black
  tft.fillScreen(ILI9341_BLACK);
  yield();

  // Draw "EQUIPPED TO DO MORE" image
  tft.drawRGBBitmap(52, 134, (uint16_t*)(CO_Startup.pixel_data),CO_Startup.width, CO_Startup.height);
  tft.setFont(); //sets font to default Adafruit font

  // Font formatting
  tft.setFont();
  tft.setTextColor(ILI9341_WHITE);
  tft.setTextSize(1);

  // Determine position for the version label text
  int16_t x1, y1;
  uint16_t w, h;
  char str[100];
  sprintf(str, "Brush Chipper Diagnostics v%s", VERSION);
  tft.getTextBounds(str, 0, 0, &x1, &y1, &w, &h);

  // Display version label text
  tft.setCursor(SCREEN_PIXEL_WIDTH/2 - w/2, 203);
  tft.println("Brush Chipper Diagnostics v0.4");
}

/* 
 * Displays the header bar
 */
void co_display_header_bar()
{
  // Make the screen white
  tft.fillScreen(ILI9341_WHITE);
  yield();

  // Draw the header bar image
  tft.drawRGBBitmap(0, 0, (uint16_t*)(CO_HeaderBarLE.pixel_data),CO_HeaderBarLE.width, CO_HeaderBarLE.height);
}

/*
 * Draws the given number as a percentage in the battery icon as a percentage
 */
void co_update_battery_life(int battery_life)
{
  // Draw the header bar image
  tft.drawRGBBitmap(0, 0, (uint16_t*)(CO_HeaderBarLE.pixel_data),CO_HeaderBarLE.width, CO_HeaderBarLE.height);
  
  // Font formatting
  tft.setFont(); //sets font to default Adafruit font
  tft.setTextColor(ILI9341_WHITE);
  tft.setTextSize(1);

  // Determine position for battery life
  char str[10];
  sprintf(str, "%d%%", battery_life);
  int16_t x, y;
  uint16_t w, h;
  tft.getTextBounds(str, 0, 0, &x, &y, &w, &h);

  // Display battery life in middle of battery icon
  tft.setCursor(218 - w/2, 12);
  tft.println(str);
}

/*
 * Prints the menu to the screen, hilighting the selected item
 */
void co_display_menu(can_opener_t* co_inst)
{
  // Extra amount of padding around text
  int pad = 3;

  // Number of pixels per line (including text and gap between lines)
  int line_height = 25;
  
  // Use a white background for all non-selected items, and Vermeer yellow for the selected item
  uint16_t bg_colors[] = {0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF};
  bg_colors[co_inst->menu_idx] = VERMEER_YELLOW;

  // Variables for getting the bounds of text
  int16_t x, y;
  uint16_t w, h;

  // Variables for tracking the cursor
  int cur_x, cur_y, i;

  // Font formatting
  tft.setFont();
  tft.setTextSize(2);
  tft.setTextColor(ILI9341_BLACK); // Black text
  char str[40];

  // Draw background, then text for each line
  for (cur_x = 10, cur_y = 50, i = 0; i < 4; cur_y += line_height, i++)
  {
    sprintf(str, "%s", co_inst->menu_items[i]);
    tft.getTextBounds(str, cur_x, cur_y, &x, &y, &w, &h);
    tft.fillRect(x - pad, y - pad, w + 2*pad, h + 2*pad, bg_colors[i]);
    tft.setCursor(cur_x, cur_y);
    tft.println(str);
  }
  
}

/*
 * Prints the message stored in co_inst->lines to the display
 */
void co_display_print(can_opener_t* co_inst)
{
  // Number of pixels per line (including text and gap between lines)
  int line_height = 16;

  // Font formatting
  tft.setFont();
  tft.setTextSize(2);
  tft.setTextColor(ILI9341_WHITE);
  
  // Clear screen by writing over previous message in white
  for(int i = 0; i < SCREEN_HEIGHT; i++)
  {
      tft.setCursor(1, i*line_height + 50);
      tft.println(co_inst->prev_lines[i]);
  }

  // Write new message in black
  tft.setTextColor(ILI9341_BLACK); // Black text
  for(int i = 0; i < SCREEN_HEIGHT; i++)
  {
      tft.setCursor(1, i*line_height + 50);
      tft.println(co_inst->lines[i]);
  }

  // Copy new message into prev_lines
  memcpy(co_inst->prev_lines[0], co_inst->lines[0], (SCREEN_WIDTH+1) * SCREEN_HEIGHT);
}



/*
 * Sets co_inst->lines to contain the given message
 * This is important for dealing with text wrapping
 */
void co_set_message(can_opener_t* co_inst, const char* msg)
{
  int offset = 0;   // Used to avoid incrementing a const char*
  int i = 0;        // Current line
  int j = 0;        // Position in current line
  int j_space = 0;  // Position of the most recent space in the current line (used for overwriting the word with spaces if the word gets too long to fit on the current line)
  int j_next = 0;   // Position in next line (used for writing the current word to the next line in case the word ends up being too long to fit on the current line)
  int width = SCREEN_WIDTH;     // Number of characters that can fit on a single line
  int height = SCREEN_HEIGHT;   // Number of lines that can fit on a single screen
  
  // Fill all of the lines containing text
  while (msg[offset] != '\0' && i < height)
  {
    // If the character is a new line, move to the next line
    if (msg[offset] == '\n')
    {
      // Fill the rest of the current line with spaces
      for (; j < width; j++)
      {
        co_inst->lines[i][j] = ' ';
      }
      
      // Null-terminate the line
      co_inst->lines[i][width] = '\0';

      // Move to next line
      i++;
      j = 0;
      j_next = 0;
      j_space = 0;

      // Move to next character
      offset++; 
    }
    
    // If the character is a space AND we're at the end of the line, just move on to the next line
    else if (msg[offset] == ' ' && j == width)
    {
      // Null-terminate the line
      co_inst->lines[i][width] = '\0';

      // Move to next line
      i++;
      j = 0;
      j_next = 0;
      j_space = 0;

      // Move to next character
      offset++;
    }
    
    // If we have ran out of room on this line, overwrite the current word with spaces and move to the next line
    else if (j == width)
    {
      // Overwrite the current word with spaces
      for (; j_space < width; j_space++)
      {
        co_inst->lines[i][j_space] = ' ';
      }

      // Null-terminate the line
      co_inst->lines[i][width] = '\0';

      // Move to the next line
      i++;
      j = j_next;
      j_next = 0;
      j_space = 0;

      // Write the current character to this line, and to the next line in case this word ends up being too big to fit on this line
      co_inst->lines[i][j] = msg[offset];
      co_inst->lines[i + 1][j_next] = msg[offset];

      // Move to next character
      j++;
      j_next++;
      offset++;
    }

    // If we are in the middle of a line:
    else
    {
      // Write the current character to the current line
      co_inst->lines[i][j] = msg[offset];

      // If the character is a space, we've reached the end of a word
      if (msg[offset] == ' ')
      {
        // Move j_space
        j_space = j;

        // Reset the position on the next line since the word did fit on the current line
        j_next = 0;

        // Move to next character
        j++;
        offset++;
      }
      else {
        // Write the current character to the next line as well in case this word ends up being too long to fit on the current line
        co_inst->lines[i + 1][j_next] = msg[offset];

        // Move to next character
        j++;
        j_next++;
        offset++;
      }
    }
  }
  
  // Fill the rest of the current line with spaces  
  for (; j < width; j++)
  {
    co_inst->lines[i][j] = ' ';
  }

  // Null-terminate the line
  co_inst->lines[i][width] = '\0';
  
  // Fill the remaining lines with spaces
  i++;
  for (; i < height; i++)
  {
    for (j = 0; j < width; j++)
    {
      co_inst->lines[i][j] = ' ';
    }

    // Null-terminate the line
    co_inst->lines[i][width] = '\0';
  }
}

