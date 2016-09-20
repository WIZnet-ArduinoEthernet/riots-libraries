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
 *
 * This Library is based on Arduino BMP280 library version 1.01 written by mhafuzul islam. Original Library code uses
 * the "pizza-eating" license. Riots will offer Italian Pizza for original library writer someday.
 */

#ifndef Riots_BMP280_h
#define Riots_BMP280_h

#include "Arduino.h"
#include "Riots_Helper.h"

#define RIOTS_BMP280_ADDR 0x77
#define RIOTS_BMP280_REG_CONTROL 0xF4
#define RIOTS_BMP280_REG_RESULT_PRESSURE 0xF7
#define RIOTS_BMP280_REG_RESULT_TEMPRERATURE 0xFA
#define RIOTS_BMP280_COMMAND_TEMPERATURE 0x2E
#define RIOTS_BMP280_COMMAND_PRESSURE0 0x25 // 001 001 01  x1 x1
#define RIOTS_BMP280_COMMAND_PRESSURE4 0x5D // 010 111 01  x2 x16

class Riots_BMP280 {

  public:
    byte setup();
    void startMeasurement(byte low_resolution = 0);
    int32_t getPressure();

  private:
  void calcPressure(double &P, double uP);
  void calcTemperature(double &T, double &uT);
  byte readInt(byte address, int &value);
  byte readUInt(byte address, unsigned int &value);
  byte readBytes(unsigned char *values, byte length);
  byte writeBytes(unsigned char *values, byte length);
  byte getUnCalValues(double &uP, double &uT);

  int dig_T2, dig_T3, dig_T4, dig_P2, dig_P3, dig_P4, dig_P5, dig_P6, dig_P7, dig_P8, dig_P9;
  unsigned int dig_P1, dig_T1;
  long signed int t_fine;
  byte error;
};
#endif
