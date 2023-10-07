
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

#include <EEPROM.h>

// Using Keyboard HID volume codes so we can use it in the low layer set_key
// communication
const int HID_KEY_MUTE = 0x7f;
const int HID_KEY_VOLUMEUP = 0x80;
const int HID_KEY_VOLUMEDOWN = 0x81;

// I/O pins used
const int CLK_Pin = 2;   // Feel free to make this any unused digital pin (must be 5v tolerant!)
const int DATA_Pin = 3;  // Feel free to make this any unused digital pin (must be 5v tolerant!)
const int LED_Pin = 13;  // the number of the LED pin
const int BUZZ_Pin = 23; // pin for the piezo buzzer

// Buzzer config
const unsigned int BUZZ_DURATION = 100;  // duration of one buzz (millis)
const unsigned int BUZZ_INTERVAL = 500;  // interval between buzzes (millis)
const unsigned int BUZZ_FREQUENCY = 200; // frequency of buzz (Hz)

// Config for saving key press count
const unsigned int SAVE_DELAY = 5 * 60 * 1000;               // Delay after keypress after which the keypress count is saved to EEPROM (millis)
const unsigned int EEPROM_WRITE_SAFETY_THRESHOLD = 60000;    // Warn after this many writes. EEPROM memory is rated for 100,000 writes.
const int KEYPRESSES_ADDRESS = 0;                            // EEPROM address to write the keypresses data to.
const int SAVES_ADDRESS = KEYPRESSES_ADDRESS + sizeof(long); // EEPROM address to write the saves data to.

// Array tracks keys currently being held
int keysHeld[6] = {0, 0, 0, 0, 0, 0}; // 6 keys max

// global variables, track Ctrl, Shift, Alt, Gui (Windows), and NumLock key statuses.
int modifiers = 0; // 1=c, 2=s, 3=sc, 4=a, 5=ac, 6=as, 7=asc, 8=g, 9=gc...

// variables utilized in main loop for reading data
int cycleReadYet = 0; // was data read for this cycle?
int scanCode = 0;     // Raw XT scancode
int lastScanCode = 0; // Used to ignore internal key repeating
int numBits = 0;      // How many bits of the scancode have been read?
int sigStart = 0;     // Used to bypass the first clock cycle of a scancode
int temp = 0;         // used to bitshift the read bit
int readCode = 0;     // 1 means we're being sent a scancode

unsigned long buzzingNumber = 0;
elapsedMillis sinceToneStop = BUZZ_INTERVAL + 1;
elapsedMillis sinceTone = sinceToneStop + 10000; // we pretend that tone has stopped rather than started to initialise buzzing correctly

unsigned int savePending = 0; // whether a keypress count save to EEPROM is scheduled
elapsedMillis sinceSaveScheduled = 0;

unsigned long keyPresses = 0;
unsigned int saves = 0;

elapsedMillis sinceLiveReport = 0;

void setup()
{
  Serial.begin(9600);
  Serial.println("hello from jewel");

  pinMode(CLK_Pin, INPUT);
  pinMode(DATA_Pin, INPUT);
  pinMode(LED_Pin, OUTPUT);

  EEPROM.get(KEYPRESSES_ADDRESS, keyPresses);
  EEPROM.get(SAVES_ADDRESS, saves);

  Serial.printf("retrieved stats: %d keypresses and %d saves\n", keyPresses, saves);

  digitalWrite(LED_Pin, HIGH);

  Serial.printf("set LED to on\n");
  Serial.printf("jewel is operational\n");
}

void saveKeyPress()
{
  keyPresses++;
  savePending = 1;
  sinceSaveScheduled = 0;
}

void buzzNumber(unsigned long n)
{
  if (n == 0)
  {
    return;
  }
  n = (n << 1) + 1; // save the end of the bitstring
  while ((n & 0x80000000ul) == 0)
  {
    n = n << 1;
  }
  buzzingNumber = n;
}

// This function translates scan codes to proper key events
// First half are key presses, second half are key releases.
// setOpenKey()/clearKey() are used for siple keypresses (optimized)
// modKeyPress()/modKeyRel() are used for ctrl,shift,alt,windows (optimized)
void handleKeyEvent(int value)
{
  switch (value)
  {
  case 1:
    Keyboard.press(KEY_ESC);
    break;
  case 2:
    Keyboard.press(KEY_1);
    break;
  case 3:
    Keyboard.press(KEY_2);
    break;
  case 4:
    Keyboard.press(KEY_3);
    break;
  case 5:
    Keyboard.press(KEY_4);
    break;
  case 6:
    Keyboard.press(KEY_5);
    break;
  case 7:
    Keyboard.press(KEY_6);
    break;
  case 8:
    Keyboard.press(KEY_7);
    break;
  case 9:
    Keyboard.press(KEY_8);
    break;
  case 10:
    Keyboard.press(KEY_9);
    break;
  case 11:
    Keyboard.press(KEY_0);
    break;
  case 12:
    Keyboard.press(KEY_MINUS);
    break;
  case 13:
    Keyboard.press(KEY_EQUAL);
    break;
  case 14:
    Keyboard.press(KEY_BACKSPACE);
    break;
  case 15:
    Keyboard.press(KEY_TAB);
    break;
  case 16:
    Keyboard.press(KEY_Q);
    break;
  case 17:
    Keyboard.press(KEY_W);
    break;
  case 18:
    Keyboard.press(KEY_E);
    break;
  case 19:
    Keyboard.press(KEY_R);
    break;
  case 20:
    Keyboard.press(KEY_T);
    break;
  case 21:
    Keyboard.press(KEY_Y);
    break;
  case 22:
    Keyboard.press(KEY_U);
    break;
  case 23:
    Keyboard.press(KEY_I);
    break;
  case 24:
    Keyboard.press(KEY_O);
    break;
  case 25:
    Keyboard.press(KEY_P);
    break;
  case 26:
    Keyboard.press(KEY_LEFT_BRACE);
    break;
  case 27:
    Keyboard.press(KEY_RIGHT_BRACE);
    break;
  case 28:
    Keyboard.press(KEY_ENTER); // technically KEYPAD_ENTER
    break;
  case 29:
    Keyboard.press(MODIFIERKEY_CTRL); // Model F CONTROL KEY IS IN STRANGE SPOT
    break;
  case 30:
    Keyboard.press(KEY_A);
    break;
  case 31:
    Keyboard.press(KEY_S);
    break;
  case 32:
    Keyboard.press(KEY_D);
    break;
  case 33:
    Keyboard.press(KEY_F);
    break;
  case 34:
    Keyboard.press(KEY_G);
    break;
  case 35:
    Keyboard.press(KEY_H);
    break;
  case 36:
    Keyboard.press(KEY_J);
    break;
  case 37:
    Keyboard.press(KEY_K);
    break;
  case 38:
    Keyboard.press(KEY_L);
    break;
  case 39:
    Keyboard.press(KEY_SEMICOLON);
    break;
  case 40:
    Keyboard.press(KEY_QUOTE);
    break;
  case 41:
    Keyboard.press(KEY_BACKSLASH); // Actually Tilde
    break;
  case 42:
    Keyboard.press(MODIFIERKEY_SHIFT); // Left shift
    break;
  case 43:
    Keyboard.press(KEY_TILDE); // Actually Backslash
    break;
  case 44:
    Keyboard.press(KEY_Z);
    break;
  case 45:
    Keyboard.press(KEY_X);
    break;
  case 46:
    Keyboard.press(KEY_C);
    break;
  case 47:
    Keyboard.press(KEY_V);
    break;
  case 48:
    Keyboard.press(KEY_B);
    break;
  case 49:
    Keyboard.press(KEY_N);
    break;
  case 50:
    Keyboard.press(KEY_M);
    break;
  case 51:
    Keyboard.press(KEY_COMMA);
    break;
  case 52:
    Keyboard.press(KEY_PERIOD);
    break;
  case 53:
    Keyboard.press(KEY_SLASH);
    break;
  case 54:
    Keyboard.press(MODIFIERKEY_RIGHT_SHIFT);
    break;
  case 55:
    Keyboard.press(KEYPAD_ASTERIX); // Make sure to handle NUM LOCK internally!!!!!
    break;
  case 56:
    Keyboard.press(MODIFIERKEY_GUI); // actually the Alt key
    break;
  case 57:
    Keyboard.press(KEY_SPACE);
    break;
  case 58:
    Keyboard.press(MODIFIERKEY_RIGHT_GUI); // actually the Caps Lock key
    break;
  case 59:
    Keyboard.press(KEY_F1);
    break;
  case 60:
    Keyboard.press(KEY_F2);
    break;
  case 61:
    // This causes Brightness Down on macOS
    Keyboard.press(KEY_SCROLL_LOCK);
    break;
  case 62:
    // This causes Brightness Up on macOS
    Keyboard.press(KEY_PAUSE);
    break;
  case 63:
    Keyboard.press(KEY_MEDIA_PLAY_PAUSE);
    break;
  case 64:
    Keyboard.press(HID_KEY_MUTE);
    break;
  case 65:
    Keyboard.press(KEY_MEDIA_REWIND);
    break;
  case 66:
    Keyboard.press(KEY_MEDIA_FAST_FORWARD);
    break;
  case 67:
    Keyboard.press(HID_KEY_VOLUMEDOWN);
    break;
  case 68:
    Keyboard.press(HID_KEY_VOLUMEUP);
    break;
  case 69:
    Keyboard.press(KEY_NUM_LOCK); // HANDLED SEMI-INTERNALLY!
    break;
  case 70: // Scroll Lock
    buzzNumber(keyPresses);
    break;
  case 71:
    Keyboard.press(KEY_HOME); // actually Keypad 7
    break;
  case 72:
    Keyboard.press(KEY_UP); // actually Keypad 8
    break;
  case 73:
    Keyboard.press(KEY_PAGE_UP); // actually Keypad 9
    break;
  case 74:
    Keyboard.press(KEYPAD_MINUS);
    break;
  case 75:
    Keyboard.press(KEY_LEFT); // Actually Keypad 4
    break;
  case 76:
    Keyboard.press(KEY_DOWN); // Actually Keypad 5
    break;
  case 77:
    Keyboard.press(KEY_RIGHT); // Actually Keypad 6
    break;
  case 78:
    Keyboard.press(KEYPAD_PLUS);
    break;
  case 79:
    Keyboard.press(KEY_END); // Actually Keypad 1
    break;
  case 80:
    Keyboard.press(KEY_DOWN); // Actually Keypad 2
    break;
  case 81:
    Keyboard.press(KEY_PAGE_DOWN); // Actually Keypad 3
    break;
  case 82:
    Keyboard.press(MODIFIERKEY_RIGHT_ALT); // actually Keypad 0
    break;
  case 83:
    Keyboard.press(KEY_DELETE); // Actually Keypad Period
    break;

    /////////////THESE ARE THE BREAK SIGNALS//////////////

  case 129:
    Keyboard.release(KEY_ESC);
    break;
  case 130:
    Keyboard.release(KEY_1);
    break;
  case 131:
    Keyboard.release(KEY_2);
    break;
  case 132:
    Keyboard.release(KEY_3);
    break;
  case 133:
    Keyboard.release(KEY_4);
    break;
  case 134:
    Keyboard.release(KEY_5);
    break;
  case 135:
    Keyboard.release(KEY_6);
    break;
  case 136:
    Keyboard.release(KEY_7);
    break;
  case 137:
    Keyboard.release(KEY_8);
    break;
  case 138:
    Keyboard.release(KEY_9);
    break;
  case 139:
    Keyboard.release(KEY_0);
    break;
  case 140:
    Keyboard.release(KEY_MINUS);
    break;
  case 141:
    Keyboard.release(KEY_EQUAL);
    break;
  case 142:
    Keyboard.release(KEY_BACKSPACE);
    break;
  case 143:
    Keyboard.release(KEY_TAB);
    break;
  case 144:
    Keyboard.release(KEY_Q);
    break;
  case 145:
    Keyboard.release(KEY_W);
    break;
  case 146:
    Keyboard.release(KEY_E);
    break;
  case 147:
    Keyboard.release(KEY_R);
    break;
  case 148:
    Keyboard.release(KEY_T);
    break;
  case 149:
    Keyboard.release(KEY_Y);
    break;
  case 150:
    Keyboard.release(KEY_U);
    break;
  case 151:
    Keyboard.release(KEY_I);
    break;
  case 152:
    Keyboard.release(KEY_O);
    break;
  case 153:
    Keyboard.release(KEY_P);
    break;
  case 154:
    Keyboard.release(KEY_LEFT_BRACE);
    break;
  case 155:
    Keyboard.release(KEY_RIGHT_BRACE);
    break;
  case 156:
    Keyboard.release(KEY_ENTER); // This is technically KEYPAD_ENTER
    break;
  case 157:
    Keyboard.release(MODIFIERKEY_CTRL); // Model F CONTROL KEY IS IN STRANGE SPOT
    break;
  case 158:
    Keyboard.release(KEY_A);
    break;
  case 159:
    Keyboard.release(KEY_S);
    break;
  case 160:
    Keyboard.release(KEY_D);
    break;
  case 161:
    Keyboard.release(KEY_F);
    break;
  case 162:
    Keyboard.release(KEY_G);
    break;
  case 163:
    Keyboard.release(KEY_H);
    break;
  case 164:
    Keyboard.release(KEY_J);
    break;
  case 165:
    Keyboard.release(KEY_K);
    break;
  case 166:
    Keyboard.release(KEY_L);
    break;
  case 167:
    Keyboard.release(KEY_SEMICOLON);
    break;
  case 168:
    Keyboard.release(KEY_QUOTE);
    break;
  case 169:
    Keyboard.release(KEY_BACKSLASH); // Actually Tilde
    break;
  case 170:
    Keyboard.release(MODIFIERKEY_SHIFT); // Left shift
    break;
  case 171:
    Keyboard.release(KEY_TILDE); // Actually Backslash
    break;
  case 172:
    Keyboard.release(KEY_Z);
    break;
  case 173:
    Keyboard.release(KEY_X);
    break;
  case 174:
    Keyboard.release(KEY_C);
    break;
  case 175:
    Keyboard.release(KEY_V);
    break;
  case 176:
    Keyboard.release(KEY_B);
    break;
  case 177:
    Keyboard.release(KEY_N);
    break;
  case 178:
    Keyboard.release(KEY_M);
    break;
  case 179:
    Keyboard.release(KEY_COMMA);
    break;
  case 180:
    Keyboard.release(KEY_PERIOD);
    break;
  case 181:
    Keyboard.release(KEY_SLASH);
    break;
  case 182:
    Keyboard.release(MODIFIERKEY_RIGHT_SHIFT);
    break;
  case 183:
    Keyboard.release(KEYPAD_ASTERIX); // Make sure to handle NUM LOCK internally!!!!!
    break;
  case 184:
    Keyboard.release(MODIFIERKEY_GUI); // Actually the Alt key
    break;
  case 185:
    Keyboard.release(KEY_SPACE);
    break;
  case 186:
    Keyboard.release(MODIFIERKEY_RIGHT_GUI); // Actually the Caps Lock key
    break;
  case 187:
    Keyboard.release(KEY_F1);
    break;
  case 188:
    Keyboard.release(KEY_F2);
    break;
  case 189:
    Keyboard.release(KEY_SCROLL_LOCK);
    break;
  case 190:
    Keyboard.release(KEY_PAUSE);
    break;
  case 191:
    Keyboard.release(KEY_MEDIA_PLAY_PAUSE);
    break;
  case 192:
    Keyboard.release(HID_KEY_MUTE);
    break;
  case 193:
    Keyboard.release(KEY_MEDIA_REWIND);
    break;
  case 194:
    Keyboard.release(KEY_MEDIA_FAST_FORWARD);
    break;
  case 195:
    Keyboard.release(HID_KEY_VOLUMEDOWN);
    break;
  case 196:
    Keyboard.release(HID_KEY_VOLUMEUP);
    break;
  case 197:
    Keyboard.release(KEY_NUM_LOCK); // HANDLED SEMI-INTERNALLY!
    break;
  case 198: // Scroll Lock
    break;
  case 199:
    Keyboard.release(KEY_HOME); // Actually Keypad 7
    break;
  case 200:
    Keyboard.release(KEY_UP); // Actually Keypad 8
    break;
  case 201:
    Keyboard.release(KEY_PAGE_UP); // Actually Keypad 9
    break;
  case 202:
    Keyboard.release(KEYPAD_MINUS);
    break;
  case 203:
    Keyboard.release(KEY_LEFT); // Actually Keypad 4
    break;
  case 204:
    Keyboard.release(KEY_DOWN); // Actually Keypad 5
    break;
  case 205:
    Keyboard.release(KEY_RIGHT); // Actually Keypad 6
    break;
  case 206:
    Keyboard.release(KEYPAD_PLUS);
    break;
  case 207:
    Keyboard.release(KEY_END); // Actually Keypad 1
    break;
  case 208:
    Keyboard.release(KEY_DOWN); // Actually Keypad 2
    break;
  case 209:
    Keyboard.release(KEY_PAGE_DOWN); // Actually Keypad 3
    break;
  case 210:
    Keyboard.release(MODIFIERKEY_RIGHT_ALT); // actually Keypad 0
    break;
  case 211:
    Keyboard.release(KEY_DELETE); // actually Keypad Period
    break;

  default:
    break;
  }
}

void loop()
{
  // Serial.printf("%d\n", digitalRead(CLK_Pin));
  // This catches a LOW in clock, signaling the start of a scan code
  if (readCode == 0 && digitalRead(CLK_Pin) == 0)
  {
    readCode = 1;
  }

  // Note how numbits must be 9 instead of 8.
  // If we ignore the 9th pulse, it re-triggers readCode,
  // causing invalid scancodes.
  // The the 9th value should always be 0, so it
  // shouldn't change the scan code.
  if (numBits == 9)
  {
    if (scanCode != lastScanCode)
    {
      handleKeyEvent(scanCode); // Disable this line and enable the following line
                                //    Keyboard.println(scanCode);  // to determine scan codes for larger keyboards

      lastScanCode = scanCode; // This is the magic that prevents key repeating.
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
  if (readCode == 1)
  {
    // On each rising edge, get the respective DATA.
    // cycleReadYet prevents multiple reads on the same cycle.
    if (digitalRead(CLK_Pin) == 1 && cycleReadYet == 0)
    {

      // The first clock pulse signals the start of a scan code.
      // This means it has no relevant data.
      if (sigStart == 0)
      {
        sigStart = 1;
      }
      else
      {
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
    if (digitalRead(CLK_Pin) == 0)
    {
      cycleReadYet = 0;
    }
  }

  if (buzzingNumber != 0x80000000ul && buzzingNumber != 0)
  {
    if (sinceTone < sinceToneStop && sinceTone > BUZZ_DURATION)
    {
      noTone(BUZZ_Pin);
      sinceToneStop = 0;
      buzzingNumber = buzzingNumber << 1;
    }
    else if (sinceToneStop < sinceTone && sinceToneStop > BUZZ_INTERVAL)
    {
      int value = ((buzzingNumber & 0x80000000ul) > 0) ? 4 : 1;
      tone(BUZZ_Pin, value * BUZZ_FREQUENCY);
      sinceTone = 0;
    }
  }
  else if (buzzingNumber == 0x80000000ul)
  {
    buzzingNumber = 0;
  }

  if (savePending && sinceSaveScheduled > SAVE_DELAY)
  {
    saves++;
    EEPROM.put(KEYPRESSES_ADDRESS, keyPresses);
    EEPROM.put(SAVES_ADDRESS, saves);
    // We want to warn when we start writing more than the safety threshold.
    if (saves > EEPROM_WRITE_SAFETY_THRESHOLD)
    {
      buzzNumber(5); // 101 in binary, sounds like SOS
    }
    savePending = 0;
  }

  if (sinceLiveReport > 5000)
  {
    Serial.printf("/running for %d seconds/\n", millis() / 1000);
    sinceLiveReport = 0;
  }
}
