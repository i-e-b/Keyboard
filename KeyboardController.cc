// Onboard driver for keyboard prototype 2

// modifier keys
const int Kshift = 19, Kctrl = 31, Kalt = 18, Kgui = 17;
// mode keys
const int Mnum = 28, Mmov = 16, Mfn = 5, Mmak = 6, Msym = 7;

const int ledPin =  13;      // the number of the LED pin
const int x = -1;            // unused key
char* _ = 0;                 // unused macro
int ledState = LOW;          // ledState used to set the LED
int lastMods = 0;

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



basic layout
Mzkq    gjx=       (M for windows menu key)
cthe    dnup
rlio    _saf
-mv.    wby,
    C<>n           ctrl   bksp  del   num
    SAWm           shift  alt   win   move
    smfE           symb   macr  fkey  esc

number layout
a123      x
b456
c789    ,.
def0    #~

symbols layout
        %$ +
+`()    []"/
=^:!    {}'*
;\|?    <>&@
 
 
*/

// 'Macro' keys -- output whole strings with one keypress. Customise to your workflow.
int macroMap[0] = {};
const char* macroStrings[48] =
{
  /* 0*/_, _, _, ":w\n", _, _, _, _, _, _,
  /*10*/":make %", _, _, _, _, "    ",_, _, _, _,
  /*20*/_, "Iain Ballard", _, _, _, _, _, _, _, _,
  /*30*/_, _, _, _, _, "characteristic", _, _, "JSON.stringify(", _,
  /*40*/_,_,_,_, _, _, _, _
};

// key states
boolean keys [48];
boolean prevKeys[48];
//const int KEY_MENU = 0x76 | 0x4000;
int baseMap [48] = // with no mode keys
{ 
  /* 0*/KEY_COMMA, KEY_Y, KEY_B, KEY_W, KEY_ESC, x, x, x, KEY_PERIOD, KEY_V,
  /*10*/KEY_M, KEY_MINUS, KEY_F, KEY_A, KEY_S, KEY_SPACE,x, x, x, x,
  /*20*/KEY_O, KEY_I, KEY_L, KEY_R, KEY_P, KEY_U, KEY_N, KEY_D, x, KEY_DELETE,
  /*30*/KEY_BACKSPACE, x, KEY_E, KEY_H, KEY_T, KEY_C, KEY_EQUAL, KEY_X, KEY_J, KEY_G,
  /*40*/x,x,x,x, KEY_Q, KEY_K, KEY_Z, KEY_MENU
};

int moveMap[48] = // movement keys
{
  /* 0*/x, x, x, x, KEY_ESC, x, x, x, x, x,
  /*10*/x, x, x, x, x, KEY_ENTER,x, x, x, x,
  /*20*/KEY_RIGHT, KEY_DOWN, KEY_LEFT, KEY_PAGE_DOWN, x, x, KEY_TAB, KEY_BACKSPACE, x, KEY_DELETE,
  /*30*/KEY_BACKSPACE, x, x, KEY_UP, x, KEY_PAGE_UP, x, x, x, x,
  /*40*/x,x,x,x, KEY_END, x, KEY_HOME, x
};


int fkeysMap[48] = // function keys
{
  /* 0*/KEY_F12, KEY_F11, KEY_F10,KEY_F9, x, x, x, x, KEY_INSERT, x,
  /*10*/x, KEY_PRINTSCREEN,KEY_F8, KEY_F7, KEY_F6, KEY_F5,x, x, x, x,
  /*20*/x, x, x, x, KEY_F4, KEY_F3, KEY_F2, KEY_F1, x, x,
  /*30*/x, x,x, x, x, x, KEY_PAUSE, x, x, x,
  /*40*/x,x,x,x, x, x,x, x
};

// KEY_BACKSLASH -> hash/tilde
// KEY_TILDE -> backtick/log not
const int KEY_NON_US_BACKSLASH = 0 - '\\'; // I cant get the backslash key to work any other way :-(
const int KEY_BACKTICK = KEY_TILDE; // wrong on UK layout
const int KEY_NON_US_HASH = KEY_BACKSLASH;
const int KEY_NON_US_TILDE = 0 - '~';
int numberMap [48] = // number keys, hex, plus a few symbols
{
  /* 0*/x, x, KEY_NON_US_TILDE, KEY_NON_US_HASH, KEY_ESC, x, x, x, KEY_0, KEY_F,
  /*10*/KEY_E, KEY_D, x, x, KEY_PERIOD, KEY_COMMA,x, x, x, x,
  /*20*/KEY_9, KEY_8, KEY_7, KEY_C, x, x, x,x, x, KEY_DELETE,
  /*30*/KEY_BACKSPACE, x, KEY_6, KEY_5, KEY_4, KEY_B, x, KEY_X, x, x,
  /*40*/x,x,x,x, KEY_3, KEY_2, KEY_1, KEY_A
};

int symbolMap [48] = // special symbols. These are done with write so can be any printable
{
  /* 0*/'@', '&', '>', '<', KEY_ESC, x, x, x, '?', '|',
  /*10*/'\\', ';', '*', '\'', '}', '{',x, x, x, x,
  /*20*/'!', ':', '^', '=', '/', '"', ']', '[', x, KEY_DELETE,
  /*30*/KEY_BACKSPACE, x, ')', '(', '`', '+', '+', x, '$', '%',
  /*40*/x,x,x,x, x, x, x, x
};


// the used map switched by mode keys
int* currentMap = baseMap;

// compensate for wire order -- makes typos less common.
// the purpose is to scan left hand left-to-right, and right hand right-to-left, as these are the fastest directions.
// There are a few inversions and oddities which are specific refinements.
// We only have entries for typing keys, not mode keys. The normal key loop must match this.
int keyScanOrder[35] = {47,46,45,44,36,37,38,24,25,12,13,26,39,27,14,2,1,3,35,34,33,23,10,22,21,32,20,11,0,8,9,15,30,29,4};

void setup() {
  // initialize control over the keyboard:
  Keyboard.begin();

  // LED for diagnostics
  pinMode(ledPin, OUTPUT);

  // set sense lines
  int i = 1;
  for (i = 1; i <= 12; i++) {
    pinMode(i, INPUT_PULLUP);
    digitalWrite(i, HIGH);
  }

  // set scan lines
  i = 20;
  for (i = 20; i <= 23; i++) {
    pinMode(i, INPUT);
    digitalWrite(i, HIGH);
  }

  i = 0;
  for (i = 0; i < 48; i++) {
    keys[i] = false;
    prevKeys[i] = false;
  }

  // delay for enumeration (helps on some funky systems)
  for (i = 0; i < 17; i++) {
    digitalWrite(ledPin, HIGH); // flash to show connection
    delay(100);
    digitalWrite(ledPin, LOW);
    delay(100);
  }
  
  digitalWrite(ledPin, HIGH);
  delay(500);
  digitalWrite(ledPin, LOW); // ready for action!
}

void loop() {
  int scan;
  int sense;
  int i;
  int state = false;
  int idx = 0;
  ledState = LOW;

  // STEP 1
  // Debounce delay --adjust higher for stronger debounce
  // the higher this value, the more likely key will be detected as being pressed at the same time (and so subject to anti-typo order)
  // sensible range seems to be 15 to 70. Anything outside this range either has too much bounce or misses fast typing.
  delay(45);

  // STEP 2
  // Scan all the keys, and update the current state:
  for (scan = 0; scan < 4; scan++) { // scan line loop
    for (sense = 0; sense < 12; sense++) { // sense line loop
      idx = sense + (scan * 12); // key state index
      state = ReadKey(sense, scan + 20); // read physical switch. The parameter may need to be swapped if you put the diodes on in reverse.
      if (state) ledState = HIGH; // light up the LED when any key is down

      keys[idx] = state;
    }
  }

  // STEP 3
  // Set modes and modifiers
  // read modifiers (set later, so modes can override)
  int mods = 0;
  if (keys[Kshift]) mods |= MODIFIERKEY_SHIFT;
  if (keys[Kctrl]) mods |= MODIFIERKEY_CTRL;
  if (keys[Kalt]) mods |= MODIFIERKEY_ALT;
  if (keys[Kgui]) mods |= MODIFIERKEY_GUI;

  // set mode
  int* nextMap;
  if (keys[Mmov]) nextMap = moveMap;
  else if (keys[Mnum]) nextMap = numberMap;
  else if (keys[Msym]) nextMap = symbolMap;
  else if (keys[Mfn]) nextMap = fkeysMap;
  else if (keys[Mmak]) nextMap = macroMap;
  else nextMap = baseMap;
  if (nextMap != currentMap) {// If we switch mode, clear keys. Otherwise we will get stuck keys.
    clearKeys();
    currentMap = nextMap;
    Keyboard.set_modifier(mods);
    Keyboard.send_now();
  } else if (mods != lastMods) { // set modifiers (mod keys need to be pressed a scan before affected keys)
    Keyboard.set_modifier(mods);
    Keyboard.send_now();
    lastMods = mods;
  }

  // STEP 4
  // Send key events
  for (i = 0; i < 35; i++) {
    idx = keyScanOrder[i]; // lookup table so we scan in a specific order to make typos less common.
    if (currentMap[idx] == x) continue; // is an ignored key (should not happen!)
    
    if (!keys[idx] && prevKeys[idx]) { // released -- for normal keys only!
      if (currentMap[idx] != x && currentMap != symbolMap) Keyboard.release(currentMap[idx]);
      continue; // next key
    }
    if (!keys[idx] || prevKeys[idx]) continue; // not pressed on this cycle
    
    // We haye a keypress...
    // Chain of `if`s to handle special maps...
    if (currentMap == symbolMap) Keyboard.write(currentMap[idx]);
    else if (currentMap == macroMap && macroStrings[idx] != 0) {
      // if shift is down, flip the case bit for first character.
      if (keys[Kshift]) {
        Keyboard.write(macroStrings[idx][0] ^ 0x20);
        Keyboard.print(macroStrings[idx] + 1);
      } else {
        Keyboard.print(macroStrings[idx]);
      }
    }
    // handle some awkward keys:
    else if (currentMap[idx] < 0) Keyboard.write(-currentMap[idx]);
    // Then finally, NORMAL KEY PRESSES HERE:
    else {
      Keyboard.press(currentMap[idx]);
    }
  }

  // STEP 5
  // Copy states for next cycle
  for (i = 0; i < 48; i++) {
    prevKeys[i] = keys[i];
    keys[i] = false;
  }

  digitalWrite(ledPin, ledState);
}

// inefficient, but hopefully working.
int ReadKey(int scanLine, int senseLine) {
  // start pulse
  pinMode(scanLine, OUTPUT);
  digitalWrite(scanLine, LOW);

  // sense line
  int val = digitalRead(senseLine);

  //end pulse
  digitalWrite(scanLine, HIGH);
  pinMode(scanLine, INPUT);

  return val == LOW; // sense is inverted
}

// momentarily press a key without changing modifiers
void tapKey(int scanCode) {
  Keyboard.press(scanCode);
  delay(30);
  Keyboard.release(scanCode);
}

// Turn all non modifier keys off
void clearKeys() {
  Keyboard.set_key1(0);
  Keyboard.set_key2(0);
  Keyboard.set_key3(0);
  Keyboard.set_key4(0);
  Keyboard.set_key5(0);
  Keyboard.set_key6(0);
  Keyboard.send_now();
}
