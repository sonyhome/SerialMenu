#if SERIALMENU_DISABLE_PROGMEM_SUPPORT != true
constexpr PROGMEM char SERIAL_MENU_COPYRIGHT[] = 
#else
constexpr char SERIAL_MENU_COPYRIGHT[] = 
#endif
///////////////////////////////////////////////////////////////////////////////
// SerialMenu - An Efficient Menu Library for Arduino Serial Console
"SerialMenu - Copyright (c) 2019 Dan Truong";
///////////////////////////////////////////////////////////////////////////////
//
// This library allows you to easily create menus on the Arduino Serial console
// with single function call to display them, and automatically execute the
// callback functions when an enry is selected.
// The menu is called at every iteration but only executes when there is user
// input.
//
////////////
// Benefits:
////////////
// * Simple to declare a menu as a global variable
// * One call to display the menu
// * Easily support sub-menu hierarchies
// * Compact code
// * Menu text can be stored in SRAM or Flash (data or program memory)
//
/////////
// Notes:
/////////
// Be carefuly with memory constrained boards, running out of SRAM will lead
// to flaky code behavior. Keep menu text minimal as it could eat up memory.
// To alleviate memory pressure menus can be stored in Flash program memory
// using the PROGMEM keyword, instead of SRAM data memory. However, even Flash
// memory is limited.
//
// Menu callback functions can be declared as separate functions or as lambda
// functions directly in the data structure with the menu data. Lambda notation
// is best for simple tasks like setting a global variable or calling another
// menu, as it keeps the code for a menu entry concise and within the menu
// entry definition. See the example.
// A lambda function syntax is written "[](){}" where the code goes inside {}.
// The other elements "[]()" are not used here.
//
/////////////////
// Usage example:
/////////////////
//
// Let's create two menus which call each other, with a varying number of menu
// entries, some of them stored in Flash (PROGMEM), and some in SRAM:
//
// //// Forward declaration of other menus, needed before using them: ////
// extern const SerialMenuEntry menu2[];
// extern const uint8_t menu2Size;
//
// //// You can declare some menu strings separately (needed for FLASH) ////
// const char menu1String1[] = "Y - residplay this menu (Text in SRAM)";
// const char menu1String2[] PROGMEM = "Z - second menu (Text in FLASH)";
// 
// //// Definition of menu1: ///
// //// Text is either embedded direct or a string name is referenced ////
// //// Text in FLASH via PROGMEM is flagged as true ////
// //// The menu entry's key tp press is specified (converts to lowercase) ////
// //// followed by the lambda functions ////
// const SerialMenuEntry menu1[] = {
//  {"X (Text in SRAM)", false, '1', [](){ Serial.println("choice X!"); } },
//  {menu1String1,       false, 'y', [](){ menu.show(); } },
//  {menu1String2,       true,  'Z', [](){ menu.load(menu2, menu2Size);
//                                         menu.show(); } }
// };
// constexpr uint8_t menu1Size = GET_MENU_SIZE(menu1);
//
// //// Global variables updated by menu2 ////
// uint16_t var1, var2;
//
// //// Function called by menu2 ////
// void foo()
// {
//    var1 = menu.getUint16_t();
//    Serial.println("Running foo!");
// }
//
// //// Definition of menu2: ////
// //// Notice we can call foo() function pointer instead of a lambda ////
// const SerialMenuEntry menu2[] = {
//  {"Execute foo()", false, 'e', foo },
//  {"Set var2",      false, 'S', [](){ var2 = menu.getUint16_t(); } },
//  {"Rediplay menu", false, 'r', [](){ menu.show(); } },
//  {"Back to menu1", false, 'B', [](){ menu.load(menu1, menu1Size);
//                                      menu.show(); } }
// };
// constexpr uint8_t menu2Size = GET_MENU_SIZE(menu2);
//
// //// Main arduino code ////
// void setup() {
//  //// Install menu1 ////
//  menu.load(menu1, menu1Size);
//  //// Display current menu (menu1) ////
//  menu.show();
// }
//
// void loop() {
//  //// Wait for menu user input. Just pass the main loop delay so menu ////
//  //// code is told how long elapsed since it was last checked. ////
//  menu.run(100);
//  //// Add here your code to do stuff ////
//  delay(100);
// }
//
//////////////////
// Design overview
//////////////////
// The code is pretty easy to follow. However here are some pointers.
// A menu is defined as an array of SerialMenuEntry elements. Each entry
// tracks a pointer to the callback to run when the menu is selected, a pointer
// to the string to display (the pointer can be a PROGMEM pointer), and a key
// which is the keypress that will trigger the selection of this menu entry.
// The keys are converted to lowercase by OR-ing 0x20. Since the bit 0x20 is
// not used, it is used as a flag to tell if the string is stored in FLASH via
// PROGMEM, or if it is stored in SRAM like regular data.
//
// The SerialMenu class is a singleton class. Only one instance can exist.
// To do so we provide the get() method, which if needed allocates that one
// singleton instance. To prevent other instances to exist, the constructor is
// kept private. Furthermore I declared all the class variables static.
//
// The usage pattern is to copy the pointer to the array. We must pass the size
// too so a macro is provided for that. One those are in SerialMenu, the show()
// command can scan the array to print the menu, and run() can scan the keys to
// detect if an input is a valid menu entry, and if so it calls the callback
// function defined in the array.
// This implementation allows the use of lambda functions in the array. This
// makes for menues that are very concise, where all the data and code for a
// menu entry is declared in one source line. Other solutions will tend to have
// a clutter to support menus and their actions.
// I think I am being clever doing this! :)
//
/////////
// Future
/////////
// Can I make the code more compact? Maybe circumvent using Serial's overhead?
//
// I don't foresee the need for much, except maybe support menus on other I/O
// device with a print() or println() implementation. For example LCD screen
// drivers. I would have to figure out how to do that (template?)
///////////////////////////////////////////////////////////////////////////////

#include <avr/pgmspace.h>
#include <HardwareSerial.h>

///////////////////////////////////////////////////////////////////////////////
// If user doesn't specify disabling PROGMEM support, support is on by default.
// To disable set SERIALMENU_DISABLE_PROGMEM_SUPPORT explicitly to true.
// This is safe to do if none of the menu entries are stored in FLASH memory.
///////////////////////////////////////////////////////////////////////////////
//#define SERIALMENU_DISABLE_PROGMEM_SUPPORT true

///////////////////////////////////////////////////////////////////////////////
// The menu prints dots every 10s, and blinks the status LED after 10s to show
// the program has not crashed and is still waiting for input.
// To disable set SERIALMENU_DISABLE_HEARTBEAT_ON_IDLE explicitly to true.
///////////////////////////////////////////////////////////////////////////////
//#define SERIALMENU_DISABLE_HEARTBEAT_ON_IDLE true

///////////////////////////////////////////////////////////////////////////////
// The library prints some extra text like copyrights etc.
// To disable set SERIALMENU_MINIMAL_FOOTPRINT explicitly to true.
///////////////////////////////////////////////////////////////////////////////
//#define SERIALMENU_MINIMAL_FOOTPRINT true


///////////////////////////////////////////////////////////////////////////////
// Define a menu entry as:
// - a menu message to display
// - a boolean to specify if the message is in SRAM or PROGMEM Flash memory
// - a menu key to select
// - a callback function to perform the menu action
///////////////////////////////////////////////////////////////////////////////
class SerialMenuEntry {
  public:
    // Callback function that performs this menu's action
    void (*actionCallback)();

  private:
    // Message to display via getMenu()
    // The pointer can be in SRAM or in FLASH (requires PROGMEM to access)
    const char * message;
    // Keyboard character entry to select this menu entry, overloaded:
    // We set bit 0x20 to 0 for normal message, to 1 for a PROGMEM message
    const char key;
    
  public:
    // Constructor used to init the array of menu entries
    SerialMenuEntry(const char * m, bool isprogMem, char k, void (*c)()) :
      message(m),
      //#if SERIALMENU_DISABLE_PROGMEM_SUPPORT != true
        key(((isprogMem) ? (k|0x20) : (k&(~0x20)))),
      //#else
      //  key(k),
      //#endif
      actionCallback(c)
    {}
  
    // Get the menu message to display
    inline const char * getMenu() const
    {
      return message;
    }

    inline bool isProgMem() const
    {
      return key & 0x20;
    }

    // Check if the user input k matches this menu entry
    // Characters are converted to lowecase ASCII.
    // @note this impacts also symbols, not numbers, so test before using those
    inline bool isChosen(const char k) const
    {
      return (k|0x20) == (key|0x20);
    }
};

///////////////////////////////////////////////////////////////////////////////
// Macro to get the number of menu entries in a menu array.
///////////////////////////////////////////////////////////////////////////////
#define GET_MENU_SIZE(menu) sizeof(menu)/sizeof(SerialMenuEntry)


///////////////////////////////////////////////////////////////////////////////
// The menu is a singleton class in which you load an array of menu entries.
//
// Singleton:
// In other words, you do not instantiate the class. Instead you declare a
// pointer to the class and ask the class to allocate an instance of itself
// once, and return a pointer to the only static instance the program has.
// A call to SerialMenu::get() will always return the same pointer to that
// one single instance.
//
///////////
// Example:
///////////
// loop()
// {
//    SerialMenu& menu = SerialMenu::get();
// }
// @todo Instead of a singleton we could avoid having any instance and
// convert methods to static methods using static data. It saves a pointer...
///////////////////////////////////////////////////////////////////////////////
class SerialMenu
{
  private:
    // Usually Arduino boards have a status LED on them. If not the code should
    // optimize away the LED control code because LED_BUILTIN should not be
    // defined. If it is defined and the code breaks because the LED is not
    // on the board, try to comment the SHOW_HEARTBEAT_ON_IDLE define.
    #ifdef  LED_BUILTIN
    #if SERIALMENU_DISABLE_HEARTBEAT_ON_IDLE != true
    #define SERIALMENU_SHOW_HEARTBEAT_ON_IDLE true
    #endif
    #endif

    // If PROGMEM is used, copy using this SRAM buffer size.
    #define PROGMEM_BUF_SIZE 8

    // This class implements a singleton desgin pattern with one static instance
    static const SerialMenu * singleton;

    // Points to the array of menu entries for the current menu
    static const SerialMenuEntry * menu;
    // Count how long we've been waiting for the user to input data
    static uint16_t waiting;
    // number of entries in the current menu
    static uint8_t size;

    // Private constructor for singleton design.
    // Initializes with an empty menu, prepares serial console and staus LED.
    SerialMenu()
    {
      Serial.begin(9600);
      while (!Serial){};

      #if SERIALMENU_MINIMAL_FOOTPRINT != true
        #if SERIALMENU_DISABLE_PROGMEM_SUPPORT != true
          char buffer[sizeof(SERIAL_MENU_COPYRIGHT)];
          strlcpy_P(buffer, SERIAL_MENU_COPYRIGHT, sizeof(SERIAL_MENU_COPYRIGHT));
          Serial.println(buffer);
        #else
          Serial.println(SERIAL_MENU_COPYRIGHT);
        #endif
      #endif

      // Prepare to blink built-in LED.
      #ifdef SERIALMENU_SHOW_HEARTBEAT_ON_IDLE
      {
        pinMode(LED_BUILTIN, OUTPUT);
      }
      #endif
    }

  public:
    // Get a pointer to the one singleton instance of this class
    static const SerialMenu & get()
    {
      if (singleton == nullptr)
      {
        singleton = new SerialMenu;
      }
      return *singleton;
    }

    // Get a pointer to the one singleton instance of this class and point it
    // to the current menu
    static const SerialMenu & get(const SerialMenuEntry* array, uint8_t arraySize)
    {
      const SerialMenu & m = get();
      m.load(array, arraySize);
      return m;
    }
    
    // Install the current menu to display
    inline void load(const SerialMenuEntry* array, uint8_t arraySize)
    {
      menu = array;
      size = arraySize;
    }

    // Display the current menu on the Serial console
    void show() const
    {
      #if SERIALMENU_MINIMAL_FOOTPRINT != true
      Serial.println("\nMenu:");
      #endif

      for (uint8_t i = 0; i < size; ++i)
      {
      #if SERIALMENU_DISABLE_PROGMEM_SUPPORT != true
        if (menu[i].isProgMem())
        {
          // String in PROGMEM Flash, move it via a SRAM buffer to print it
          char buffer[PROGMEM_BUF_SIZE];
          char * progMemPt = menu[i].getMenu();
          uint8_t len = strlcpy_P(buffer, progMemPt, PROGMEM_BUF_SIZE);
          Serial.print(buffer);
          while (len >= PROGMEM_BUF_SIZE)
          {
            len -= PROGMEM_BUF_SIZE - 1;
            progMemPt += PROGMEM_BUF_SIZE - 1;
            // @todo replace strlcpy_P() and buffer with moving a uint32?
            len = strlcpy_P(buffer, progMemPt, PROGMEM_BUF_SIZE);
            Serial.print(buffer);
          }
          Serial.println("");
        }
        else
      #endif
        {
          // String in data SRAM, print directly
          Serial.println(menu[i].getMenu());
        }
      }
    }

    // return a single ASCII character input read form the serial console.
    // Note: this routine is blocking execution until a number is input
    inline char getChar()
    {
      while (!Serial.available());
      return Serial.read();
    }

    // return a number input read form the serial console.
    // Note: this routine is blocking execution until a number is input
    template <class T>
    inline T getNumber(const char * const message = nullptr)
    {
      T value = 0;
      bool isNegative = false;
      T decimals = 0;
      
      if (message)
      {
        Serial.print(message);
      }
      char c = '0';
      
      // Skip the first invalid carriage return
      while (!Serial.available());
      c = Serial.read();
      if (c == 0x0A)
      {
        while (!Serial.available());
        c = Serial.read();
      }

      if (c == '-')
      {
        isNegative = true;
        while (!Serial.available());
        c = Serial.read();
      }
      
      while ((c >= '0' and c <= '9') || c == '.')
      {
        decimals *= 10;

        if (c >= '0' and c <= '9')
        {
          value = value * 10 + (c - '0');
        }
        else if (decimals == 0 && c == '.')
        {
          decimals = 1;
        }

        while (!Serial.available());
        c = Serial.read();
      }
      
      if (isNegative)
      {
        value = -value;
      }
      
      if (decimals)
      {
        value /= decimals;
      }
      
      if (message)
      {
        Serial.println(value);
      }
      return value;
    }


///////////////////////////////////////////////////////////////////////////////
    // run the menu. If the user presses a key, it will be parsed, and trigger
    // running the matching menu entry callback action. If not print an error.
    // Returns false if there was no menu input, true if there was
    bool run(const uint16_t loopDelayMs)
    {
      const bool userInputAvailable = Serial.available();

      // Code block to display a heartbeat as a dot on the Serial console and
      // also by blinking the status LED on the board.
      #if SERIALMENU_SHOW_HEARTBEAT_ON_IDLE == true
      {
        const uint16_t callsPerSecond = 1000 / loopDelayMs;
        const uint16_t loopsPerTick = 10 * callsPerSecond;
        const uint16_t loopsPerBlink = callsPerSecond; // blink every second

        // Waiting for input
        if (!userInputAvailable)
        {
          ++waiting;
          // After waiting for 10s, heartbeat blink the LED every second.
          if (waiting >= loopsPerTick && waiting % loopsPerBlink == 0)
          {
            digitalWrite(LED_BUILTIN, ((waiting / loopsPerBlink) & 0x01) ? HIGH : LOW);
         }
          // Print heartbeat every 10s on console.
          if (waiting % loopsPerTick == 0)
          {
            Serial.print(".");
          }
        }
        else
        {
          // New input: Clear to a new line if we printed ticks.
          if (waiting >= loopsPerTick)
          {
            Serial.println("");
            waiting = 0;
          }
        }
      }
      #endif

      // Process the input
      if (!userInputAvailable)
      {
        return false;
      }
      else
      {
        // Read one character from the Serial console as a menu choice.
        char menuChoice = Serial.read();
        
        // Carriage return is not a menu choice
        if (menuChoice == 0x0A)
        {
          return false;
        }
       
        uint8_t i;
        for (i = 0; i < size; ++i)
        {
          if (menu[i].isChosen(menuChoice))
          {
            menu[i].actionCallback();
            break;
          }
        }
        if (i == size)
        {
          Serial.print(menuChoice);
          Serial.println(": Invalid menu choice.");
        }
        return true;
      }
    }

};