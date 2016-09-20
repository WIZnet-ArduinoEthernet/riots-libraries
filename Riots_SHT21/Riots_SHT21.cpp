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

#include "Riots_SHT21.h"

/**
 * Default contstructor
 *
 * @return Riots_SHT21        Riots SHT21 library instance.
 */
byte Riots_SHT21::setup() {
  return RIOTS_OK;
}

/**
 * Starts measurement.
 *
 * @param low_resolution      Set to 1 if LOW_RESOLUTION measurement should be used.
 */
void Riots_SHT21::startMeasurement(byte low_resolution) {
  // Start conversion for temp & humi, takes max 10ms/23ms

  if(low_resolution == 1) {
    // Conversion time for temp & humi is max 10ms
    i2cWrite(WUREG, FASTER_MEAS);
  }
  else {
    i2cWrite(WUREG, ACCURATE_MEAS);
  }
  // Start measurement
  i2cWrite(HNMEAS);
}

/**
 * Reads temperature
 *
 * @return int                Temperature measured by the sensor.
 */
int Riots_SHT21::readTemperature() {

  // Resolution 14bit
  // T=-46.85+175,72*Reg/2^16
  i2cWrite(TREAD); // read temperature from previous conversion
  uint16_t temperature = i2cRead(2);
  if(temperature == 0) {
    return RIOTS_SENSOR_FAIL;
  }

  // Clear status bits
  temperature &= 0xFFFC;

  // Convert temperature to Celcius
  float tempcalc = (175.72*(temperature/pow(2,16)))-46.85;

  // Return including one decimal
  return (int)(tempcalc*10);

}

/**
 * Reads humidity
 *
 * @return int                Humidity measured by the sensor.
 */
int Riots_SHT21::readHumidity() {
  // Resolution 12bit
  // RH=-6+125*Reg/2^16
  uint16_t humidity = i2cRead(2);
  if(humidity == 0) {
    return RIOTS_SENSOR_FAIL;
  }
  // Clear status bits
  humidity &= 0xFFFC;
  float humicalc = (125*(humidity/pow(2,16)))-6;
  // Return as deci%RH
  return (int)(humicalc*10);
}

/**
 * Reads value from i2c
 *
 * @param count              How many times value should be read from i2c.
 * @return uint16_t          value read from the I2C.
 */
uint16_t Riots_SHT21::i2cRead(byte count) {
  uint16_t retval = 0;
  byte counter = 0;

  Wire.requestFrom(I2CA, count);
  while(Wire.available() < count && counter < 100) {
    counter += 1;
  }
  if (counter == 100) {
    //Serial.println("Fail");
    return 0;
  }

  for (int i=0; i<count; i++) {
    retval = (retval << 8) + Wire.read();
  }
  return retval;
}

/**
 * Writes a byte to given register
 *
 * @param reg              Register address to be used.
 * @param val              Value to be written to the register.
 */
void Riots_SHT21::i2cWrite(byte reg, byte val) {
  Wire.beginTransmission(I2CA);
  Wire.write(reg);
  if(val != 0) { // something to write to register
    Wire.write(val);
  }
  Wire.endTransmission();
}
