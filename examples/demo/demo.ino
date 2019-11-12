///////////////////////////////////////////////////////////////////////////////
// Serial port Menus Demo
//
// Usage:
// - Compile and load this sketch onto your Arduino board.
// - Keep the USB cable connected while the board is running.
// - In the Arduino programming IDE, go in the "Tools" menu and click on the
//   "Serial Monitor" menu entry.
// A window should appear, and in it a menu will be displayed. Try to type
// the first character of one of the menu entries, followed by enter, in the
// window's text input field.
//
// If there is no clearly legible text shown in the window, set the speed to
// 9600 baud, and set the autoscroll checkbox. Reset the board or reload the
// program.
///////////////////////////////////////////////////////////////////////////////
#define DEMOCOPYRIGHT "SerialMenu demo - Copyright (c) 2019 Dan Truong"

#include <SerialMenu.hpp>

///////////////////////////////////////////////////////////////////////////////
// Main menu
///////////////////////////////////////////////////////////////////////////////

// Forward declarations for the sub-menu referenced before it is defined.
extern const SerialMenuEntry subMenu[];
extern const uint8_t subMenuSize;

// Define the main menu
const SerialMenuEntry mainMenu[] = {
  {"1 - Print 1",        false, '1', [](){ Serial.println("One"); } },
  {"M - Redisplay Menu", false, 'm', [](){ menu.show(); } },
  {"> - Sub-menu",       false, '>', [](){ menu.load(subMenu, subMenuSize); menu.show();} }
};
constexpr uint8_t mainMenuSize = GET_MENU_SIZE(mainMenu);


///////////////////////////////////////////////////////////////////////////////
// Sub-menu
///////////////////////////////////////////////////////////////////////////////

float value = 0;

// Example callback function
void foo() {
  Serial.print("Generic foo() function sees value is set to ");
  Serial.println(value);
}

// For this menu most entries are in PROGMEM Flash, and the last two in SRAM
// which is the normal default
const char subMenuStr0[] PROGMEM = "L - Left ear";
const char subMenuStr1[] PROGMEM = "R - Right ear";
const char subMenuStr2[] PROGMEM = "D - Down a value";
const char subMenuStr3[] PROGMEM = "N - Nuke a value";
const char subMenuStr4[] PROGMEM = "U - Up a value";
const char subMenuStr5[] PROGMEM = "B - Back";
const char subMenuStr6[] PROGMEM = "C - Center";
const char subMenuStr7[] PROGMEM = "F - Front";
const char subMenuStr8[] = "T - Twitch";
const char subMenuStr9[] = "I - Input a value";

// Define the sub-menu
// The last two menu entries declare their string directly
const SerialMenuEntry subMenu[] = {
  {subMenuStr0, true, 'l', foo },
  {subMenuStr1, true, 'r', foo},
  {subMenuStr2, true, 'd', [](){ Serial.println(--value); }},
  {subMenuStr3, true, 'n', [](){ value = 0; Serial.println(value); }},
  {subMenuStr4, true, 'u', [](){ Serial.println(++value); }},
  {subMenuStr5, true, 'b', foo},
  {subMenuStr6, true, 'c', foo},
  {subMenuStr7, true, 'f', foo},
  {subMenuStr8, false,'t', foo},
  {subMenuStr9, false,'I',
    [](){ value = menu.getNumber<float>("Input floating point: "); }},
  {"M - Menu",  false, 'm',
    [](){ menu.show(); } },
  {"< - Main",  false, '<',
    [](){ menu.load(mainMenu, mainMenuSize);
          menu.show(); } }
};
constexpr uint8_t subMenuSize = GET_MENU_SIZE(subMenu);

///////////////////////////////////////////////////////////////////////////////
// Main program
///////////////////////////////////////////////////////////////////////////////

void setup() {
  Serial.begin(9600);
  while (!Serial){};
  Serial.println(DEMOCOPYRIGHT);

  // Set the main menu as the current active menu
  menu.load(mainMenu, mainMenuSize);

  // Display the current menu
  menu.show();
}

constexpr uint8_t loopDelayMs = 100;

void loop() {
  // Run the current menu monitoring and callbacks
  menu.run(loopDelayMs);

  // put your main code here

  delay(loopDelayMs);
}
