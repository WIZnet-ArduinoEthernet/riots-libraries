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

#include "Riots_Button.h"
#include "Wire.h"

/**
 * Default contstructor
 *
 * @return                    Riots button instance.
 */
Riots_Button::Riots_Button( ) {
  intPin = 3;
  pinMode(intPin, INPUT);
  digitalWrite(intPin, HIGH);
}

/**
 * Setups the button library instance.
 *
 */
void Riots_Button::setup() {
  // Start I2c
  Wire.begin();

  // Check product id
  byte proid = capRead(PROID);
  if (proid != 0x6F) {
    // Should be 0x6F, try button
    while (proid != 0x6F) {
      proid = capRead(PROID);
    }
  }

  // Check manufacturer id
  byte manid = capRead(MANID);
  if (manid != 0x5D) {
    // Should be 0x5D, try again
    while (manid != 0x5D) {
      manid = capRead(MANID);
    }
  }
  // Enable sensors
  capWrite(SENEN, SENSORS);
  // Allow multitouch
  capWrite(MULTI, 0x0C);
  // Disable repeated interrupts
  capWrite(INREP, 0);

  // Read main register
  mainReg = capRead(MAINC);
}

/**
 * Read button statuses.
 *
 * @return byte               Status of the buttons
 */
byte Riots_Button::read() {
  // Check the interrupt
  if (digitalRead(intPin) == 1) {
    return 0;
  }
  // Clear the interrupt
  capWrite(MAINC, mainReg & ~INTC);
  // Read pressed buttons
  byte pressed = capRead(SENIN);
  return pressed;
}

/**
 * Read register status.
 *
 * @return byte               Status of the register.
 */
byte Riots_Button::capRead(byte reg) {
  Wire.beginTransmission(I2CA);
  Wire.write(reg);
  Wire.endTransmission();
  Wire.requestFrom(I2CA, 1);
  return Wire.read();
}

/**
 * Writes value to given register.
 *
 * @param reg                 Register to be written.
 * @param val                 Value to be written.
 * @return byte               Status of the register.
 */
void Riots_Button::capWrite(byte reg, byte val) {
  Wire.beginTransmission(I2CA);
  Wire.write(reg);
  Wire.write(val);
  Wire.endTransmission();
}
