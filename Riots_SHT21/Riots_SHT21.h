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

#ifndef Riots_SHT21_h
#define Riots_SHT21_h

#include "Arduino.h"
#include "Riots_Helper.h"
#include "Wire.h"

#define TBMEAS  0xE3
#define HBMEAS  0xE5
#define TNMEAS  0xF3
#define HNMEAS  0xF5
#define WUREG   0xE6
#define RUREG   0xE7
#define SRES    0xFE

#define ACCURATE_MEAS   0x3A // DEFAULT: 12 bit RH, 14 bit humidity. Conv time max 23ms, Repeatability/Noise: 0.025RH / 0.01DEG
#define FASTER_MEAS     0xBB // 11bit RH, 11bit humidity. Conv time max 10ms, 0.05RH / 0.08DEG
#define HNMEAS          0xF5
#define TREAD           0xE0

#define I2CA  0x40

class Riots_SHT21 {
  public:
    byte setup();
    void startMeasurement(byte low_resolution = 0);  // call this, wait for conversion time (23ms or 10ms), then call below fuctions
    int readTemperature();  // returns temperature
    int readHumidity();     // returns humidity

  private:
    uint16_t i2cRead(byte count);
    void i2cWrite(byte reg, byte val=0);
};

#endif
