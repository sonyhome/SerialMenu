///////////////////////////////////////////////////////////////////////////////
// Serial port Menus
// SerialMenu - Copyright (c) 2019 Dan Truong
// See SerialMenu.hpp for details
///////////////////////////////////////////////////////////////////////////////
#include <SerialMenu.hpp>

// Instantiate the singleton menu instance. It is initialized when called
SerialMenu SerialMenu::singleton;

// Predeclare a single variable for the menus
SerialMenu& menu = SerialMenu::get();