# SerialMenu
Serial Console Menu Library for Arduino

## Overview
This library allows you to create menus on the Arduino Serial console
and call a single function to display them, and call another function
in the main loop to automatically monitor them and execute the callback
function associated to the menu entry selected.

The menus are non-blocking, but only trigger when there is user input.
This allows your programs to simultaneously go about the work you want
them to do while responding to serial commands promptly.

## Benefits:

* Simple to declare a menu as a global variable
* One call to display the menu
* Easily support sub-menu hierarchies
* Compact user code
* Menu text can be stored in SRAM or Flash (data or program memory)

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

## Usage example:

Let's create two menus which call each other, with a varying number of menu
entries, some of them stored in Flash (PROGMEM), and some in SRAM.

    //// Forward declaration of other menus, needed before using them:
    extern const SerialMenuEntry menu2[];
    extern const uint8_t menu2Size;
    
    //// You can declare some menu strings separately (needed for FLASH)
    const char menu1String1[] = "Y - residplay this menu (Text in SRAM)";
    const char menu1String2[] PROGMEM = "Z - second menu (Text in FLASH)";
    
    //// Definition of menu1:
    
    //// Text is either embedded direct or a string name is referenced
    //// Text in FLASH via PROGMEM is flagged as true
    //// The menu entry's key tp press is specified (converts to lowercase)
    //// followed by the lambda functions
    const SerialMenuEntry menu1[] = {
     {"X (Text in SRAM)", false, '1', [](){ Serial.println("choice X!"); } },
     {menu1String1,       false, 'y', [](){ menu.show(); } },
     {menu1String2,       true,  'Z', [](){ menu.load(menu2, menu2Size);
                                            menu.show(); } }
    };
    constexpr uint8_t menu1Size = GET_MENU_SIZE(menu1);
    
    //// Global variables updated by menu2
    uint16_t var1, var2;
    
    //// Function called by menu2
    void foo()
    {
       var1 = menu.getUint16_t();
       Serial.println("Running foo!");
    }
    
    //// Definition of menu2:
    //// Notice we can call foo() function pointer instead of a lambda
    const SerialMenuEntry menu2[] = {
     {"Execute foo()", false, 'e', foo },
     {"Set var2",      false, 'S', [](){ var2 = menu.getUint16_t(); } },
     {"Rediplay menu", false, 'r', [](){ menu.show(); } },
     {"Back to menu1", false, 'B', [](){ menu.load(menu1, menu1Size);
                                         menu.show(); } }
    };
    constexpr uint8_t menu2Size = GET_MENU_SIZE(menu2);
    
    //// Main arduino code
    void setup() {
     //// Install menu1
     menu.load(menu1, menu1Size);
     //// Display current menu (menu1)
     menu.show();
    }
    
    void loop() {
     //// Wait for menu user input. Pass-on the main loop delay so menu
     //// library knows the elapsed time since it was last checked.
     menu.run(100);
     //// Add here your code to do stuff
     delay(100);
    }
