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

#ifndef RIOTS_MAMADEF_H
#define RIOTS_MAMADEF_H

#include <avr/io.h>
// EEPROM INDEX

#define _SET(type,name,bit)       type ## name  |= _BV(bit)
#define _CLEAR(type,name,bit)     type ## name  &= ~ _BV(bit)
#define _TOGGLE(type,name,bit)    type ## name  ^= _BV(bit)
#define _GET(type,name,bit)       ((type ## name >> bit) &  1)
#define _PUT(type,name,bit,value) type ## name = ( type ## name & ( ~ _BV(bit)) ) | ( ( 1 & (unsigned char)value ) << bit )

//these macros are used by end user
#define PIN_OUTPUT(pin)          _SET(DDR,pin)
#define PIN_INPUT(pin)           _CLEAR(DDR,pin)
#define PIN_HIGH(pin)            _SET(PORT,pin)
#define PIN_LOW(pin)             _CLEAR(PORT,pin)
#define PIN_TOGGLE(pin)          _TOGGLE(PORT,pin)
#define PIN_READ(pin)            _GET(PIN,pin)

// Definition of the pins
#define RIOTS_ETHERNET_RESET      10
#define RIOTS_SIM800_RESET        10
#define RIOTS_ETHERNET_CS         0

// Cloud message definitions
#define DATA_BLOCK_SIZE           16
#define DATA_HEADER_SIZE          2

// Cloud connection configurations
#define RIOTS_CLOUD_ADDRESS       "mama.riots.fi"
#define RIOTS_CLOUD_PORT          8000
#define PINGING_INTERVAL          20000
#define CONNECTION_RETRY_TIME     1000
#define MAMA_CLOUD_PAUSE          64

// Possible actions for cloud interaction

#define NO_ACTION_REQUIRED        0x00
#define SET_RADIO_RECEIVER        0x01
#define FORWARD_DATA              0x02

// Mama cloud message types
#define NO_MESSAGE_TO_CLOUD       0x00
#define CONFIRM_CONFIG            0x01
#define MAMA_IS_ONLINE            0x02

// EEPROM addresses for mama
#define I2C_NEXT_MESSAGE_ADDR     0x0000  // 4 bytes
#define I2C_EEPROM_MIN            128
#define I2C_EEPROM_MAX            64000
#define I2C_EEPROM_MSG_SIZE       20

enum Riots_Message {
  // possible client messages
  CLIENT_INTRODUCTION           = 0x01,
  CLIENT_VERIFICATION           = 0x02,
  CLIENT_DATA_POST              = 0x03,
  CLIENT_SAVED_DATA_POST        = 0x04,
  CLIENT_DATA_NOT_RECEIVED      = 0x05,
  KEEP_ALIVE                    = 0x06,

  // Possible server initiated messages
  SERVER_VERIFICATION           = 0x21,
  SERVER_DATA_RECEIVER          = 0x22,
  SERVER_DATA_POST              = 0x23,
  SERVER_REQUESTS_INTRODUCTION  = 0x24,

  // Debug over serial
  MAMA_SERIAL_DEBUG             = 0xDD,
  MAMA_SERIAL_SEND_MORE_DATA    = 0xED,
  MAMA_SERIAL_SEND_INTRO_AGAIN  = 0xAD
};

#endif // RIOTS_MAMA_DEF_H
