
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
#include <stdbool.h>

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

bool isPanicking = false;

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

  // Esc
  case 1:
    Keyboard.press(KEY_ESC);
    break;
  case 129:
    Keyboard.release(KEY_ESC);
    break;

  // 1
  case 2:
    Keyboard.press(KEY_1);
    break;
  case 130:
    Keyboard.release(KEY_1);
    break;

  // 2
  case 3:
    Keyboard.press(KEY_2);
    break;
  case 131:
    Keyboard.release(KEY_2);
    break;

  // 3
  case 4:
    Keyboard.press(KEY_3);
    break;
  case 132:
    Keyboard.release(KEY_3);
    break;

  // 4
  case 5:
    Keyboard.press(KEY_4);
    break;
  case 133:
    Keyboard.release(KEY_4);
    break;

  // 5
  case 6:
    Keyboard.press(KEY_5);
    break;
  case 134:
    Keyboard.release(KEY_5);
    break;

  // 6
  case 7:
    Keyboard.press(KEY_6);
    break;
  case 135:
    Keyboard.release(KEY_6);
    break;

  // 7
  case 8:
    Keyboard.press(KEY_7);
    break;
  case 136:
    Keyboard.release(KEY_7);
    break;

  // 8
  case 9:
    Keyboard.press(KEY_8);
    break;
  case 137:
    Keyboard.release(KEY_8);
    break;

  // 9
  case 10:
    Keyboard.press(KEY_9);
    break;
  case 138:
    Keyboard.release(KEY_9);
    break;

  // 0
  case 11:
    Keyboard.press(KEY_0);
    break;
  case 139:
    Keyboard.release(KEY_0);
    break;

  // -
  case 12:
    Keyboard.press(KEY_MINUS);
    break;
  case 140:
    Keyboard.release(KEY_MINUS);
    break;

  // =
  case 13:
    Keyboard.press(KEY_EQUAL);
    break;
  case 141:
    Keyboard.release(KEY_EQUAL);
    break;

  // Backspace
  case 14:
    Keyboard.press(KEY_BACKSPACE);
    break;
  case 142:
    Keyboard.release(KEY_BACKSPACE);
    break;

  // Tab
  case 15:
    Keyboard.press(KEY_TAB);
    break;
  case 143:
    Keyboard.release(KEY_TAB);
    break;

  // Q
  case 16:
    Keyboard.press(KEY_Q);
    break;
  case 144:
    Keyboard.release(KEY_Q);
    break;

  // W
  case 17:
    Keyboard.press(KEY_W);
    break;
  case 145:
    Keyboard.release(KEY_W);
    break;

  // E
  case 18:
    Keyboard.press(KEY_E);
    break;
  case 146:
    Keyboard.release(KEY_E);
    break;

  // R
  case 19:
    Keyboard.press(KEY_R);
    break;
  case 147:
    Keyboard.release(KEY_R);
    break;

  // T
  case 20:
    Keyboard.press(KEY_T);
    break;
  case 148:
    Keyboard.release(KEY_T);
    break;

  // Y
  case 21:
    Keyboard.press(KEY_Y);
    break;
  case 149:
    Keyboard.release(KEY_Y);
    break;

  // U
  case 22:
    Keyboard.press(KEY_U);
    break;
  case 150:
    Keyboard.release(KEY_U);
    break;

  // I
  case 23:
    Keyboard.press(KEY_I);
    break;
  case 151:
    Keyboard.release(KEY_I);
    break;

  // O
  case 24:
    Keyboard.press(KEY_O);
    break;
  case 152:
    Keyboard.release(KEY_O);
    break;

  // P
  case 25:
    Keyboard.press(KEY_P);
    break;
  case 153:
    Keyboard.release(KEY_P);
    break;

  // {
  case 26:
    Keyboard.press(KEY_LEFT_BRACE);
    break;
  case 154:
    Keyboard.release(KEY_LEFT_BRACE);
    break;

  // }
  case 27:
    Keyboard.press(KEY_RIGHT_BRACE);
    break;
  case 155:
    Keyboard.release(KEY_RIGHT_BRACE);
    break;

  // Enter
  case 28:
    Keyboard.press(KEY_ENTER);
    break;
  case 156:
    Keyboard.release(KEY_ENTER);
    break;

  // Ctrl
  case 29:
    Keyboard.press(MODIFIERKEY_CTRL);
    break;
  case 157:
    Keyboard.release(MODIFIERKEY_CTRL);
    break;

  // A
  case 30:
    Keyboard.press(KEY_A);
    break;
  case 158:
    Keyboard.release(KEY_A);
    break;

  // S
  case 31:
    Keyboard.press(KEY_S);
    break;
  case 159:
    Keyboard.release(KEY_S);
    break;

  // D
  case 32:
    Keyboard.press(KEY_D);
    break;
  case 160:
    Keyboard.release(KEY_D);
    break;

  // F
  case 33:
    Keyboard.press(KEY_F);
    break;
  case 161:
    Keyboard.release(KEY_F);
    break;

  // G
  case 34:
    Keyboard.press(KEY_G);
    break;
  case 162:
    Keyboard.release(KEY_G);
    break;

  // H
  case 35:
    Keyboard.press(KEY_H);
    break;
  case 163:
    Keyboard.release(KEY_H);
    break;

  // J
  case 36:
    Keyboard.press(KEY_J);
    break;
  case 164:
    Keyboard.release(KEY_J);
    break;

  // K
  case 37:
    Keyboard.press(KEY_K);
    break;
  case 165:
    Keyboard.release(KEY_K);
    break;

  // L
  case 38:
    Keyboard.press(KEY_L);
    break;
  case 166:
    Keyboard.release(KEY_L);
    break;

  // ;
  case 39:
    Keyboard.press(KEY_SEMICOLON);
    break;
  case 167:
    Keyboard.release(KEY_SEMICOLON);
    break;

  // '
  case 40:
    Keyboard.press(KEY_QUOTE);
    break;
  case 168:
    Keyboard.release(KEY_QUOTE);
    break;

  // Backslash (originally ~)
  case 41:
    Keyboard.press(KEY_BACKSLASH);
    break;
  case 169:
    Keyboard.release(KEY_BACKSLASH);
    break;

  // Left Shift
  case 42:
    Keyboard.press(MODIFIERKEY_SHIFT);
    break;
  case 170:
    Keyboard.release(MODIFIERKEY_SHIFT);
    break;

  // ~ (originally Backslash)
  case 43:
    Keyboard.press(KEY_TILDE);
    break;
  case 171:
    Keyboard.release(KEY_TILDE);
    break;

  // Z
  case 44:
    Keyboard.press(KEY_Z);
    break;
  case 172:
    Keyboard.release(KEY_Z);
    break;

  // X
  case 45:
    Keyboard.press(KEY_X);
    break;
  case 173:
    Keyboard.release(KEY_X);
    break;

  // C
  case 46:
    Keyboard.press(KEY_C);
    break;
  case 174:
    Keyboard.release(KEY_C);
    break;

  // V
  case 47:
    Keyboard.press(KEY_V);
    break;
  case 175:
    Keyboard.release(KEY_V);
    break;

  // B
  case 48:
    Keyboard.press(KEY_B);
    break;
  case 176:
    Keyboard.release(KEY_B);
    break;

  // N
  case 49:
    Keyboard.press(KEY_N);
    break;
  case 177:
    Keyboard.release(KEY_N);
    break;

  // M
  case 50:
    Keyboard.press(KEY_M);
    break;
  case 178:
    Keyboard.release(KEY_M);
    break;

  // ,
  case 51:
    Keyboard.press(KEY_COMMA);
    break;
  case 179:
    Keyboard.release(KEY_COMMA);
    break;

  // .
  case 52:
    Keyboard.press(KEY_PERIOD);
    break;
  case 180:
    Keyboard.release(KEY_PERIOD);
    break;

  // /
  case 53:
    Keyboard.press(KEY_SLASH);
    break;
  case 181:
    Keyboard.release(KEY_SLASH);
    break;

  // Right Shift
  case 54:
    Keyboard.press(MODIFIERKEY_RIGHT_SHIFT);
    break;
  case 182:
    Keyboard.release(MODIFIERKEY_RIGHT_SHIFT);
    break;

  // PANIC (originally *)
  case 55:
    Keyboard.print("AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA");
    break;
  case 183:
    break;

  // Alt
  case 56:
    Keyboard.press(MODIFIERKEY_GUI);
    break;
  case 184:
    Keyboard.release(MODIFIERKEY_GUI);
    break;

  // Space
  case 57:
    Keyboard.press(KEY_SPACE);
    break;
  case 185:
    Keyboard.release(KEY_SPACE);
    break;

  // Caps Lock
  case 58:
    Keyboard.press(MODIFIERKEY_RIGHT_GUI);
    break;
  case 186:
    Keyboard.release(MODIFIERKEY_RIGHT_GUI);
    break;

  // Empty 1 (function block top left) (originally F1)
  case 59:
    Keyboard.press(KEY_F1);
    break;
  case 187:
    Keyboard.release(KEY_F1);
    break;

  // Empty 2 (function block top right) (originally F2)
  case 60:
    Keyboard.press(KEY_F2);
    break;
  case 188:
    Keyboard.release(KEY_F2);
    break;

  // Brightness Down (originally F3)
  case 61:
    // This causes Brightness Down on macOS
    Keyboard.press(KEY_SCROLL_LOCK);
    break;
  case 189:
    Keyboard.release(KEY_SCROLL_LOCK);
    break;

  // Brightness Up (originally F4)
  case 62:
    // This causes Brightness Up on macOS
    Keyboard.press(KEY_PAUSE);
    break;
  case 190:
    Keyboard.release(KEY_PAUSE);
    break;

  // Play Pause (originally F5)
  case 63:
    Keyboard.press(KEY_MEDIA_PLAY_PAUSE);
    break;
  case 191:
    Keyboard.release(KEY_MEDIA_PLAY_PAUSE);
    break;

  // Mute (originally F6)
  case 64:
    Keyboard.press(HID_KEY_MUTE);
    break;
  case 192:
    Keyboard.release(HID_KEY_MUTE);
    break;

  // Rewind (originally F7)
  case 65:
    Keyboard.press(KEY_MEDIA_REWIND);
    break;
  case 193:
    Keyboard.release(KEY_MEDIA_REWIND);
    break;

  // Fast Forward (originally F8)
  case 66:
    Keyboard.press(KEY_MEDIA_FAST_FORWARD);
    break;
  case 194:
    Keyboard.release(KEY_MEDIA_FAST_FORWARD);
    break;

  // Volume Down (originally F9)
  case 67:
    Keyboard.press(HID_KEY_VOLUMEDOWN);
    break;
  case 195:
    Keyboard.release(HID_KEY_VOLUMEDOWN);
    break;

  // Volume Up (originally F10)
  case 68:
    Keyboard.press(HID_KEY_VOLUMEUP);
    break;
  case 196:
    Keyboard.release(HID_KEY_VOLUMEUP);
    break;

  // Num Lock
  case 69:
    Keyboard.press(KEY_NUM_LOCK);
    break;
  case 197:
    Keyboard.release(KEY_NUM_LOCK);
    break;

  // Scroll Lock
  case 70:
    buzzNumber(keyPresses);
    break;
  case 198:
    break;

  // Keypad 7
  case 71:
    Keyboard.press(KEY_HOME);
    break;
  case 199:
    Keyboard.release(KEY_HOME);
    break;

  // Keypad 8
  case 72:
    Keyboard.press(KEY_UP);
    break;
  case 200:
    Keyboard.release(KEY_UP);
    break;

  // Keypad 9
  case 73:
    Keyboard.press(KEY_PAGE_UP);
    break;
  case 201:
    Keyboard.release(KEY_PAGE_UP);
    break;

  // Keypad -
  case 74:
    Keyboard.press(KEYPAD_MINUS);
    break;
  case 202:
    Keyboard.release(KEYPAD_MINUS);
    break;

  // Keypad 4
  case 75:
    Keyboard.press(KEY_LEFT);
    break;
  case 203:
    Keyboard.release(KEY_LEFT);
    break;

  // Keypad 5
  case 76:
    Keyboard.press(KEY_DOWN);
    break;
  case 204:
    Keyboard.release(KEY_DOWN);
    break;

  // Keypad 6
  case 77:
    Keyboard.press(KEY_RIGHT);
    break;
  case 205:
    Keyboard.release(KEY_RIGHT);
    break;

  // Keypad +
  case 78:
    Keyboard.press(KEYPAD_PLUS);
    break;
  case 206:
    Keyboard.release(KEYPAD_PLUS);
    break;

  // Keypad 1
  case 79:
    Keyboard.press(KEY_END);
    break;
  case 207:
    Keyboard.release(KEY_END);
    break;

  // Keypad 2
  case 80:
    Keyboard.press(KEY_DOWN);
    break;
  case 208:
    Keyboard.release(KEY_DOWN);
    break;

  // Keypad 3
  case 81:
    Keyboard.press(KEY_PAGE_DOWN);
    break;
  case 209:
    Keyboard.release(KEY_PAGE_DOWN);

  // Keypad 0
  case 82:
    Keyboard.press(MODIFIERKEY_RIGHT_ALT);
    break;
  case 210:
    Keyboard.release(MODIFIERKEY_RIGHT_ALT);
    break;

  // Keypad .
  case 83:
    Keyboard.press(KEY_DELETE);
    break;
  case 211:
    Keyboard.release(KEY_DELETE);
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
