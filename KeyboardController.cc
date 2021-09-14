// Onboard driver for keyboard prototype 2
// Software version 3 (asymetric debounce)
// Arduino IDE settings: Teensy 3 (or match board); Keyboard type = "Keyboard, mouse, joystick"; CPU speed = 48MHz (will need to adjust timings for other speeds)
//                       Keyboard layout = "United Kingdom"; Optimise = "Smallest code"

// modifier keys
const int Kshift = 19, Kctrl = 7, Kalt = 18, Kgui = 17;
// mode keys
const int Mnum = 28, Mmov = 16, Mfn = 5, Mmak = 6, Msym = 31;

// A safe wait time between keystrokes so most OSs won't rate limit the input.
// When connected across RDP/VNC connections, modifier key pressed can get missed.
// If that happens, try increasing this.
#define HID_TAP_WAIT 30
// how many cycles before a key is considered 'up'
// Longer is a stronger debounce, but more delay in releasing keys.
#define DEBOUNCE_COUNT 25
// How many cycles before a key is considered 'down'
// More cycles means less likely for transient spikes to cause key presses
#define KEY_DOWN_THRESH 5

// How many key-scan cycles before mouse is updated? Higher values = slower mouse and scroll
#define MOUSE_RATE 30
// Adjustment for OS mouse speed setting. Higher value = slower mouse
#define MOUSE_SPEED_ADJUST 1
// How long will the mouse accelerate? Affects terminal speed
#define MOUSE_MAX_ACCEL 500
// Smallest mouse delta that will have an effect (used with precision movement, and as a base for normal movement)
#define MOUSE_MIN_SPEED 2

// bit masks
#define USB_LED_NUM_LOCK 1
#define USB_LED_CAPS_LOCK 2
#define USB_LED_SCROLL_LOCK 4
#define USB_LED_COMPOSE 8
#define USB_LED_KANA 16

const int ledPin =  13;      // the number of the LED pin
const int x = -1;            // unused key
char* _ = 0;                 // unused macro
int ledState = LOW;          // ledState used to set the LED
int lastMods = 0;
int mbtns = 0, prevMbtns = 0;
int mouseCycles = 0;         // used to reduce mouse speed

#define IDX_UNDERSCORE_KEY 11

/* Physical layout: (positions 40,41,42,43 not used)
scan\sense
       11 10  9  8  7  6  5  4  3  2  1  0

3      47 46 45 44 .. .. .. .. 39 38 37 36
2      35 34 33 32 .. .. .. .. 27 26 25 24
1      23 22 21 20 .. .. .. .. 15 14 13 12
0      11 10  9  8 .. .. .. ..  3  2  1  0
2                  31 30 29 28
1                  19 18 17 16
0                   7  6  5  4

                          1         2         3
             1    23456789012345678901234567890 1     2 3     4   5
Scan order: <bksp>-qpfwiangdcksthrlmoeuvbyzjx.,<space>=<menu><del,esc>

Regenerate this and copy down after changes:
30,11,44,24,12, 3,25,13,26,39,
27,35,45,14,34,33,23,22,10,20,
32,21, 9, 2, 1,46,37,38, 8, 0,
15,36,47,29, 4

basic layout
Mzkq    gxj=       (M for windows menu key)
cthe    dnip
rluo    _saf
-mv.    wby,
    s<>n           symb   bksp  del   num
    SAWm           shift  alt   win   move
    CmfE           ctrl   macr  fkey  esc

number layout
a123      x
b456
c789    ,.
def0    #~

symbols layout
  €£    %$ +
^`()    []"/
=;:!    {}'*
-\|?    <>&@
 
 
*/

#define SCAN_SIZE 48

// 'Macro' keys -- output whole strings with one keypress. Customise to your workflow.
int macroMap[0] = {};
const char* macroStrings[SCAN_SIZE] =
{
  /* 0*/_, _, "BJSS", ":w\n", _, _, _, _, _, _,
  /*10*/":make %", _, "function", _, "Scenario: ", "    ",_, _, _, _,
  /*20*/_, "Iain Ballard", _, "\\r\\n", "package", "tion", _, _, _, _,
  /*30*/_, _, "iain.ballard@", _, _, "characteristic", _, "JSON.stringify(", _, _,
  /*40*/_,_,_,_, ":q\n", _, _, _
};

// key states
char keyCycles [SCAN_SIZE]; // 0 is key-up, else key is down
boolean keys [SCAN_SIZE]; // key up/down states
boolean prevKeys[SCAN_SIZE]; // key up/down states from previous cycle
//const int KEY_MENU = 0x76 | 0x4000;
int baseMap [SCAN_SIZE] = // with no mode keys
{ 
  /* 0*/KEY_COMMA, KEY_Y, KEY_B, KEY_W, KEY_ESC, x, x, x, KEY_PERIOD, KEY_V,
  /*10*/KEY_M, KEY_MINUS, KEY_F, KEY_A, KEY_S, KEY_SPACE,x, x, x, x,
  /*20*/KEY_O, KEY_U, KEY_L, KEY_R, KEY_P, KEY_I, KEY_N, KEY_D, x, KEY_DELETE,
  /*30*/KEY_BACKSPACE, x, KEY_E, KEY_H, KEY_T, KEY_C, KEY_EQUAL, KEY_J, KEY_X, KEY_G,
  /*40*/x,x,x,x, KEY_Q, KEY_K, KEY_Z, KEY_MENU
};

const int MOUSE_M_LEFT = -10; // mouse movement
const int MOUSE_M_RIGHT = -11;
const int MOUSE_M_UP = -12;
const int MOUSE_M_DOWN = -13;
const int MOUSE_B_1 = -14;  // mouse buttons
const int MOUSE_B_2 = -15;
const int MOUSE_B_3 = -16;
const int MOUSE_S_UP = -17; // mouse scroll
const int MOUSE_S_DOWN = -18;
int moveMap[SCAN_SIZE] = // movement keys
{
  /* 0*/x, x, MOUSE_S_UP, MOUSE_S_DOWN, KEY_ESC, x, x, x, x, x,
  /*10*/x, x, x, MOUSE_M_DOWN, MOUSE_M_LEFT, KEY_ENTER,x, x, x, x,
  /*20*/KEY_RIGHT, KEY_DOWN, KEY_LEFT, x, MOUSE_M_RIGHT, MOUSE_M_UP, KEY_TAB, KEY_BACKSPACE, x, KEY_DELETE,
  /*30*/KEY_BACKSPACE, x, x, KEY_UP, x, x, x, MOUSE_B_3, MOUSE_B_2, MOUSE_B_1,
  /*40*/x,x,x,x, KEY_END, x, KEY_HOME, x
};


int fkeysMap[SCAN_SIZE] = // function keys
{
  /* 0*/KEY_F12, KEY_F11, KEY_F10,KEY_F9, x, x, x, x, KEY_INSERT, x,
  /*10*/x, KEY_PRINTSCREEN,KEY_F8, KEY_F7, KEY_F6, KEY_F5,x, x, x, x,
  /*20*/x, x, x, KEY_PAGE_DOWN, KEY_F4, KEY_F3, KEY_F2, KEY_F1, x, x,
  /*30*/x, x,x, x,x, KEY_PAGE_UP,  KEY_PAUSE, x, x, x,
  /*40*/x,x,x,x, x, x,x, KEY_NUM_LOCK
};

// KEY_BACKSLASH -> hash/tilde
// KEY_TILDE -> backtick/log not
const int KEY_NON_US_BACKSLASH = 0 - '\\'; // I cant get the backslash key to work any other way :-(
const int KEY_BACKTICK = KEY_TILDE; // wrong on UK layout
const int KEY_NON_US_HASH = KEY_BACKSLASH;
const int KEY_NON_US_TILDE = 0 - '~';
int numberMap [SCAN_SIZE] = // number keys, hex, plus a few symbols
{
  /* 0*/x, x, KEY_NON_US_TILDE, KEY_NON_US_HASH, KEY_ESC, x, x, x, KEY_0, KEY_F,
  /*10*/KEY_E, KEY_D, x, x, KEY_PERIOD, KEY_COMMA,x, x, x, x,
  /*20*/KEY_9, KEY_8, KEY_7, KEY_C, x, x, x,x, x, KEY_DELETE,
  /*30*/KEY_BACKSPACE, x, KEY_6, KEY_5, KEY_4, KEY_B, KEYPAD_PLUS, x, KEY_X, x,
  /*40*/x,x,x,x, KEY_3, KEY_2, KEY_1, KEY_A
};

const int SPECIAL_GBP = -2;
const int SPECIAL_EURO = -3;
int symbolMap [SCAN_SIZE] = // special symbols. These are done with write so can be any printable
{
  /* 0*/'@', '&', '>', '<', KEY_ESC, x, x, x, '?', '|',
  /*10*/'\\', '-', '*', '\'', '}', '{',x, x, x, x,
  /*20*/'!', ':', ';', '=', '/', '"', ']', '[', x, KEY_DELETE,
  /*30*/KEY_BACKSPACE, x, ')', '(', '`', '^', '+', x, '$', '%',
  /*40*/x,x,x,x, SPECIAL_GBP, SPECIAL_EURO, x, x
};


// the used map switched by mode keys
int* currentMap = baseMap;
int* nextMap = NULL;
int safeMode = false;
int mouseCursAccel = 0;
int underscoreMode = 0; // special mode where space outputs underscores. Toggled with macro plus hyphen

// compensate for wire order -- makes typos less common.
// the purpose is to scan left hand left-to-right, and right hand right-to-left, as these are the fastest directions.
// There are a few inversions and oddities which are specific refinements.
// We only have entries for typing keys, not mode keys. The normal key loop must match this.
int keyScanOrder[35] = {30,11,44,24,12, 3,25,13,26,39,27,35,45,14,34,33,23,22,10,20,32,21, 9, 2, 1,46,37,38, 8, 0,15,36,47,29, 4};
int safeModeDisable[6] = {5,6,7,17,18,47};

void setup() {
  delay(100);
  // initialize control over the keyboard:
  Keyboard.begin();

  // Set all lines safe (not sensing or activating lines)
  int i;
  for (i = 0; i <= 31; i++) {
    pinMode(i, OUTPUT);
    digitalWrite(i, HIGH);
  }
  
  // LED for diagnostics
  pinMode(ledPin, OUTPUT);

  i = 0;
  for (i = 0; i < SCAN_SIZE; i++) {
    keyCycles[i] = 0;
    keys[i] = false;
    prevKeys[i] = false;
  }
  
  digitalWrite(ledPin, HIGH);
  delay(500);
  
  // read the 'Macro' and 'Function' keys. If they are held at startup, we go into safe mode
  //   this is a child-friendly mode where we disable the following keys:
  //   ctrl, macro, function, alt, win, menu (idx = 5,6,7,17,18,47)
  //   these will stay disabled until the power is cycled (un-plug and re-plug)
  if (readSingleKey(5,0) && readSingleKey(6,0)) {safeMode = true;}
  
  digitalWrite(ledPin, LOW); // ready for action!
}

//////////////////////////////////////////////////////
// Main sense and respond loop                      //
//////////////////////////////////////////////////////
void loop() {
  int i;
  int idx = 0;
  ledState = LOW;
  mbtns = 0;
  bool anyMouse = false;

  // STEP 1
  // Scan the keyboard.
  // Any keys connected during the scan will be triggered,
  // all others will be decremented
  rescanKeyboard();
  mouseCycles = (mouseCycles + 1) % MOUSE_RATE;

  // STEP 2
  // Set modes and modifiers
  // read modifiers (set later, so modes can override)
  int mods = 0;
  if (keys[Kshift]) {
    mods |= MODIFIERKEY_SHIFT;
    // ensure the caps-lock is off, as we don't provide a physical key mapping for it.
    if (!prevKeys[Kshift] && ((keyboard_leds & USB_LED_CAPS_LOCK) != 0)) {tapKey(KEY_CAPS_LOCK);}
  }
  if (keys[Kctrl])  mods |= MODIFIERKEY_CTRL;
  if (keys[Kalt])   mods |= MODIFIERKEY_ALT;
  if (keys[Kgui])   mods |= MODIFIERKEY_GUI;

  // set mode
  if (keys[Mmov])      nextMap = moveMap;   // movement overrides symbols,
  else if (keys[Msym]) nextMap = symbolMap; // symbols overrides numbers...
  else if (keys[Mnum]) nextMap = numberMap; 
  else if (keys[Mfn])  nextMap = fkeysMap;
  else if (keys[Mmak]) nextMap = macroMap;
  else                 nextMap = baseMap;
  
  if (nextMap != currentMap) {// If we switch mode, clear keys. Otherwise we will get stuck keys.
    clearKeys();
    currentMap = nextMap;
  }
  if (mods != lastMods) { // set modifiers (mod keys need to be pressed a scan before affected keys)
    Keyboard.set_modifier(mods);
    Keyboard.send_now();
    lastMods = mods;
  }

  // STEP 3
  // Send key events
  for (i = 0; i < 35; i++) {
    idx = keyScanOrder[i]; // lookup table so we scan in a specific order to make typos less common.
    if (currentMap[idx] == x) continue; // is an ignored key (should not happen!)
    
    if (!keys[idx] && prevKeys[idx]) { // released -- for normal keys only!
      if (currentMap[idx] != x && currentMap != symbolMap && currentMap[idx] >= 0) {
        // Note: Any keyboard.PRESS calls must be mirrored here
        if (currentMap == numberMap && keys[Kalt]) {
          Keyboard.release(remapNumbersToPad(currentMap[idx]));
        } else {
          Keyboard.release(currentMap[idx]);
        }
      }
      continue; // next key
    }
    
    // mouse movement keys:
    if (currentMap == moveMap && keys[idx] && currentMap[idx] < -9 && currentMap[idx] > -19) {
      anyMouse = true;
      mbtns |= handleMouseKeys(currentMap[idx], /*medium*/ keys[Kshift], /*slow*/ keys[Kctrl], /*isRepeat*/prevKeys[idx]);
      continue;
    }
    
    // if pressed, but not on this cycle skip the rest of the loop.
    if (!keys[idx] || prevKeys[idx]) continue;
    
    // We have a keypress...
    // Chain of `if`s to handle special maps...
    if (currentMap == symbolMap) {
      int val = currentMap[idx];
      if (val == SPECIAL_GBP) {tapKeyWithMods(KEY_3, MODIFIERKEY_SHIFT);}
      else if (val == SPECIAL_EURO) {tapKeyWithMods(KEY_4, MODIFIERKEY_CTRL | MODIFIERKEY_ALT);}
      else {tapKeyWithMods(val, mods); }
    }
    else if (currentMap == macroMap && (idx==IDX_UNDERSCORE_KEY)) { // special mode -- toggle space outputs underscores
      underscoreMode = !underscoreMode;
    }
    else if (currentMap == macroMap && macroStrings[idx] != 0) {
      smartPrint(macroStrings[idx], /*shifted*/keys[Kshift]);
    }
    // handle some awkward keys:
    else if (currentMap[idx] < 0) Keyboard.write(-currentMap[idx]);
    // allow alt-numpad entry:
    else if (currentMap == numberMap && keys[Kalt]) {
      if ((keyboard_leds & USB_LED_NUM_LOCK) == 0) {tapKey(KEY_NUM_LOCK);}// ensure the numlock is on
      Keyboard.press(remapNumbersToPad(currentMap[idx]));
    }
    else if (currentMap[idx] == KEY_SPACE && underscoreMode) {
      tapKeyWithMods('_', mods);
    }
    // Then finally, NORMAL KEY PRESSES HERE:
    else {
      Keyboard.press(currentMap[idx]);
    }
  }
  
  // STEP 4
  // update mouse buttons
  if (!anyMouse) mouseCursAccel = 0;
  if (mbtns != prevMbtns) Mouse.set_buttons((mbtns&1), (mbtns&2)>>1, (mbtns&4)>>2);


  // STEP 5
  // Copy states for next cycle
  for (i = 0; i < 48; i++) {
    prevKeys[i] = keys[i];
  }
  prevMbtns = mbtns;
  
  // flash the LED if any key is down
  digitalWrite(ledPin, ledState);
}

// momentarily press a key without changing modifiers
void tapKey(int scanCode) {
  Keyboard.press(scanCode);
  delay(HID_TAP_WAIT);
  Keyboard.release(scanCode);
}

// Same as keyboard.print, but handles special characters
// if shift is down, flip the case bit for first character.
void smartPrint(const char *str, bool shifted) {
  int i = 0;
  if (shifted) {
    Keyboard.write(str[0] ^ 0x20);
    delay(HID_TAP_WAIT);
    i++;
  }
  for (; str[i] != 0; i++) {
    if (str[i] == '\t') {
      tapKey(KEY_TAB);
    } else {
      Keyboard.write(str[i]);
    }
    delay(HID_TAP_WAIT);
  }
}

// momentarily press a key and change modifiers. They get changed back afterwards.
void tapKeyWithMods(int scanCode, int tempMods) {
  Keyboard.set_modifier(tempMods);
  Keyboard.send_now();
  delay(HID_TAP_WAIT); // Only needed to make Microsoft RDP behave.
  
  Keyboard.press(scanCode);
  delay(HID_TAP_WAIT);
  Keyboard.release(scanCode);

  Keyboard.set_modifier(lastMods);
  Keyboard.send_now();
}

int handleMouseKeys(int key, int medium, int slow, int isRepeat) {
  int s = 1;

  // calculate movement speeds
  int fast = (medium) ? (3) : (1); // this way around, shift = faster. Flip for shift = slower
  int rs = MOUSE_MIN_SPEED + (mouseCursAccel >> MOUSE_SPEED_ADJUST);
  int d = (slow) ? (MOUSE_MIN_SPEED) : (rs * fast);

  // Rate limiting and acceleration
  if (isRepeat) {
    if (slow) {d = 0; s = 0;} // don't repeat in 'slow' move (ctrl = single step)
    else if (mouseCycles != 0) {d = 0; s = 0;} // apply move and scroll rate limit from defines at top
    else if (mouseCursAccel < MOUSE_MAX_ACCEL) mouseCursAccel++;
  }
  
  switch (key) {
    // this is quite rough, and makes diagonal movement overspeed.
    case MOUSE_M_LEFT: Mouse.move(-d,0); break;
    case MOUSE_M_RIGHT: Mouse.move(d,0); break;
    case MOUSE_M_UP: Mouse.move(0,-d); break;
    case MOUSE_M_DOWN: Mouse.move(0,d); break;
    
    case MOUSE_S_UP: Mouse.scroll(s); break;
    case MOUSE_S_DOWN: Mouse.scroll(-s); break;
    
    case MOUSE_B_1: return 1;
    case MOUSE_B_2: return 2;
    case MOUSE_B_3: return 4;

  }
  return 0;
}

int remapNumbersToPad(int pressed) {
  switch (pressed) {
    case KEY_0: return KEYPAD_0;
    case KEY_1: return KEYPAD_1;
    case KEY_2: return KEYPAD_2;
    case KEY_3: return KEYPAD_3;
    case KEY_4: return KEYPAD_4;
    case KEY_5: return KEYPAD_5;
    case KEY_6: return KEYPAD_6;
    case KEY_7: return KEYPAD_7;
    case KEY_8: return KEYPAD_8;
    case KEY_9: return KEYPAD_9;
    default: return pressed;    
  }
}

// Scan all the keys, and update the current state:
// Reads physical switches. The loops may need to be swapped if you put the diodes on in reverse.
void rescanKeyboard() {
  int scan;
  int sense;
  int state = false;
  int i, idx;

  noInterrupts(); // don't accept timer interrupts during scan
  for (i = 20; i <= 23; i++) { // set sense line to pull high
    pinMode(i, INPUT_PULLUP);
    digitalWrite(i, HIGH);
  }
  
  for (scan = 0; scan < 12; scan++) { // scan line loop
    // activate scan line
    pinMode(scan, OUTPUT);
    digitalWrite(scan, LOW);
  
    for (sense = 0; sense < 4; sense++) { // sense line loop
      idx = scan + (sense * 12); // key state index
      
      state = digitalRead(sense + 20) == LOW;
      if (state) {
        ledState = HIGH; // light up the LED when any key is down
        keyCycles[idx] = saturateIncr(keyCycles[idx], DEBOUNCE_COUNT); // increase key down cycles
      } else {
        keyCycles[idx] = saturateDecr(keyCycles[idx], 0); // reduce key down cycles
      }
    }
    
    // deactivate scan line
    digitalWrite(scan, HIGH);
  }
  
  for (i = 20; i <= 23; i++) { // switch sense lines off
    digitalWrite(i, HIGH);
  }
  
  // If in safe mode, turn off system control keys
  if (safeMode) {
    for (i = 0; i < 6; i++) {
      if (keyCycles[safeModeDisable[i]]) ledState = LOW; // turn the LED off to show it's working
      keyCycles[safeModeDisable[i]] = 0;
    }
  }

  for (i = 0; i < SCAN_SIZE; i++) {
    keys[i] = keyCycles[i] > KEY_DOWN_THRESH; // key latency. Increasing reduces spike ghosting, but increases delay to key down event
    if (keys[i] && !prevKeys[i]) keyCycles[i] = DEBOUNCE_COUNT; // key transitioned, so peg it for debounce.
  }
  interrupts(); // re-enable timers
}

inline int saturateIncr(int current, int maximum) {
  return (current >= maximum) ? maximum : current + 1;
}

inline int saturateDecr(int current, int minimum) {
  return (current <= minimum) ? minimum : current - 1;
}

// Reads physical switches. 'col' and 'row' may need to be swapped if you put the diodes on in reverse.
int readSingleKey(int col, int row) {
  int state = false;
  // power on
  pinMode(col, OUTPUT);
  pinMode(row, INPUT_PULLUP);
  digitalWrite(col, LOW);
  digitalWrite(row, HIGH);
  
  state = digitalRead(row + 20) == LOW;
    
  // power off
  digitalWrite(col, HIGH);
  digitalWrite(row, HIGH);
  
  return state;
}

// Turn all non modifier keys off 
void clearKeys() {
  // Reset the keyboard state for the host
  Keyboard.set_key1(0);
  Keyboard.set_key2(0);
  Keyboard.set_key3(0);
  Keyboard.set_key4(0);
  Keyboard.set_key5(0);
  Keyboard.set_key6(0);
  Keyboard.send_now();
  Mouse.set_buttons(0, 0, 0);
}
