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

#include "Riots_TMD3782x.h"
#include "Wire.h"

/**
 * Default constructor for the class.
 *
 * @return Riots_TMD3782x     Instance of the class.
 */
Riots_TMD3782x::Riots_TMD3782x() {

}

/**
 * Sets state for the DC in range 0-255, where 0 is off and 255 is max
 *
 * @param output_pin          New DC state for the controller.
 */
void Riots_TMD3782x::setup() {

  // Start I2c
  Wire.begin();

  // Check product ID
  byte proid = byteRead(RIOTS_TMD3782X_PRODUCT_ID_REG);

  if (RIOTS_TMD3782X_PRODUCT_ID == proid) {

    // Set WLONG to 0
    byteWrite(RIOTS_TMD3782X_CONFIG_REG, 0x00);
    // Set AGAIN to 1
    byteWrite(RIOTS_TMD3782X_CONTROL_REG, 0x20);
    AGAINx = 1;
    // Set ATIME to 152ms
    byteWrite(RIOTS_TMD3782X_ATIME_REG, 0xAD);
    ATIME_ms = 152;
    // Enable RGBC cycle
    byteWrite(RIOTS_TMD3782X_ENABLE_REG, 0x03);
  }

}

/**
 * Reads a byte from the give register.
 *
 * @param reg                 Register address for reading.
 * @return byte               Value read from the register.
 */
byte Riots_TMD3782x::byteRead(byte reg) {

  Wire.beginTransmission(RIOTS_TMD3782X_I2C_ADDRESS);
  Wire.write(RIOTS_TMD3782X_COMMAND_TYPE | reg);
  Wire.endTransmission();
  Wire.requestFrom(RIOTS_TMD3782X_I2C_ADDRESS, 1);
  return Wire.read();
}

/**
 * Writes a byte to the given register.
 *
 * @param reg                 Register address for writing.
 * @param val                 Value to be written in the register.
 */
void Riots_TMD3782x::byteWrite(byte reg, byte val) {

  Wire.beginTransmission(RIOTS_TMD3782X_I2C_ADDRESS);
  Wire.write(RIOTS_TMD3782X_COMMAND_TYPE | reg);
  Wire.write(val);
  Wire.endTransmission();
}

/**
 * Starts measurement.
 *
 */
void Riots_TMD3782x::startMeasurement() {

  byteWrite(RIOTS_TMD3782X_ENABLE_REG, 0x03);

}

/**
 * Stops measurement.
 *
 */
void Riots_TMD3782x::stopMeasurement() {

  byteWrite(RIOTS_TMD3782X_ENABLE_REG, 0x00);

}

/**
 * Reads RGB information.
 *
 */
void Riots_TMD3782x::readRgbcData () {

  // Get red, green, blue and clear values
  for (int i=0; i < 8; i++) {
    rgbc[i] = byteRead(RIOTS_TMD3782X_RGBC_DATA+i);
  }

  R = 256*rgbc[3] + rgbc[2];
  G = 256*rgbc[5] + rgbc[4];
  B = 256*rgbc[7] + rgbc[6];
  C = 256*rgbc[1] + rgbc[0];

  // Calculate IR component
  IR = ((R+G+B-C)/2);

  // Remove IR components
  R = R - IR;
  G = G - IR;
  B = B - IR;
  C = C - IR;

}

/**
 * Calculates color temperature.
 *
 * @return double             Temperature of the color in kelvin scale.
 */
double Riots_TMD3782x::getColorTemperature() {

  // CT (degrees Kelvin) = CT_Coef*(B’/R’) + CT_Offset

  if (R<=0) {
    return 0;
  }

  CT = (CT_COEF*(B/R)) + CT_OFFSET;

  return CT;

}

/**
 * Calculates brigthness.
 *
 * @return double             Brigthness in the LUX scale.
 */

double Riots_TMD3782x::getLux() {

  // G” = R_Coef * R’ + G_Coef * G’ + B_Coef * B’

  GG = ((R_COEF/1000) * R) + ((G_COEF/1000) * G) + ((B_COEF/1000) * B);

  // CPL = (ATIME_ms * AGAINx) / (GA * DF) = (ATIME_ms * AGAINx)/DGF

  if (DGF <= 0) {
    return 0;
  }

  CPL = (double(ATIME_ms) * double(AGAINx)) / DGF;

  if (CPL == 0) {
    return 0;
  }

  // Calculate Lux
  lux = GG / CPL;

  if (lux < 0) { // saturated value
    return 0;
  }

  return lux;

}
