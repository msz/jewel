
/* Copyright (C) 2017 Michal Szewczak
 * Original code copyright (C) 2016 Armando L. Montanez
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

// I/O pins used
const int CLK_Pin = 2;       // Feel free to make this any unused digital pin (must be 5v tolerant!)
const int DATA_Pin = 3;      // Feel free to make this any unused digital pin (must be 5v tolerant!)
const int LED_Pin = 13;      // the number of the LED pin (for NumLock)


// Array tracks keys currently being held
uint8_t keysHeld[6] = {0, 0, 0, 0, 0, 0}; // 6 keys max


// global variables, track Ctrl, Shift, Alt, Gui (Windows), and NumLock key statuses.
int modifiers = 0;            // 1=c, 2=s, 3=sc, 4=a, 5=ac, 6=as, 7=asc, 8=g, 9=gc...
int numLock = 0;              // so you have F11, F12, and the windows key (F8 when numlock is on)

// variables utilized in main loop for reading data
int cycleReadYet = 0;        // was data read for this cycle?
int scanCode = 0;            // Raw XT scancode
int lastScanCode = 0;        // Used to ignore internal key repeating
int numBits = 0;             // How many bits of the scancode have been read?
int sigStart = 0;            // Used to bypass the first clock cycle of a scancode
int temp = 0;                // used to bitshift the read bit
int readCode = 0;            // 1 means we're being sent a scancode


void setup() {
  pinMode(CLK_Pin, INPUT);
  pinMode(DATA_Pin, INPUT);
  pinMode(LED_Pin, OUTPUT);    // LED is on when NumLock is enabled
}

// These handle press/release of Ctrl, Shift, Alt, and Gui keys
// We don't combine these as a single toggle (using XOR ^) because
// if the key press state between teensy and computer get out of sync,
// you would be stuck with the key state being reversed (up=down, down=up)
// (Okay, I'm paranoid)
void modKeyPress(uint8_t target) {
  modifiers = modifiers | target;
  updateModifiers();
}
void modKeyRel(uint8_t target) {
  modifiers = modifiers ^ target;
  updateModifiers();
}
void updateModifiers() {
  Keyboard.set_modifier(modifiers);
  Keyboard.send_now();
}

// Finds an open slot to place the key press signal in. (6 keys max)
// Ignores built-in key repeating.
void setOpenKey(uint8_t target) {
  for (int i = 0; i < 6; i++) {
//    if (keysHeld[i] == target)
//      break;
    if (keysHeld[i] == 0) {
      switch (i) {
        case 0:
          Keyboard.set_key1(target);
          break;
        case 1:
          Keyboard.set_key2(target);
          break;
        case 2:
          Keyboard.set_key3(target);
          break;
        case 3:
          Keyboard.set_key4(target);
          break;
        case 4:
          Keyboard.set_key5(target);
          break;
        case 5:
          Keyboard.set_key6(target);
          break;
        default:
          break;
      }
      keysHeld[i] = target;
      Keyboard.send_now();
      break;
    }
  }
}

// Clears a key from the buffer if it's there. Otherwise, ignores.
void clearKey(uint8_t target) {
  for (int i = 0; i < 6; i++) {
    if (keysHeld[i] == target) {
      switch (i) {
        case 0:
          Keyboard.set_key1(0);
          break;
        case 1:
          Keyboard.set_key2(0);
          break;
        case 2:
          Keyboard.set_key3(0);
          break;
        case 3:
          Keyboard.set_key4(0);
          break;
        case 4:
          Keyboard.set_key5(0);
          break;
        case 5:
          Keyboard.set_key6(0);
          break;
        default:
          break;
      }
      keysHeld[i] = 0;
      Keyboard.send_now();
    }
  }
}

// These handle differences between regular key presses and numlock functions
// previously, I ran all keypresses through this. However, the number of "if"
// statements makes it somewhat inefficent, and it would break the "d" key when
// NumLock was enabled. I couldn't find out why, so I just manually re-routed
// most keys to setOpenKey()
void pressKey(uint8_t target) {
  if (target == 83) { //number of keylock key
    setOpenKey(KEY_NUM_LOCK);
    numLock = numLock ^ 1;
    if (numLock == 1)
      digitalWrite(LED_Pin, HIGH);
    else
      digitalWrite(LED_Pin, LOW);
  } else {        /// HANDLING OF EXTRA NUMLOCK KEYS. F11, F12, PAUSE, PRTSC, AND SUCH
    if (numLock == 1) {
      if (target == 66)
        setOpenKey(KEY_F11);
      else if (target == 7)
        setOpenKey(KEY_F12);
      else if (target == 71)
        setOpenKey(KEY_PAUSE);
      else if (target == 85)
        setOpenKey(KEY_PRINTSCREEN);
      else if (target == 58)
        setOpenKey(KEY_INSERT);
      else if (target == 59)
        setOpenKey(KEY_DELETE);
      else if (target == 65) {             // Since there is no windows key.
        modifiers = modifiers | 8;
        updateModifiers();
      } else
      setOpenKey(target);
    } else {
      setOpenKey(target);
    }
  }
}

void releaseKey(uint8_t target) {
  if (target == 83) {
    clearKey(KEY_NUM_LOCK);
  } else {        /// HANDLING OF EXTRA NUMLOCK KEYS. F11, F12, PAUSE, PRTSC, AND SUCH
    if (numLock == 1) {
      if (target == 66)
        clearKey(KEY_F11);
      else if (target == 7)
        clearKey(KEY_F12);
      else if (target == 71)
        clearKey(KEY_PAUSE);
      else if (target == 85)
        clearKey(KEY_PRINTSCREEN);
      else if (target == 58)
        clearKey(KEY_INSERT);
      else if (target == 59)
        clearKey(KEY_DELETE);
      else if (target == 65) {             // Since there is no windows key.
        modifiers = modifiers ^ 8;
        updateModifiers();
      } else
      clearKey(target);
    } else {
      clearKey(target);
    }
  }
}
// This function translates scan codes to proper key events
// First half are key presses, second half are key releases.
// setOpenKey()/clearKey() are used for siple keypresses (optimized)
// modKeyPress()/modKeyRel() are used for ctrl,shift,alt,windows (optimized)
// pressKey()/releaseKey() are used for keys that are potentially affected by NumLock (semi-optimized. buggy?)
void handleKeyEvent(int value) {
    switch (value) {
      case 1:
        setOpenKey(KEY_ESC);
        break;
      case 2:
        setOpenKey(KEY_1);
        break;
      case 3:
        setOpenKey(KEY_2);
        break;
      case 4:
        setOpenKey(KEY_3);
        break;
      case 5:
        setOpenKey(KEY_4);
        break;
      case 6:
        setOpenKey(KEY_5);
        break;
      case 7:
        setOpenKey(KEY_6);
        break;
      case 8:
        setOpenKey(KEY_7);
        break;
      case 9:
        pressKey(KEY_8);
        break;
      case 10:
        pressKey(KEY_9);
        break;
      case 11:
        setOpenKey(KEY_0);
        break;
      case 12:
        setOpenKey(KEY_MINUS);
        break;
      case 13:
        setOpenKey(KEY_EQUAL);
        break;
      case 14:
        setOpenKey(KEY_BACKSPACE);
        break;
      case 15:
        setOpenKey(KEY_TAB);
        break;
      case 16:
        setOpenKey(KEY_Q);
        break;
      case 17:
        setOpenKey(KEY_W);
        break;
      case 18:
        setOpenKey(KEY_E);
        break;
      case 19:
        pressKey(KEY_R);
        break;
      case 20:
        setOpenKey(KEY_T);
        break;
      case 21:
        setOpenKey(KEY_Y);
        break;
      case 22:
        setOpenKey(KEY_U);
        break;
      case 23:
        setOpenKey(KEY_I);
        break;
      case 24:
        setOpenKey(KEY_O);
        break;
      case 25:
        setOpenKey(KEY_P);
        break;
      case 26:
        setOpenKey(KEY_LEFT_BRACE);
        break;
      case 27:
        setOpenKey(KEY_RIGHT_BRACE);
        break;
      case 28:
        setOpenKey(KEY_ENTER);              // technically KEYPAD_ENTER
        break;
      case 29:
        modKeyPress(MODIFIERKEY_CTRL);          // Model F CONTROL KEY IS IN STRANGE SPOT
        break;
      case 30:
        setOpenKey(KEY_A);
        break;
      case 31:
        setOpenKey(KEY_S);
        break;
      case 32:
        setOpenKey(KEY_D);
        break;
      case 33:
        setOpenKey(KEY_F);
        break;
      case 34:
        setOpenKey(KEY_G);
        break;
      case 35:
        setOpenKey(KEY_H);
        break;
      case 36:
        setOpenKey(KEY_J);
        break;
      case 37:
        setOpenKey(KEY_K);
        break;
      case 38:
        setOpenKey(KEY_L);
        break;
      case 39:
        setOpenKey(KEY_SEMICOLON);
        break;
      case 40:
        setOpenKey(KEY_QUOTE);
        break;
      case 41:
        setOpenKey(KEY_BACKSLASH);  // Actually Tilde
        break;
      case 42:
        modKeyPress(MODIFIERKEY_SHIFT);                  // Left shift
        break;
      case 43:
        setOpenKey(KEY_TILDE);  // Actually Backslash
        break;
      case 44:
        setOpenKey(KEY_Z);
        break;
      case 45:
        setOpenKey(KEY_X);
        break;
      case 46:
        setOpenKey(KEY_C);
        break;
      case 47:
        setOpenKey(KEY_V);
        break;
      case 48:
        setOpenKey(KEY_B);
        break;
      case 49:
        setOpenKey(KEY_N);
        break;
      case 50:
        setOpenKey(KEY_M);
        break;
      case 51:
        setOpenKey(KEY_COMMA);
        break;
      case 52:
        setOpenKey(KEY_PERIOD);
        break;
      case 53:
        setOpenKey(KEY_SLASH);
        break;
      case 54:
        modKeyPress(MODIFIERKEY_RIGHT_SHIFT);
        break;
      case 55:
        pressKey(KEYPAD_ASTERIX);                     // Make sure to handle NUM LOCK internally!!!!!
        break;
      case 56:
        modKeyPress(MODIFIERKEY_GUI);   // actually the Alt key
        break;
      case 57:
        setOpenKey(KEY_SPACE);
        break;
      case 58:
        modKeyPress(MODIFIERKEY_RIGHT_GUI);  // actually the Caps Lock key
        break;
      case 59:
        pressKey(KEY_F1);                        // F* Keys are handled under NumLock. Numlock off = 1-10. When on, F9=F11, F10=F12
        break;
      case 60:
        pressKey(KEY_F2);
        break;
      case 61:
        pressKey(KEY_F3);
        break;
      case 62:
        pressKey(KEY_F4);
        break;
      case 63:
        pressKey(KEY_F5);
        break;
      case 64:
        pressKey(KEY_F6);
        break;
      case 65:
        pressKey(KEY_F7);
        break;
      case 66:
        pressKey(KEY_F8);
        break;
      case 67:
        pressKey(KEY_F9);
        break;
      case 68:
        pressKey(KEY_F10);
        break;
      case 69:
        pressKey(KEY_NUM_LOCK);                      // HANDLED SEMI-INTERNALLY!
        break;
      case 70:
        pressKey(KEY_SCROLL_LOCK);
        break;
      case 71:
        pressKey(KEYPAD_7);                          // numbers are NumLock handled by OS
        break;
      case 72:
        pressKey(KEYPAD_8);
        break;
      case 73:
        pressKey(KEYPAD_9);
        break;
      case 74:
        pressKey(KEYPAD_MINUS);
        break;
      case 75:
        pressKey(KEYPAD_4);
        break;
      case 76:
        pressKey(KEYPAD_5);
        break;
      case 77:
        pressKey(KEYPAD_6);
        break;
      case 78:
        pressKey(KEYPAD_PLUS);
        break;
      case 79:
        pressKey(KEYPAD_1);
        break;
      case 80:
        pressKey(KEYPAD_2);
        break;
      case 81:
        pressKey(KEYPAD_3);
        break;
      case 82:
        modKeyPress(MODIFIERKEY_RIGHT_ALT); // actually Keypad 0
        break;
      case 83:
        pressKey(KEYPAD_PERIOD);          ///THIS IS THE LAST KEY ON THE MODEL F
        break;

      /////////////THESE ARE THE BREAK SIGNALS//////////////

      case 129:
        clearKey(KEY_ESC);
        break;
      case 130:
        clearKey(KEY_1);
        break;
      case 131:
        clearKey(KEY_2);
        break;
      case 132:
        clearKey(KEY_3);
        break;
      case 133:
        clearKey(KEY_4);
        break;
      case 134:
        clearKey(KEY_5);
        break;
      case 135:
        clearKey(KEY_6);
        break;
      case 136:
        clearKey(KEY_7);
        break;
      case 137:
        clearKey(KEY_8);
        break;
      case 138:
        clearKey(KEY_9);
        break;
      case 139:
        clearKey(KEY_0);
        break;
      case 140:
        clearKey(KEY_MINUS);
        break;
      case 141:
        clearKey(KEY_EQUAL);
        break;
      case 142:
        clearKey(KEY_BACKSPACE);
        break;
      case 143:
        clearKey(KEY_TAB);
        break;
      case 144:
        clearKey(KEY_Q);
        break;
      case 145:
        clearKey(KEY_W);
        break;
      case 146:
        clearKey(KEY_E);
        break;
      case 147:
        clearKey(KEY_R);
        break;
      case 148:
        clearKey(KEY_T);
        break;
      case 149:
        clearKey(KEY_Y);
        break;
      case 150:
        clearKey(KEY_U);
        break;
      case 151:
        clearKey(KEY_I);
        break;
      case 152:
        clearKey(KEY_O);
        break;
      case 153:
        clearKey(KEY_P);
        break;
      case 154:
        clearKey(KEY_LEFT_BRACE);
        break;
      case 155:
        clearKey(KEY_RIGHT_BRACE);
        break;
      case 156:
        clearKey(KEY_ENTER);              // This is technically KEYPAD_ENTER
        break;
      case 157:
        modKeyRel(MODIFIERKEY_CTRL);          // Model F CONTROL KEY IS IN STRANGE SPOT
        break;
      case 158:
        clearKey(KEY_A);
        break;
      case 159:
        clearKey(KEY_S);
        break;
      case 160:
        clearKey(KEY_D);
        break;
      case 161:
        clearKey(KEY_F);
        break;
      case 162:
        clearKey(KEY_G);
        break;
      case 163:
        clearKey(KEY_H);
        break;
      case 164:
        clearKey(KEY_J);
        break;
      case 165:
        clearKey(KEY_K);
        break;
      case 166:
        clearKey(KEY_L);
        break;
      case 167:
        clearKey(KEY_SEMICOLON);
        break;
      case 168:
        clearKey(KEY_QUOTE);
        break;
      case 169:
        clearKey(KEY_BACKSLASH); // Actually Tilde
        break;
      case 170:
        modKeyRel(MODIFIERKEY_SHIFT);                  // Left shift
        break;
      case 171:
        clearKey(KEY_TILDE);  // Actually Backslash
        break;
      case 172:
        clearKey(KEY_Z);
        break;
      case 173:
        clearKey(KEY_X);
        break;
      case 174:
        clearKey(KEY_C);
        break;
      case 175:
        clearKey(KEY_V);
        break;
      case 176:
        clearKey(KEY_B);
        break;
      case 177:
        clearKey(KEY_N);
        break;
      case 178:
        clearKey(KEY_M);
        break;
      case 179:
        clearKey(KEY_COMMA);
        break;
      case 180:
        clearKey(KEY_PERIOD);
        break;
      case 181:
        clearKey(KEY_SLASH);
        break;
      case 182:
        modKeyRel(MODIFIERKEY_RIGHT_SHIFT);
        break;
      case 183:
        releaseKey(KEYPAD_ASTERIX);                     // Make sure to handle NUM LOCK internally!!!!!
        break;
      case 184:
        modKeyRel(MODIFIERKEY_GUI);   // Actually the Alt key
        break;
      case 185:
        clearKey(KEY_SPACE);
        break;
      case 186:
        modKeyRel(MODIFIERKEY_RIGHT_GUI);   // Actually the Caps Lock key
        break;
      case 187:
        releaseKey(KEY_F1);                        // F* Keys are handled under NumLock. Numlock off = 1-10. When on, F9=F11, F10=F12
        break;
      case 188:
        releaseKey(KEY_F2);
        break;
      case 189:
        releaseKey(KEY_F3);
        break;
      case 190:
        releaseKey(KEY_F4);
        break;
      case 191:
        releaseKey(KEY_F5);
        break;
      case 192:
        releaseKey(KEY_F6);
        break;
      case 193:
        releaseKey(KEY_F7);
        break;
      case 194:
        releaseKey(KEY_F8);
        break;
      case 195:
        releaseKey(KEY_F9);
        break;
      case 196:
        releaseKey(KEY_F10);
        break;
      case 197:
        releaseKey(KEY_NUM_LOCK);                      // HANDLED SEMI-INTERNALLY!
        break;
      case 198:
        releaseKey(KEY_SCROLL_LOCK);
        break;
      case 199:
        releaseKey(KEYPAD_7);                          // numbers are NumLock handled by OS
        break;
      case 200:
        releaseKey(KEYPAD_8);
        break;
      case 201:
        releaseKey(KEYPAD_9);
        break;
      case 202:
        releaseKey(KEYPAD_MINUS);
        break;
      case 203:
        releaseKey(KEYPAD_4);
        break;
      case 204:
        releaseKey(KEYPAD_5);
        break;
      case 205:
        releaseKey(KEYPAD_6);
        break;
      case 206:
        releaseKey(KEYPAD_PLUS);
        break;
      case 207:
        releaseKey(KEYPAD_1);
        break;
      case 208:
        releaseKey(KEYPAD_2);
        break;
      case 209:
        releaseKey(KEYPAD_3);
        break;
      case 210:
        modKeyRel(MODIFIERKEY_RIGHT_ALT); // actually Keypad 0
        break;
      case 211:
        releaseKey(KEYPAD_PERIOD);          ///THIS IS THE LAST KEY ON THE MODEL F
        break;

      /////////SORRY, I ONLY IMPLEMENTED MODEL F XT 83 KEY SUPPORT/////////////

      default:
        break;
    }
}


void loop() {
  // This catches a LOW in clock, signaling the start of a scan code
  if (readCode == 0 && digitalRead(CLK_Pin) == 0) {
    readCode = 1;
  }

  // Note how numbits must be 9 instead of 8.
  // If we ignore the 9th pulse, it re-triggers readCode,
  // causing invalid scancodes.
  // The the 9th value should always be 0, so it
  // shouldn't change the scan code.
  if (numBits == 9) {
    if (scanCode != lastScanCode) {
      handleKeyEvent(scanCode);    // Disable this line and enable the following line
//    Keyboard.println(scanCode);  // to determine scan codes for larger keyboards

      lastScanCode = scanCode;     // This is the magic that prevents key repeating.
    }
    // Reset all after successfully sending scanCode.
    sigStart = 0;
    scanCode = 0;
    numBits = 0;
    readCode = 0;
  }

  // since the code detects the rising edge of clock,
  // we use readCode to filter out the fact that
  // clock rests HIGH.
  if (readCode == 1) {
    // On each rising edge, get the respective DATA.
    // cycleReadYet prevents multiple reads on the same cycle.
    if (digitalRead(CLK_Pin) == 1 && cycleReadYet == 0) {

      // The first clock pulse signals the start of a scan code.
      // This means it has no relevant data.
      if (sigStart == 0){
        sigStart = 1;
      } else {
        // after the first cycle, read data.
        temp = digitalRead(DATA_Pin);
        temp = temp << numBits;
        scanCode = scanCode | temp;
        numBits++;
      }
      cycleReadYet = 1;
    }
    // once the clock drops again, we can reset cycleReadYet
    // since DATA is only read when clock is HIGH.
    if (digitalRead(CLK_Pin) == 0) {
      cycleReadYet = 0;
    }
  }
}

