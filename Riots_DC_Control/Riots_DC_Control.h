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

#ifndef Riots_DC_CONTROL_h
#define Riots_DC_CONTROL_h

#include "Arduino.h"

class Riots_DC_Control {
  public:
    Riots_DC_Control();
    void setup(uint8_t output_pin = 10);
    void setState(uint8_t newOuputValue);
    uint8_t getState();
  private:
    uint8_t myOutputValue;              /*!< Current value of DC                                                            */
    uint8_t control_pin;                /*!< Pin number for controlling DC.                                                 */
};

#endif
