# SerialMenu
Serial Console Menu Library for Arduino

## Overview
This library allows you to define menus for the Arduino Serial console.
Menus are very simple to build, by just declaring them in an array.
A single show() function call displays them, and a single call to the
run() function from loop() is enough to automatically monitor the menu
and execute the callback functions associated to the menu entries.

The menus are non-blocking, but only trigger when there is user input.
This means run() returns immediately if there's no activity, or just
after calling the callback routine if there was input. This allows
programs to simultaneously go about the work you want them to do while
responding to serial commands promptly but with almost no overhead.

Furthermore because of how the menu library is implemented, they consume
very little memory.

## Benefits:

* Simple to declare a menu as a global variable
* One call to display the menu
* Easily support sub-menu hierarchies
* Compact user code
* Menu text can be stored in SRAM or Flash (data or program memory)

## Memory overhead analysis:

To analyze the efficiency of this library, it was added to one of the existing examples shipped with the Arduino IDE, and we compared the memory footprint overhead.

This analysis is for the following configuration:
* Example program: **07.Display -> barGraph**
* Platform: Arduino UNO
* SerialMenu version 1.0 released Nov. 12 2019.

**Overhead Results:**
* Low **5 Bytes of SRAM** data memory overhead per menu line entry with strings stored in FLASH memory.
* Fixed **43 Bytes of SRAM** base cost for the library (of which 12B for the singleton).
* Fixed **1150 Bytes of FLASH** base cost for the library's code.

### Original code:
```
Sketch uses 1086 bytes (3%) of program storage space. Maximum is 32256 bytes.
Global variables use 29 bytes (1%) of dynamic memory, leaving 2019 bytes for local variables. Maximum is 2048 bytes.
```
### Original code using Serial:
```C++
void setup() {
  Serial.begin(9600);
  while (!Serial){};
  Serial.print("hello ");
  Serial.println("world");
  ...}
```
```
Sketch uses 2120 bytes (6%) of program storage space. Maximum is 32256 bytes.
Global variables use 220 bytes (10%) of dynamic memory, leaving 1828 bytes for local variables. Maximum is 2048 bytes.
```
Note:
We'll leave this extra code in setup(), so as to not bias the cost of using the SerialMenu library. It will be our reference.
The intent is to write to the Serial console so this overhead (1034B code, 191B data) is unavoidable unless we rewrite the default HardwareSerial library. The console output is:
```
hello world
```

### Adding SerialMenu code:
```C++
#include <SerialMenu.hpp>
SerialMenu& menu = SerialMenu::get();
```
```
Sketch uses 2934 bytes (9%) of program storage space. Maximum is 32256 bytes.
Global variables use 232 bytes (11%) of dynamic memory, leaving 1816 bytes for local variables. Maximum is 2048 bytes.
```
**Overhead: 814B code, 12B data**
If there's no reference to SerialMenu there's no overhead to include the library. If there is, as above, the overhead is the creation of the SerialMenu singleton instance. It's a one time hit.
As coded above, this will just print the library's copyright stored in FLASH memory:
```
SerialMenu - Copyright (c) 2019 Dan Truong
hello world
```

It is possible to disable printing unecessary text like the copyright with a macro to reduce memory consumption:
```C++
#define SERIALMENU_MINIMAL_FOOTPRINT true
#include <SerialMenu.hpp>
SerialMenu& menu = SerialMenu::get();
```
```
Sketch uses 2782 bytes (8%) of program storage space. Maximum is 32256 bytes.
Global variables use 232 bytes (11%) of dynamic memory, leaving 1816 bytes for local variables. Maximum is 2048 bytes.
```
**Overhead: 662B code, 12B data**
This result is reasonable: The singleton is made up of 2 pointers, a short and a byte (11B total).
Let's add the function calls needed to run the menus. We now are just displaying an empty menu.
```C++
#define SERIALMENU_MINIMAL_FOOTPRINT true
#include <SerialMenu.hpp>
SerialMenu& menu = SerialMenu::get();

void setup() {
  Serial.begin(9600);
  while (!Serial){};
  Serial.print("hello ");
  Serial.println("world");
...
  menu.load(nullptr, 0);
  menu.show();
}

void loop() {
  menu.run(1);
...}
```
```
Sketch uses 3270 bytes (10%) of program storage space. Maximum is 32256 bytes.
Global variables use 263 bytes (12%) of dynamic memory, leaving 1785 bytes for local variables. Maximum is 2048 bytes.
```
**Overhead: 1150B code, 43B data**
The 3 routines added extra code and another 32B of space used. I don't know where that went.

### Adding a menu with 2 entries:
Since the goal is to use the least SRAM data memory, we'll declare a small menu with two entries where the text is stored in FLASH memory via the PROGMEM keyword. Here's the full modified program:
```C++
/*
  LED bar graph
...
  created 4 Sep 2010
  by Tom Igoe
...
  http://www.arduino.cc/en/Tutorial/BarGraph
*/
#define SERIALMENU_MINIMAL_FOOTPRINT true
#include <SerialMenu.hpp>

// these constants won't change:
const int analogPin = A0;
const int ledCount = 10;

int ledPins[] = {
  2, 3, 4, 5, 6, 7, 8, 9, 10, 11
};   // an array of pin numbers to which LEDs are attached

SerialMenu& menu = SerialMenu::get();

const char PROGMEM menu1[] = "A: display one";
const char PROGMEM menu2[] = "B: redisplay the menu";

const SerialMenuEntry mainMenu[] = {
  {menu1,  true, 'A', [](){ Serial.println("1"); } },
  {menu2, true, 'B', [](){ menu.show(); } }
};
constexpr uint8_t mainMenuSize = GET_MENU_SIZE(mainMenu);

void setup() {
  Serial.begin(9600);
  while (!Serial){};
  Serial.print("hello ");
  Serial.println("world");

  for (int thisLed = 0; thisLed < ledCount; thisLed++) {
    pinMode(ledPins[thisLed], OUTPUT);
  }

  menu.load(mainMenu, mainMenuSize);
  menu.show();
}

void loop() {
  menu.run(1);

  int sensorReading = analogRead(analogPin);
  int ledLevel = map(sensorReading, 0, 1023, 0, ledCount);

  for (int thisLed = 0; thisLed < ledCount; thisLed++) {
    if (thisLed < ledLevel) {
      digitalWrite(ledPins[thisLed], HIGH);
    }
    else {
      digitalWrite(ledPins[thisLed], LOW);
    }
  }
}
```
```
Sketch uses 3424 bytes (10%) of program storage space. Maximum is 32256 bytes.
Global variables use 275 bytes (13%) of dynamic memory, leaving 1773 bytes for local variables. Maximum is 2048 bytes.
```
**Overhead: 1304B code, 55B data**
We added two menu entries, each holding a function pointer (not in SRAM), a data pointer and a byte (5B).
The menu is functional, and the output is as follows, when entering A then B:
```
hello world
A: display one
B: redisplay the menu
1
A: display one
B: redisplay the menu
...
```

## Notes:

Be carefuly with memory constrained boards, running out of SRAM will lead
to flaky code behavior. Keep menu text minimal as it could eat up memory.
To alleviate memory pressure menus can be stored in Flash program memory
using the PROGMEM keyword, instead of SRAM data memory. However, even Flash
memory is limited.

Menu callback functions can be declared as separate functions or as lambda
functions directly in the data structure with the menu data. Lambda notation
is best for simple tasks like setting a global variable or calling another
menu, as it keeps the code for a menu entry concise and within the menu
entry definition. See the example.
A lambda function syntax is written "[](){}" where the code goes inside {}.
The other elements "[]()" are not used here.

## Installation:

Copy this package in your Arduino's "library" directory. For example on Mac
this will be in "Documents/Arduino/libraries".

To test load and run the example from the Files menu:
File -> Examples -> SerialMenu -> Demo
Open your Serial console window in the IDE from the Tools menu:
Tools -> Serial Monitor

You should see the demo menu, and interact with it.

## Usage example:

Let's create two menus which call each other, with a varying number of menu
entries, some of them stored in Flash (PROGMEM), and some in SRAM.

```C++
    #include <SerialMenu.hpp>
    
    // Forward declaration of menu2, because it is referenced before definition
    extern const SerialMenuEntry menu2[];
    extern const uint8_t menu2Size;
    
    // You can declare menu strings separately (a must for PROGMEM FLASH)
    const char menu1String1[] = "Y - residplay this menu (Text in SRAM)";
    const char menu1String2[] PROGMEM = "Z - second menu (Text in FLASH)";
    
    // Definition of menu1:
    // A menu entry is defined with four fields.
    // -Text can be embedded directly or you can reference a string name
    // -Text in FLASH via PROGMEM is flagged as true, else flagged as false
    // -Declare the keypress assigned to a menu entry (converts to lowercase)
    // -Declare the callback as a lambda function or use a function pointer
    const SerialMenuEntry menu1[] = {
     {"X (Text in SRAM)", false, '1', [](){ Serial.println("choice X!"); } },
     {menu1String1,       false, 'y', [](){ menu.show(); } },
     {menu1String2,       true,  'z', [](){ menu.load(menu2, menu2Size);
                                            menu.show(); } }
    };
    constexpr uint8_t menu1Size = GET_MENU_SIZE(menu1);
    
    // Global variables updated by menu2
    uint16_t var1, var2;
    
    // Function called by menu2
    void foo()
    {
       var1 = menu.getUint16_t();
       Serial.println("Running foo!");
    }
    
    // Definition of menu2:
    // Notice that:
    // -Embedded strings can't be declared PROGMEM so we declare "false"
    // -Using 'B' vs 'b' doesn't matter (lowercase auto-conversion)
    // -We call the function foo() instead of a lambda function
    const SerialMenuEntry menu2[] = {
     {"Execute foo()", false, 'e', foo },
     {"Set var2",      false, 'S', [](){ var2 = menu.getNumber<uint16_t>(); } },
     {"Redisplay menu",false, 'r', [](){ menu.show(); } },
     {"Back to menu1", false, 'B', [](){ menu.load(menu1, menu1Size);
                                         menu.show(); } }
    };
    constexpr uint8_t menu2Size = GET_MENU_SIZE(menu2);
    
    // Main arduino code:
    
    void setup() {
     // Install menu1 as the current menu to run
     menu.load(menu1, menu1Size);
     // Display current menu (menu1)
     menu.show();
    }
    
    void loop() {
     // Run the menus:
     // Wait for menu user input. Pass-on the main loop delay so menu
     // library knows the elapsed time since it was last checked.
     menu.run(100);
     
     // Add here your code to do stuff
     
     delay(100);
    }
```

# Optimization Macros
The library by default will maximize functionality and verbosity. It is possible to define macros *before* the library's inclusion, to disable some features and henceforce reduce the memory footprint.

```C++
///////////////////////////////////////////////////////////////////////////////
// If user doesn't specify disabling PROGMEM support, support is on by default.
// To disable set SERIALMENU_DISABLE_PROGMEM_SUPPORT explicitly to true.
// This is safe to do if none of the menu entries are stored in FLASH memory.
///////////////////////////////////////////////////////////////////////////////
#define SERIALMENU_DISABLE_PROGMEM_SUPPORT true

///////////////////////////////////////////////////////////////////////////////
// The menu prints dots every 10s, and blinks the status LED after 10s to show
// the program has not crashed and is still waiting for input.
// To disable set SERIALMENU_DISABLE_HEARTBEAT_ON_IDLE explicitly to true.
///////////////////////////////////////////////////////////////////////////////
#define SERIALMENU_DISABLE_HEARTBEAT_ON_IDLE true

///////////////////////////////////////////////////////////////////////////////
// The library prints some extra text like copyrights etc.
// To disable set SERIALMENU_MINIMAL_FOOTPRINT explicitly to true.
///////////////////////////////////////////////////////////////////////////////
#define SERIALMENU_MINIMAL_FOOTPRINT true
#include <SerialMenu.hpp>
```
