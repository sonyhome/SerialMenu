///////////////////////////////////////////////////////////////////////////////
// Serial port Menus
#define COPYRIGHT "SerialMenu - Copyright (c) 2019 Dan Truong"
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
///////////////////////////////////////////////////////////////////////////////

#include <avr/pgmspace.h>
#include <HardwareSerial.h>

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
      key(((isprogMem) ? (k|0x20) : (k&(~0x20)))),
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
//    SerialMenu* menuPtr = SerialMenu::get();
// }
///////////////////////////////////////////////////////////////////////////////
class SerialMenu
{
  private:
    // Usually Arduino boards have a status LED on them. If not the code should
    // optimize away the LED control code because LED_BUILTIN should not be
    // defined. If it is defined and the code breaks because the LED is not
    // on the board, try to comment the SHOW_HEARTBEAT_ON_IDLE define.
    #ifdef  LED_BUILTIN
    #define SHOW_HEARTBEAT_ON_IDLE 1
    #endif

    // If PROGMEM is used, copy using this SRAM buffer size.
    #define PROGMEM_BUF_SIZE 8

    // This class implements a singleton desgin pattern with one static instance
    static SerialMenu singleton;

    // Points to the array of menu entries for the current menu
    const SerialMenuEntry * menu;
    // number of entries in the current menu
    uint8_t size;

    // Private constructor for singleton design.
    // Initializes with an empty menu, prepares serial console and staus LED.
    SerialMenu() :
    menu(nullptr),
    size(0)
    {
      Serial.begin(9600);
      while (!Serial){};
      Serial.println(COPYRIGHT);
      
      // Prepare to blink built-in LED.
      #ifdef SHOW_HEARTBEAT_ON_IDLE
      {
        pinMode(LED_BUILTIN, OUTPUT);
      }
      #endif
    }

  public:
    // Get a pointer to the one singleton instance of this class
    static SerialMenu & get()
    {
      return singleton;
    }

    // Get a pointer to the one singleton instance of this class and point it
    // to the current menu
    static SerialMenu & get(const SerialMenuEntry* array, uint8_t arraySize)
    {
      SerialMenu & m = get();
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
      Serial.println("\nMenu:");
      for (uint8_t i = 0; i < size; ++i)
      {
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
            len = strlcpy_P(buffer, progMemPt, PROGMEM_BUF_SIZE);
            Serial.print(buffer);
          }
          Serial.println("");
        }
        else
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
        Serial.print("[negative] ");
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
      #ifdef SHOW_HEARTBEAT_ON_IDLE
      {
        const uint16_t callsPerSecond = 1000 / loopDelayMs;
        const uint16_t loopsPerTick = 10 * callsPerSecond;
        const uint16_t loopsPerBlink = callsPerSecond; // blink every second

        // Count how long we've been waiting for the user to input data
        static uint16_t waiting = 0;

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

// A single variable for the menus
extern SerialMenu& menu;