///////////////////////////////////////////////////////////////////////////////
// Serial port Menus
// SerialMenu - Copyright (c) 2019 Dan Truong
// See SerialMenu.hpp for details
///////////////////////////////////////////////////////////////////////////////
#define SERIALMENU_DISABLE_PROGMEM_SUPPORT true
#define SERIALMENU_MINIMAL_FOOTPRINT true
#include <SerialMenu.hpp>

// Instantiate the singleton menu instance. It is initialized when called
//SerialMenu SerialMenu::singleton;
const SerialMenu* SerialMenu::singleton = nullptr;
const SerialMenuEntry* SerialMenu::menu = nullptr;
uint16_t SerialMenu::waiting = uint16_t(0);
uint8_t SerialMenu::size = uint8_t(0);