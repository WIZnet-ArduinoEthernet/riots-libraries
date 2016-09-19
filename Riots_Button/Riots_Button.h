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

#ifndef Riots_Button_h
#define Riots_Button_h

#include "Arduino.h"

#define MAINC 0x00
#define SENIN 0x03
#define SENEN 0x21
#define INREP 0x28
#define MULTI 0x2A
#define PROID 0xFD
#define MANID 0xFE
#define SENSORS 0x07
#define INTC  0x01
#define I2CA  0x28

class Riots_Button {
  public:
    Riots_Button();
    void setup();
    byte read();
  private:
    byte capRead(byte reg);
    void capWrite(byte reg, byte val);
    int intPin;                         /*!< Interrupt pin number                                                           */
    byte mainReg;                       /*!< Main register status for the button                                            */

};

#endif
