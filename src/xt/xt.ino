
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

// Define BYTE() to be able to use binary notation
#define B_0000 0
#define B_0001 1
#define B_0010 2
#define B_0011 3
#define B_0100 4
#define B_0101 5
#define B_0110 6
#define B_0111 7
#define B_1000 8
#define B_1001 9
#define B_1010 a
#define B_1011 b
#define B_1100 c
#define B_1101 d
#define B_1110 e
#define B_1111 f
#define _B2H(bits) B_##bits
#define B2H(bits) _B2H(bits)
#define _HEX(n) 0x##n
#define HHEX(n) _HEX(n)
#define _CCAT(a, b) a##b
#define CCAT(a, b) _CCAT(a, b)
#define BYTE(a, b) HHEX(CCAT(B2H(a), B2H(b)))

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
bool cycleReadYet = false;      // was data read for this cycle?
unsigned char scanCode = 0;     // Raw XT scancode
unsigned char lastScanCode = 0; // Used to ignore internal key repeating
int numBits = 0;                // How many bits of the scancode have been read?
int sigStart = 0;               // Used to bypass the first clock cycle of a scancode
int temp = 0;                   // used to bitshift the read bit
bool readCode = false;          // 1 means we're being sent a scancode

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

  // Esc
  case BYTE(0000, 0001):
    Keyboard.press(KEY_ESC);
    break;
  case BYTE(1000, 0001):
    Keyboard.release(KEY_ESC);
    break;

  // 1
  case BYTE(0000, 0010):
    Keyboard.press(KEY_1);
    break;
  case BYTE(1000, 0010):
    Keyboard.release(KEY_1);
    break;

  // 2
  case BYTE(0000, 0011):
    Keyboard.press(KEY_2);
    break;
  case BYTE(1000, 0011):
    Keyboard.release(KEY_2);
    break;

  // 3
  case BYTE(0000, 0100):
    Keyboard.press(KEY_3);
    break;
  case BYTE(1000, 0100):
    Keyboard.release(KEY_3);
    break;

  // 4
  case BYTE(0000, 0101):
    Keyboard.press(KEY_4);
    break;
  case BYTE(1000, 0101):
    Keyboard.release(KEY_4);
    break;

  // 5
  case BYTE(0000, 0110):
    Keyboard.press(KEY_5);
    break;
  case BYTE(1000, 0110):
    Keyboard.release(KEY_5);
    break;

  // 6
  case BYTE(0000, 0111):
    Keyboard.press(KEY_6);
    break;
  case BYTE(1000, 0111):
    Keyboard.release(KEY_6);
    break;

  // 7
  case BYTE(0000, 1000):
    Keyboard.press(KEY_7);
    break;
  case BYTE(1000, 1000):
    Keyboard.release(KEY_7);
    break;

  // 8
  case BYTE(0000, 1001):
    Keyboard.press(KEY_8);
    break;
  case BYTE(1000, 1001):
    Keyboard.release(KEY_8);
    break;

  // 9
  case BYTE(0000, 1010):
    Keyboard.press(KEY_9);
    break;
  case BYTE(1000, 1010):
    Keyboard.release(KEY_9);
    break;

  // 0
  case BYTE(0000, 1011):
    Keyboard.press(KEY_0);
    break;
  case BYTE(1000, 1011):
    Keyboard.release(KEY_0);
    break;

  // -
  case BYTE(0000, 1100):
    Keyboard.press(KEY_MINUS);
    break;
  case BYTE(1000, 1100):
    Keyboard.release(KEY_MINUS);
    break;

  // =
  case BYTE(0000, 1101):
    Keyboard.press(KEY_EQUAL);
    break;
  case BYTE(1000, 1101):
    Keyboard.release(KEY_EQUAL);
    break;

  // Backspace
  case BYTE(0000, 1110):
    Keyboard.press(KEY_BACKSPACE);
    break;
  case BYTE(1000, 1110):
    Keyboard.release(KEY_BACKSPACE);
    break;

  // Tab
  case BYTE(0000, 1111):
    Keyboard.press(KEY_TAB);
    break;
  case BYTE(1000, 1111):
    Keyboard.release(KEY_TAB);
    break;

  // Q
  case BYTE(0001, 0000):
    Keyboard.press(KEY_Q);
    break;
  case BYTE(1001, 0000):
    Keyboard.release(KEY_Q);
    break;

  // W
  case BYTE(0001, 0001):
    Keyboard.press(KEY_W);
    break;
  case BYTE(1001, 0001):
    Keyboard.release(KEY_W);
    break;

  // E
  case BYTE(0001, 0010):
    Keyboard.press(KEY_E);
    break;
  case BYTE(1001, 0010):
    Keyboard.release(KEY_E);
    break;

  // R
  case BYTE(0001, 0011):
    Keyboard.press(KEY_R);
    break;
  case BYTE(1001, 0011):
    Keyboard.release(KEY_R);
    break;

  // T
  case BYTE(0001, 0100):
    Keyboard.press(KEY_T);
    break;
  case BYTE(1001, 0100):
    Keyboard.release(KEY_T);
    break;

  // Y
  case BYTE(0001, 0101):
    Keyboard.press(KEY_Y);
    break;
  case BYTE(1001, 0101):
    Keyboard.release(KEY_Y);
    break;

  // U
  case BYTE(0001, 0110):
    Keyboard.press(KEY_U);
    break;
  case BYTE(1001, 0110):
    Keyboard.release(KEY_U);
    break;

  // I
  case BYTE(0001, 0111):
    Keyboard.press(KEY_I);
    break;
  case BYTE(1001, 0111):
    Keyboard.release(KEY_I);
    break;

  // O
  case BYTE(0001, 1000):
    Keyboard.press(KEY_O);
    break;
  case BYTE(1001, 1000):
    Keyboard.release(KEY_O);
    break;

  // P
  case BYTE(0001, 1001):
    Keyboard.press(KEY_P);
    break;
  case BYTE(1001, 1001):
    Keyboard.release(KEY_P);
    break;

  // {
  case BYTE(0001, 1010):
    Keyboard.press(KEY_LEFT_BRACE);
    break;
  case BYTE(1001, 1010):
    Keyboard.release(KEY_LEFT_BRACE);
    break;

  // }
  case BYTE(0001, 1011):
    Keyboard.press(KEY_RIGHT_BRACE);
    break;
  case BYTE(1001, 1011):
    Keyboard.release(KEY_RIGHT_BRACE);
    break;

  // Enter
  case BYTE(0001, 1100):
    Keyboard.press(KEY_ENTER);
    break;
  case BYTE(1001, 1100):
    Keyboard.release(KEY_ENTER);
    break;

  // Ctrl
  case BYTE(0001, 1101):
    Keyboard.press(MODIFIERKEY_CTRL);
    break;
  case BYTE(1001, 1101):
    Keyboard.release(MODIFIERKEY_CTRL);
    break;

  // A
  case BYTE(0001, 1110):
    Keyboard.press(KEY_A);
    break;
  case BYTE(1001, 1110):
    Keyboard.release(KEY_A);
    break;

  // S
  case BYTE(0001, 1111):
    Keyboard.press(KEY_S);
    break;
  case BYTE(1001, 1111):
    Keyboard.release(KEY_S);
    break;

  // D
  case BYTE(0010, 0000):
    Keyboard.press(KEY_D);
    break;
  case BYTE(1010, 0000):
    Keyboard.release(KEY_D);
    break;

  // F
  case BYTE(0010, 0001):
    Keyboard.press(KEY_F);
    break;
  case BYTE(1010, 0001):
    Keyboard.release(KEY_F);
    break;

  // G
  case BYTE(0010, 0010):
    Keyboard.press(KEY_G);
    break;
  case BYTE(1010, 0010):
    Keyboard.release(KEY_G);
    break;

  // H
  case BYTE(0010, 0011):
    Keyboard.press(KEY_H);
    break;
  case BYTE(1010, 0011):
    Keyboard.release(KEY_H);
    break;

  // J
  case BYTE(0010, 0100):
    Keyboard.press(KEY_J);
    break;
  case BYTE(1010, 0100):
    Keyboard.release(KEY_J);
    break;

  // K
  case BYTE(0010, 0101):
    Keyboard.press(KEY_K);
    break;
  case BYTE(1010, 0101):
    Keyboard.release(KEY_K);
    break;

  // L
  case BYTE(0010, 0110):
    Keyboard.press(KEY_L);
    break;
  case BYTE(1010, 0110):
    Keyboard.release(KEY_L);
    break;

  // ;
  case BYTE(0010, 0111):
    Keyboard.press(KEY_SEMICOLON);
    break;
  case BYTE(1010, 0111):
    Keyboard.release(KEY_SEMICOLON);
    break;

  // '
  case BYTE(0010, 1000):
    Keyboard.press(KEY_QUOTE);
    break;
  case BYTE(1010, 1000):
    Keyboard.release(KEY_QUOTE);
    break;

  // Backslash (originally ~)
  case BYTE(0010, 1001):
    Keyboard.press(KEY_BACKSLASH);
    break;
  case BYTE(1010, 1001):
    Keyboard.release(KEY_BACKSLASH);
    break;

  // Left Shift
  case BYTE(0010, 1010):
    Keyboard.press(MODIFIERKEY_SHIFT);
    break;
  case BYTE(1010, 1010):
    Keyboard.release(MODIFIERKEY_SHIFT);
    break;

  // ~ (originally Backslash)
  case BYTE(0010, 1011):
    Keyboard.press(KEY_TILDE);
    break;
  case BYTE(1010, 1011):
    Keyboard.release(KEY_TILDE);
    break;

  // Z
  case BYTE(0010, 1100):
    Keyboard.press(KEY_Z);
    break;
  case BYTE(1010, 1100):
    Keyboard.release(KEY_Z);
    break;

  // X
  case BYTE(0010, 1101):
    Keyboard.press(KEY_X);
    break;
  case BYTE(1010, 1101):
    Keyboard.release(KEY_X);
    break;

  // C
  case BYTE(0010, 1110):
    Keyboard.press(KEY_C);
    break;
  case BYTE(1010, 1110):
    Keyboard.release(KEY_C);
    break;

  // V
  case BYTE(0010, 1111):
    Keyboard.press(KEY_V);
    break;
  case BYTE(1010, 1111):
    Keyboard.release(KEY_V);
    break;

  // B
  case BYTE(0011, 0000):
    Keyboard.press(KEY_B);
    break;
  case BYTE(1011, 0000):
    Keyboard.release(KEY_B);
    break;

  // N
  case BYTE(0011, 0001):
    Keyboard.press(KEY_N);
    break;
  case BYTE(1011, 0001):
    Keyboard.release(KEY_N);
    break;

  // M
  case BYTE(0011, 0010):
    Keyboard.press(KEY_M);
    break;
  case BYTE(1011, 0010):
    Keyboard.release(KEY_M);
    break;

  // ,
  case BYTE(0011, 0011):
    Keyboard.press(KEY_COMMA);
    break;
  case BYTE(1011, 0011):
    Keyboard.release(KEY_COMMA);
    break;

  // .
  case BYTE(0011, 0100):
    Keyboard.press(KEY_PERIOD);
    break;
  case BYTE(1011, 0100):
    Keyboard.release(KEY_PERIOD);
    break;

  // /
  case BYTE(0011, 0101):
    Keyboard.press(KEY_SLASH);
    break;
  case BYTE(1011, 0101):
    Keyboard.release(KEY_SLASH);
    break;

  // Right Shift
  case BYTE(0011, 0110):
    Keyboard.press(MODIFIERKEY_RIGHT_SHIFT);
    break;
  case BYTE(1011, 0110):
    Keyboard.release(MODIFIERKEY_RIGHT_SHIFT);
    break;

  // PANIC (originally *)
  case BYTE(0011, 0111):
    Keyboard.print("AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA");
    break;
  case BYTE(1011, 0111):
    break;

  // Alt
  case BYTE(0011, 1000):
    Keyboard.press(MODIFIERKEY_GUI);
    break;
  case BYTE(1011, 1000):
    Keyboard.release(MODIFIERKEY_GUI);
    break;

  // Space
  case BYTE(0011, 1001):
    Keyboard.press(KEY_SPACE);
    break;
  case BYTE(1011, 1001):
    Keyboard.release(KEY_SPACE);
    break;

  // Caps Lock
  case BYTE(0011, 1010):
    Keyboard.press(MODIFIERKEY_RIGHT_GUI);
    break;
  case BYTE(1011, 1010):
    Keyboard.release(MODIFIERKEY_RIGHT_GUI);
    break;

  // Empty 1 (function block top left) (originally F1)
  case BYTE(0011, 1011):
    Keyboard.press(KEY_F1);
    break;
  case BYTE(1011, 1011):
    Keyboard.release(KEY_F1);
    break;

  // Empty 2 (function block top right) (originally F2)
  case BYTE(0011, 1100):
    Keyboard.press(KEY_F2);
    break;
  case BYTE(1011, 1100):
    Keyboard.release(KEY_F2);
    break;

  // Brightness Down (originally F3)
  case BYTE(0011, 1101):
    // This causes Brightness Down on macOS
    Keyboard.press(KEY_SCROLL_LOCK);
    break;
  case BYTE(1011, 1101):
    Keyboard.release(KEY_SCROLL_LOCK);
    break;

  // Brightness Up (originally F4)
  case BYTE(0011, 1110):
    // This causes Brightness Up on macOS
    Keyboard.press(KEY_PAUSE);
    break;
  case BYTE(1011, 1110):
    Keyboard.release(KEY_PAUSE);
    break;

  // Play Pause (originally F5)
  case BYTE(0011, 1111):
    Keyboard.press(KEY_MEDIA_PLAY_PAUSE);
    break;
  case BYTE(1011, 1111):
    Keyboard.release(KEY_MEDIA_PLAY_PAUSE);
    break;

  // Mute (originally F6)
  case BYTE(0100, 0000):
    Keyboard.press(KEY_MEDIA_MUTE);
    break;
  case BYTE(1100, 0000):
    Keyboard.release(KEY_MEDIA_MUTE);
    break;

  // Rewind (originally F7)
  case BYTE(0100, 0001):
    Keyboard.press(KEY_MEDIA_REWIND);
    break;
  case BYTE(1100, 0001):
    Keyboard.release(KEY_MEDIA_REWIND);
    break;

  // Fast Forward (originally F8)
  case BYTE(0100, 0010):
    Keyboard.press(KEY_MEDIA_FAST_FORWARD);
    break;
  case BYTE(1100, 0010):
    Keyboard.release(KEY_MEDIA_FAST_FORWARD);
    break;

  // Volume Down (originally F9)
  case BYTE(0100, 0011):
    Keyboard.press(KEY_MEDIA_VOLUME_DEC);
    break;
  case BYTE(1100, 0011):
    Keyboard.release(KEY_MEDIA_VOLUME_DEC);
    break;

  // Volume Up (originally F10)
  case BYTE(0100, 0100):
    Keyboard.press(KEY_MEDIA_VOLUME_INC);
    break;
  case BYTE(1100, 0100):
    Keyboard.release(KEY_MEDIA_VOLUME_INC);
    break;

  // Num Lock
  case BYTE(0100, 0101):
    Keyboard.press(KEY_NUM_LOCK);
    break;
  case BYTE(1100, 0101):
    Keyboard.release(KEY_NUM_LOCK);
    break;

  // Scroll Lock
  case BYTE(0100, 0110):
    buzzNumber(keyPresses);
    break;
  case BYTE(1100, 0110):
    break;

  // Keypad 7
  case BYTE(0100, 0111):
    Keyboard.press(KEY_HOME);
    break;
  case BYTE(1100, 0111):
    Keyboard.release(KEY_HOME);
    break;

  // Keypad 8
  case BYTE(0100, 1000):
    Keyboard.press(KEY_UP);
    break;
  case BYTE(1100, 1000):
    Keyboard.release(KEY_UP);
    break;

  // Keypad 9
  case BYTE(0100, 1001):
    Keyboard.press(KEY_PAGE_UP);
    break;
  case BYTE(1100, 1001):
    Keyboard.release(KEY_PAGE_UP);
    break;

  // Keypad -
  case BYTE(0100, 1010):
    Keyboard.press(KEYPAD_MINUS);
    break;
  case BYTE(1100, 1010):
    Keyboard.release(KEYPAD_MINUS);
    break;

  // Keypad 4
  case BYTE(0100, 1011):
    Keyboard.press(KEY_LEFT);
    break;
  case BYTE(1100, 1011):
    Keyboard.release(KEY_LEFT);
    break;

  // Keypad 5
  case BYTE(0100, 1100):
    Keyboard.press(KEY_DOWN);
    break;
  case BYTE(1100, 1100):
    Keyboard.release(KEY_DOWN);
    break;

  // Keypad 6
  case BYTE(0100, 1101):
    Keyboard.press(KEY_RIGHT);
    break;
  case BYTE(1100, 1101):
    Keyboard.release(KEY_RIGHT);
    break;

  // Keypad +
  case BYTE(0100, 1110):
    Keyboard.press(KEYPAD_PLUS);
    break;
  case BYTE(1100, 1110):
    Keyboard.release(KEYPAD_PLUS);
    break;

  // Keypad 1
  case BYTE(0100, 1111):
    Keyboard.press(KEY_END);
    break;
  case BYTE(1100, 1111):
    Keyboard.release(KEY_END);
    break;

  // Keypad 2
  case BYTE(0101, 0000):
    Keyboard.press(KEY_DOWN);
    break;
  case BYTE(1101, 0000):
    Keyboard.release(KEY_DOWN);
    break;

  // Keypad 3
  case BYTE(0101, 0001):
    Keyboard.press(KEY_PAGE_DOWN);
    break;
  case BYTE(1101, 0001):
    Keyboard.release(KEY_PAGE_DOWN);
    break;

  // Keypad 0
  case BYTE(0101, 0010):
    Keyboard.press(MODIFIERKEY_RIGHT_ALT);
    break;
  case BYTE(1101, 0010):
    Keyboard.release(MODIFIERKEY_RIGHT_ALT);
    break;

  // Keypad .
  case BYTE(0101, 0011):
    Keyboard.press(KEY_DELETE);
    break;
  case BYTE(1101, 0011):
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
  if (!readCode && digitalRead(CLK_Pin) == 0)
  {
    readCode = true;
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
    readCode = false;
  }

  // since the code detects the rising edge of clock,
  // we use readCode to filter out the fact that
  // clock rests HIGH.
  if (readCode)
  {
    // On each rising edge, get the respective DATA.
    // cycleReadYet prevents multiple reads on the same cycle.
    if (digitalRead(CLK_Pin) == 1 && !cycleReadYet)
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
      cycleReadYet = true;
    }
    // once the clock drops again, we can reset cycleReadYet
    // since DATA is only read when clock is HIGH.
    if (digitalRead(CLK_Pin) == 0)
    {
      cycleReadYet = false;
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
