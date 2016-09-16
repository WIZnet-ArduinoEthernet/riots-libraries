/*
 * This file is part of Riots.
 * Copyright © 2016 Riots Global OY; <copyright@myriots.com>
 *
 * Riots is free software; you can redistribute it and/or modify it under the terms of the GNU Lesser General Public License
 * as published by the Free Software Foundation; either version 2.1 of the License, or (at your option) any later version.
 *
 * Riots is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty
 * of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License along with Riots.
 * If not, see <http://www.gnu.org/licenses/>.
 */

#include "Riots_RGBLed.h"
#include "EEPROM.h"

/**
 * Setup function for the RGB led.
 * Initializes the right pins for the RGB led and sets the pins to the output mode
 *
 * @param red       Pin number for the red led
 * @param green     Pin number for the green led
 * @param blue      Pin number for the yellow led
 */
void Riots_RGBLed::setup(int red, int green, int blue) {

  red_pin = red;
  if(red_pin == 0xFF) red_pin = EEPROM.read(EEPROM_LED_RED);
  if(red_pin == 0xFF) red_pin = 5;

  green_pin = green;
  if(green_pin == 0xFF) green_pin = EEPROM.read(EEPROM_LED_GREEN);
  if(green_pin == 0xFF) green_pin = 6;

  blue_pin = blue;
  if(blue_pin == 0xFF) blue_pin = EEPROM.read(EEPROM_LED_BLUE);
  if(blue_pin == 0xFF) blue_pin = 9;

  pinMode(red_pin, OUTPUT);
  pinMode(green_pin, OUTPUT);
  pinMode(blue_pin, OUTPUT);

  this->red_value = 0;
  this->green_value = 0;
  this->blue_value = 0;
  updateLedColor();
}

/**
 * Sets color of the led with RGB value
 *
 * @param red       Value (0-255) for the red led
 * @param green     Value (0-255) for the green led
 * @param blue      Value (0-255) for the blue led
 */
void Riots_RGBLed::setColor(uint8_t red, uint8_t green, uint8_t blue) {
  this->red_value = red;
  this->green_value = green;
  this->blue_value = blue;
  updateLedColor();
}

/**
 * Sets color of the led with uint32 value.
 * RGB = (R*65536)+(G*256)+B
 *
 * @param color      Color in the uin32 value
 */
void Riots_RGBLed::setColor(uint32_t color) {
  setColor(((color >> 16) & 0xFF), ((color >> 8) & 0xFF), (color & 0xFF));
}

/**
 * Update color values to RGB led.
 *
 */
void Riots_RGBLed::updateLedColor() {
  analogWrite(red_pin, (255-this->red_value));
  analogWrite(green_pin, (255-this->green_value));
  analogWrite(blue_pin, (255-this->blue_value));
}
