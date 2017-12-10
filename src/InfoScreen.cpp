/**********************************************************************
DCC++ BASE STATION FOR ESP32

COPYRIGHT (c) 2017 Mike Dunston

  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.
  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
  You should have received a copy of the GNU General Public License
  along with this program.  If not, see http://www.gnu.org/licenses
**********************************************************************/

#include "DCCppESP32.h"

#if defined(INFO_SCREEN_OLED) && INFO_SCREEN_OLED
#include <Wire.h>
#include "InfoScreen_OLED_font.h"
#if OLED_CHIPSET == SH1106
#include <SH1106Wire.h>
SH1106Wire oledDisplay(INFO_SCREEN_OLED_I2C_ADDRESS, SDA, SCL);
#elif OLED_CHIPSET == SH1306
#include <SSD1306Wire.h>
SSD1306Wire oledDisplay(INFO_SCREEN_OLED_I2C_ADDRESS, SDA, SCL);
#endif
#elif defined(INFO_SCREEN_LCD) && INFO_SCREEN_LCD
#include <Wire.h>
#include <LiquidCrystal_PCF8574.h>
LiquidCrystal_PCF8574 lcdDisplay(LCD_ADDRESS);
#endif

bool InfoScreen::_enabled;
String InfoScreen::_lines[INFO_SCREEN_MAX_LINES];

void InfoScreen::init() {
  _enabled = false;
  for(int i = 0; i < INFO_SCREEN_MAX_LINES; i++) {
    _lines[i] = "";
  }
#if defined(INFO_SCREEN_OLED) && INFO_SCREEN_OLED
  Wire.begin();
  // Check that we can find the OLED screen by its address before attempting
  // to use/configure it.
  Wire.beginTransmission(INFO_SCREEN_OLED_I2C_ADDRESS);
  if(Wire.endTransmission() == 0) {
    oledDisplay.init();
    oledDisplay.setContrast(255);
  	if(INFO_SCREEN_OLED_VERTICAL_FLIP == true) {
  		oledDisplay.flipScreenVertically();
  	}

    // NOTE: If the InfoScreen_OLED_font.h file is modified with a new font
    // definition, the name of the font needs to be declared on the next line.
  	oledDisplay.setFont(Monospaced_plain_10);
    _enabled = true;
  } else {
    Serial.print("OLED screen not found at 0x");
    Serial.println(INFO_SCREEN_OLED_I2C_ADDRESS, HEX);
  }
#elif defined(INFO_SCREEN_LCD) && INFO_SCREEN_LCD
  Wire.begin();
  // Check that we can find the LCD by its address before attempting to use it.
  Wire.beginTransmission(INFO_SCREEN_LCD_I2C_ADDRESS);
  if(Wire.endTransmission() == 0) {
    lcdDisplay.begin(LCD_COLUMNS, LCD_LINES);
    lcdDisplay.setBacklight(255);
    lcdDisplay.clear();
    _enabled = true;
  }
#endif
  if(!_enabled) {
    Serial.println("Unable to initialize InfoScreen, switching to Serial");
  }
}

void InfoScreen::clear() {
  if(_enabled) {
#if defined(INFO_SCREEN_OLED) && INFO_SCREEN_OLED
    oledDisplay.clear();
#elif defined(INFO_SCREEN_LCD) && INFO_SCREEN_LCD
    lcdDisplay.clear();
#endif
  }
}

void InfoScreen::printf(int col, int row, const __FlashStringHelper *format, ...) {
  char buf[256] = {0};
  va_list args;
  va_start(args, format);
  vsnprintf_P(buf, sizeof(buf), (const char *)format, args);
  va_end(args);
  if(_enabled) {
#if defined(INFO_SCREEN_OLED) && INFO_SCREEN_OLED
    _lines[row] = _lines[row].substring(0, col) + buf + _lines[row].substring(col + strlen(buf));
    oledDisplay.clear();
    for(int line = 0; line < INFO_SCREEN_MAX_LINES; line++) {
      oledDisplay.drawString(0, line * Monospaced_plain_10[1], _lines[line]);
    }
    oledDisplay.display();
#elif defined(INFO_SCREEN_LCD) && INFO_SCREEN_LCD
    if(row <= INFO_SCREEN_LCD_LINES) {
      lcdDisplay.setCursor(col, row);
      lcdDisplay.print(buf);
    }
#endif
  } else {
    Serial.println(buf);
  }
}

void InfoScreen::printf(int col, int row, const char *format, ...) {
  char buf[256] = {0};
  va_list args;
  va_start(args, format);
  vsnprintf(buf, sizeof(buf), (const char *)format, args);
  va_end(args);
  if(_enabled) {
#if defined(INFO_SCREEN_OLED) && INFO_SCREEN_OLED
    _lines[row] = _lines[row].substring(0, col) + buf + _lines[row].substring(col + strlen(buf));
    oledDisplay.clear();
    for(int line = 0; line < INFO_SCREEN_MAX_LINES; line++) {
      oledDisplay.drawString(0, line * Monospaced_plain_10[1], _lines[line]);
    }
    oledDisplay.display();
#elif defined(INFO_SCREEN_LCD) && INFO_SCREEN_LCD
    if(row <= INFO_SCREEN_LCD_LINES) {
      lcdDisplay.setCursor(col, row);
      lcdDisplay.print(buf);
    }
#endif
  } else {
    Serial.println(buf);
  }
}
