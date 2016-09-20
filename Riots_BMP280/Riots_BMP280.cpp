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

#include "Riots_BMP280.h"
#include <Wire.h>
#include <stdio.h>
#include <math.h>

/**
 * Setup BMP280 library
 *
 * @return byte               RIOTS_OK if the sensor giving correct data.
 */
byte Riots_BMP280::setup() {

  // Get calibration data from device
  if (  readUInt(0x88, dig_T1) &&
        readInt(0x8A, dig_T2)  &&
        readInt(0x8C, dig_T3)  &&
        readUInt(0x8E, dig_P1) &&
        readInt(0x90, dig_P2)  &&
        readInt(0x92, dig_P3)  &&
        readInt(0x94, dig_P4)  &&
        readInt(0x96, dig_P5)  &&
        readInt(0x98, dig_P6)  &&
        readInt(0x9A, dig_P7)  &&
        readInt(0x9C, dig_P8)  &&
        readInt(0x9E, dig_P9)) {
    return RIOTS_OK;
  }

    return RIOTS_FAIL;
}

/**
 * Reads integer value from the given register
 *
 * @param address             Address for reading.
 * @param value               Pointer to the memory where the int value should be written.
 * @return byte               1, if successful
 */
byte Riots_BMP280::readInt(byte address, int &value) {

  unsigned char data[2];  //byte is 4bit,1byte
  data[0] = address;

  if (readBytes(data,2)) {
    value = (((int)data[1]<<8)|(int)data[0]);
    return 1;
  }
  value = 0;
  return 0;

}

/**
 * Reads unsigned integer value from the given register
 *
 * @param address             Address for reading.
 * @param value               Pointer to the memory where the unsigned int value should be written.
 * @return byte               1, if successful
 */
byte Riots_BMP280::readUInt(byte address, unsigned int &value) {

  unsigned char data[2];
  data[0] = address;
  if (readBytes(data,2)) {
    value = (((unsigned int)data[1]<<8)|(unsigned int)data[0]);
    return(1);
  }
  value = 0;
  return 0;

}

/**
 * Reads bytes from the I2C.
 *
 * @param values              First index address in the memory, where values should be saved.
 * @param length              How many chars should be read from I2C.
 * @return byte               1, if successful
 */
byte Riots_BMP280::readBytes(unsigned char *values, byte length) {

  Wire.beginTransmission(RIOTS_BMP280_ADDR);
  Wire.write(values[0]);
  error = Wire.endTransmission();
  if (error == 0) {
    Wire.requestFrom(RIOTS_BMP280_ADDR,length);
    while(Wire.available() != length) ; // wait bytes
    for(int i=0;i<length;i++) {
      values[i] = Wire.read();
    }
    return 1;
  }
  return 0;
}

/**
 * Reads bytes from the I2C.
 *
 * @param values              First index address in the memory, where values should be read.
 * @param length              How many chars should be written to the I2C.
 * @return byte               RIOTS_OK, if successful
 */
byte Riots_BMP280::writeBytes(unsigned char *values, byte length) {

  Wire.beginTransmission(RIOTS_BMP280_ADDR);
  Wire.write(values,length);
  error = Wire.endTransmission();
  if (error == 0) {
    return RIOTS_OK;
  } else {
   return RIOTS_FAIL;
  }

}


/**
 * Starts measurement.
 *
 * @param low_resolution      Set to 1 if LOW_RESOLUTION measurement should be used.
 */
void Riots_BMP280::startMeasurement(byte low_resolution) {

  unsigned char data[2], result, delay;
  data[0] = RIOTS_BMP280_REG_CONTROL;

  if (low_resolution == 1) {
    data[1] = RIOTS_BMP280_COMMAND_PRESSURE0; // delay needed 8ms
  }
  else {
    data[1] = RIOTS_BMP280_COMMAND_PRESSURE4; // delay needed 37ms
  }

  writeBytes(data, 2);

}

/**
 * Get values
 *
 * @param uP                  uP value to be used.
 * @param uT                  uT value to be used.
 * @return byte               result of the calculation.
 */
byte Riots_BMP280::getUnCalValues(double &uP, double &uT) {

  byte result;

  unsigned char data[6];

  data[0] = RIOTS_BMP280_REG_RESULT_PRESSURE;

  result = readBytes(data, 6);
  if (result) {
    double factor = pow(2, 4);
  uP = (( (data[0] *256.0) + data[1] + (data[2]/256.0))) * factor;
  uT = (( (data[3] *256.0) + data[4] + (data[5]/256.0))) * factor;
  }
  return result ;
}

/**
 * Get current pressure
 *
 * @return int32_t            Current pressure measured by sensor.
 */
int32_t Riots_BMP280::getPressure() {

  double uP,uT,P,T ;
  byte result = getUnCalValues(uP, uT);
  if(result!=0){
    // Calculate the pressure
    // Temperature needs to be calculated first
    calcTemperature(T,uT);
    calcPressure(P,uP);
    return (int32_t)P;
  }
  return RIOTS_SENSOR_FAIL;

}

/**
 * Calculate tempereture based on given values.
 *
 * @param uP                  uP value to be used.
 * @param uT                  uT value to be used.
 */
void Riots_BMP280::calcTemperature(double &T, double &uT) {

  double adc_T = uT ;

  double var1 = (((double)adc_T)/16384.0-((double)dig_T1)/1024.0)*((double)dig_T2);
  double var2 = ((((double)adc_T)/131072.0 - ((double)dig_T1)/8192.0)*(((double)adc_T)/131072.0 - ((double)dig_T1)/8192.0))*((double)dig_T3);
  t_fine = (long signed int)(var1+var2);

  T = (var1+var2)/5120.0;

}

/**
 * Calculate pressure based on given values.
 *
 * @param uP                  uP value to be used.
 * @param uT                  uT value to be used.
 */
void Riots_BMP280::calcPressure(double &P,double uP) {

  double var1 , var2 ;

  var1 = ((double)t_fine/2.0) - 64000.0;
  var2 = var1 * (var1 * ((double)dig_P6)/32768.0);
  var2 = var2 + (var1 * ((double)dig_P5)*2.0);
  var2 = (var2/4.0)+(((double)dig_P4)*65536.0);
  var1 = (((double)dig_P3) * var1 * var1/524288.0 + ((double)dig_P2) * var1) / 524288.0;
  double t_var = (32768.0 + var1)/32768.0;
  double tt_var = t_var * (double)dig_P1;
  var1 = ((32768.0 + var1)/32768.0)*((double)dig_P1);
  double p = 1048576.0- (double)uP;
  p = (p-(var2/4096.0))*6250.0/var1;
  var1 = ((double)dig_P9)*p*p/2147483648.0;
  var2 = p*((double)dig_P8)/32768.0;
  p = p + (var1+var2+((double)dig_P7))/16.0;
  P = p; //Pa

}
