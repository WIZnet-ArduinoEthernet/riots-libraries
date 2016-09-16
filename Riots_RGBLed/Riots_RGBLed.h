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

#ifndef Riots_RGBLed_H
#define Riots_RGBLed_H

#include "Arduino.h"
#include "Riots_Helper.h"

class Riots_RGBLed {
  public:
    void setup(int red = 0xFF, int green = 0xFF, int blue = 0xFF);
    void setColor(uint8_t red, uint8_t green, uint8_t blue);
    void setColor(uint32_t color);

  private:
    void updateLedColor();
    int red_pin;              /*!< Pin number of red led                    */
    int green_pin;            /*!< Pin number of green led                  */
    int blue_pin;             /*!< Pin number of blue led                   */
    uint8_t red_value;        /*!< Received value for the red led           */
    uint8_t green_value;      /*!< Received value for the green led         */
    uint8_t blue_value;       /*!< Received value for the blue led          */
};

#endif
