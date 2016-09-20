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

#ifndef RIOTS_TMD3782X_h
#define RIOTS_TMD3782X_h

#include "Arduino.h"

#define RIOTS_TMD3782X_I2C_ADDRESS     0x39

#define RIOTS_TMD3782X_PRODUCT_ID_REG  0x12
#define RIOTS_TMD3782X_ENABLE_REG      0x00
#define RIOTS_TMD3782X_ATIME_REG       0x01
#define RIOTS_TMD3782X_CONFIG_REG      0x0D
#define RIOTS_TMD3782X_CONTROL_REG     0x0F
#define RIOTS_TMD3782X_STATE_REG       0x13
#define RIOTS_TMD3782X_RGBC_DATA       0x14
#define RIOTS_TMD3782X_COMMAND_TYPE    0x80
#define RIOTS_TMD3782X_PRODUCT_ID      0x60

#define DGF                            312
#define R_COEF                         93
#define G_COEF                         1000
#define B_COEF                         -522
#define CT_COEF                        4916
#define CT_OFFSET                      1427

class Riots_TMD3782x {
  public:
    Riots_TMD3782x();
    void setup();
    void startMeasurement();
    void stopMeasurement();
    void readRgbcData();
    double getColorTemperature();
    double getLux();
  private:
    double R, G, B, C, IR, GG, CPL, lux, CT; /*!< Values used for calculating RBG and brigthness                             */
    int ATIME_ms, AGAINx;                   /*!< ATIME and AGAIN for the LUX calculations                                   */
    byte rgbc[8];                           /*!< RGC values                                                                 */

    byte byteRead(byte reg);
    void byteWrite(byte reg, byte val);
};

#endif
