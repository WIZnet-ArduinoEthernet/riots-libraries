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

#ifndef Riots_Memory_H
#define Riots_Memory_H

#include "Arduino.h"
#include <inttypes.h>

#define RIOTS_PRIMARY_EEPROM    0xA0  // I2C bus address of primary x24C01 EEPROM
#define RIOTS_SECONDARY_EEPROM  0xA2  // I2C bus address of secondary x24C01 EEPROM
#define F_SCL 100000L                 // I2C clock speed 100 kHz
#define TW_SEND 0x84                  // send data (TWINT,TWEN)
#define TW_START 0xA4                 // send start condition (TWINT,TWSTA,TWEN)
#define TW_READY (TWCR & 0x80)        // ready when TWINT returns to logic 1.
#define TW_STATUS (TWSR & 0xF8)       // returns value of status register
#define TW_STOP 0x94                  // send stop condition (TWINT,TWSTO,TWEN)
#define I2C_Stop() TWCR = (1<<TWINT) | (1<<TWEN) | (1<<TWSTO) // inline macro for stop condition
#define TW_NACK 0x84                  // read data with NACK (last uint8_t)

class Riots_Memory {
  public:
    static uint8_t setup(uint8_t eeprom_addr);
    static void write(uint16_t page_addr, uint8_t data, uint8_t eeprom_addr=RIOTS_PRIMARY_EEPROM);/*!< Write a page to certain address      */
    static uint8_t read(uint16_t page_addr, uint8_t eeprom_addr=RIOTS_PRIMARY_EEPROM);            /*!< Read a data from certaing page       */
    static void startPageWrite(uint16_t page_addr, uint8_t eeprom_addr);                          /*!< Start page writing to the EEPROM     */
    static void pageFill(uint8_t data);                                                           /*!< Fills the page with the given data   */
    static void stopPageWrite();                                                                  /*!< Stops the page writing               */
    static void startRead(uint16_t page_addr, uint8_t eeprom_addr);                               /*!< Starts reading from the given address*/
    static uint8_t sequentialRead();                                                              /*!< Starts sequntial reading             */
    static uint8_t readLast();                                                                    /*!< Read last data and stops reading     */

  private:
    static void I2C_Init();                                                                       /*!< Initializes I2C bus                  */
    static uint8_t I2C_Start();                                                                   /*!< Starts I2C communication             */
    static uint8_t I2C_SendAddr(uint8_t addr);                                                    /*!< Sends data to given address          */
    static uint8_t I2C_Write(uint8_t data);                                                       /*!< Writes data to given address         */
    static uint8_t I2C_ReadACK();                                                                 /*!< Read from I2C with ACK               */
    static uint8_t I2C_ReadNACK();                                                                /*!< Read from I2C without ACK            */
};

#endif //Riots_Memory_H
