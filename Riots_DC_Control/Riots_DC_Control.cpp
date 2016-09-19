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

/* Library skeleton for Riots DC Control */
#include "Riots_DC_Control.h"

/**
 * Default contstructor
 *
 * @return                    Riots DC control instance.
 */
Riots_DC_Control::Riots_DC_Control() {
}

/**
 * Setups the DC control instance.
 *
 * @param output_pin          Output pin to use for DC setup.
 */
void Riots_DC_Control::setup(uint8_t output_pin) {
  control_pin = output_pin;
  pinMode(control_pin, OUTPUT);

  // By default
  myOutputValue = 0;
  analogWrite(control_pin, 255-myOutputValue);
}

/**
 * Sets state for the DC in range 0-255, where 0 is off and 255 is max
 *
 * @param output_pin          New DC state for the controller.
 */
void Riots_DC_Control::setState(uint8_t newOutputValue) {
  if (newOutputValue > 255) {
    newOutputValue = 255;
  }
  if (newOutputValue != myOutputValue) {
    // Reverse the value
    analogWrite(control_pin, 255-newOutputValue);
    myOutputValue = newOutputValue;
  }
}

/**
 * Sets state for the DC in range 0-255, where 0 is off and 255 is max
 *
 * @return uint8_t            Current state of the DC controller.
 */
uint8_t Riots_DC_Control::getState() {
  return myOutputValue;
}
