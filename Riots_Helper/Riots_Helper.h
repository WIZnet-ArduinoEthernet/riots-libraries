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

#ifndef Riots_Helper_h
#define Riots_Helper_h

// EEPROM INDEX
#define EEPROM_AES_UNIQUE       0x0300  // 16 bytes

#define EEPROM_RX_ADDR          0x0310  // 4 bytes

#define EEPROM_RING_NEXT        0x0314  // 4 bytes
#define EEPROM_RING_PREV        0x0318  // 4 bytes

#define EEPROM_MAMA_ADDR        0x0320  // 4 bytes
#define EEPROM_CHILD_ADDR       0x0324  // 4 bytes
#define EEPROM_DEBUG_ADDR       0x0328  // 4 bytes

#define EEPROM_BASE_ID          0x0330  // 4 bytes

#define EEPROM_CHILD_ID         0x0334  // 2 bytes
#define EEPROM_COUNTER          0x0336  // 2 bytes

#define EEPROM_MAMA_FIRMWARE_ID 0x0338  // 4 bytes
#define EEPROM_FIRMWARE_ID      0x0340  // 4 bytes

#define EEPROM_BOOT_STATUS      0x0348  // 1 byte
#define EEPROM_CORE_STATUS      0x0349  // 1 byte
#define EEPROM_NET_STATUS       0x034A  // 1 byte

#define PREV_BASE_ID            0x034C  // 4 bytes

#define EEPROM_SLEEP_ENABLED    0x03B8  // 1 byte

#define EEPROM_AES_CHANGING     0x0350  // 16 bytes
#define EEPROM_AES_OLD          0x0360  // 16 bytes

#define EEPROM_CORE_INDEX       0x03A0  // 8 bytes
#define EEPROM_IO_INDEX         0x03A8  // 1 byte
#define EEPROM_RING_INDEX       0x03B0  // 8 bytes

#define EEPROM_RING_NEXT_CHILD  0x33C   // 2 bytes
#define EEPROM_RING_PREV_CHILD  0x33E   // 2 bytes

#define EEPROM_RADIO_IRQ        0x03F0  // 1 byte
#define EEPROM_RADIO_CSN        0x03F1  // 1 byte
#define EEPROM_RADIO_CE         0x03F2  // 1 byte

#define EEPROM_LED_RED          0x03F3  // 1 byte
#define EEPROM_LED_GREEN        0x03F4  // 1 byte
#define EEPROM_LED_BLUE         0x03F5  // 1 byte

#define EEPROM_RESET            0x03F6  // 1 byte
#define EEPROM_FIRST_BOOT       0x03F7  // 1 byte

// I2C EEPROM INDEX
#define I2C_EEPROM_BASE_ID      0x0000  // 4 bytes
#define I2C_EEPROM_O_FW_ID      0x0004  // 4 bytes
#define I2C_EEPROM_O_FW_LENGTH  0x0008  // 2 bytes
#define I2C_EEPROM_UO_FW_ID     0x000A  // 4 bytes
#define I2C_EEPROM_UO_FW_LENGTH 0x000E  // 2 bytes
#define I2C_EEPROM_MAC_ADDRESS  0x0010  // 4 bytes
#define I2C_EEPROM_O_FW         0x0080  // 0x6F80 bytes
#define I2C_EEPROM_UO_FW        0x7000  // 0x6F80 bytes
#define I2C_EEPROM_FREE_SPACE   0xDF80

#define I2C_EEPROM_PAGE_SIZE    0x0080

// Boot status
#define BOOT_NORMAL             0x07
#define BOOT_LOAD_OFFICIAL      0x12
#define BOOT_LOAD_UNOFFICIAL    0x1D
#define BOOT_LOAD_FAILSAFE      0x28

// Message structure
#define RF_PAYLOAD_SIZE         16
#define RF_ADDRESS_SIZE         4
#define MAC_ADDRESS_SIZE        4
#define AES_KEY_SIZE            16
#define AES_MSG_DELIVERY_SIZE   8

#define MAGIC_ADDRESS_BYTE      0x42

#define M_TYPE                  0x0
#define M_LENGTH                0x1
#define M_VALUE                 0x2
#define M_STATUS                0x3
#define M_VALUE2                0x6
#define M_COUNTER               0xB
#define M_CHILD_ID              0xD
#define M_LAST_DIGIT            0xF

// Retry values
#define BABY_RADIO_RETRY_COUNT  4
#define BABY_RADIO_RETRY_TIME   38
#define MAMA_RETRY_COUNT        4
#define MAX_SKIPPED_RING_EVENTS 8

#define RIOTS_DELAY_MIN         7
#define RIOTS_DELAY_MAX         23

// Max. radio airtime is 5.2ms, this is to check that no hangup happens
#define MAX_RADIO_AIRTIME       100

// Default pins for Core v06
#define RIOTS_IRQ_PIN           2
#define RIOTS_CSN_PIN           14
#define RIOTS_CE_PIN            15
#define RIOTS_LED_RED_PIN       5
#define RIOTS_LED_GREEN_PIN     6
#define RIOTS_LED_BLUE_PIN      9
#define RIOTS_RESET_PIN         4


// Message types
#define TYPE_CLOUD_EVENT      0x01
#define TYPE_CLOUD_EVENT_DOWN 0x02
#define TYPE_RING_EVENT       0x10
#define TYPE_RING_EVENT_BACK  0x11

#define TYPE_DEACTIVATE_RING  0x22
#define TYPE_ACTIVATE_RING    0x23

#define TYPE_SET_ADDRESS_PREV 0x30
#define TYPE_SET_ADDRESS_NEXT 0x31

#define TYPE_MAMA_ADDRESS     0x32
#define TYPE_DEBUG_ADDRESS    0x33
#define TYPE_CHILD_ADDRESS    0x34

#define TYPE_SET_BATTERY_OP   0x36

#define TYPE_AES_PART1        0x40
#define TYPE_AES_PART2        0x41

#define TYPE_AES_USAGE        0x42
#define TYPE_CHILD_ID         0x43

#define TYPE_CONFIRM_CONFIG   0x49
#define TYPE_CORE_NOT_REACHED 0x4A

#define TYPE_ENTER_PROGMODE   0x50
#define TYPE_LEAVE_PROGMODE   0x51
#define TYPE_LOAD_ADDRESS     0x55
#define TYPE_PROG_FLASH       0x60
#define TYPE_PROG_PAGE        0x64

#define TYPE_INIT_AES_PART1   0x72
#define TYPE_INIT_AES_PART2   0x73
#define TYPE_INIT_PARENT      0x74
#define TYPE_INIT_STATUS      0x75
#define TYPE_INIT_CONFIRM     0x76
#define TYPE_IM_ALIVE         0x77
#define TYPE_INIT_MAMA        0x78

#define TYPE_DEBUG_START      0x80
#define TYPE_DEBUG_DATA       0x81

// Message lenghts
#define INIT_STATUS_LEN       0x8
#define INIT_IDS_LEN          0x8
#define CONFIRM_CONFIG_LEN    0x2
#define IM_ALIVE_NO_BASE_LEN  0x4
#define IM_ALIVE_LEN          0x8
#define CORE_NOT_REACHED_LEN  0x4

 // Net status
#define NET_CLOUD_CONNECTION  0
#define NET_ROUTE_CONNECTION  1
#define NET_RING_CONNECTION   2
#define NET_DEBUG_CONNECTION  3

 // Core status
#define CORE_INIT_AES1          0
#define CORE_INIT_AES2          1
#define CORE_INIT_PARENT        2
#define CORE_MAMA_ADDRESS_SET   6
#define CORE_CHILD_ADDRESS_SET  7


// Send status
#define SEND_SUCCESS      0
#define SEND_FAIL         1
#define SEND_DECRYPT_FAIL 2
#define SEND_LENGTH_FAIL  3
#define SEND_TYPE_FAIL    4
#define SEND_PROG_FAIL    5
#define SEND_CONFIG_OK    6

 // Error codes
#define RIOTS_OK                  0x00
#define RIOTS_FAIL                0x01
#define RIOTS_NOT_FOUND           0x02
#define RIOTS_NOT_CONNECTED       0x03
#define RIOTS_INVALID_COUNTER     0x04
#define RIOTS_WRONG_PACKAGE_ORDER 0x05
#define RIOTS_NO_DATA_AVAILABLE   0x06
#define RIOTS_NO_CLOUD_CONNECTION 0x07
#define RIOTS_RING_NOT_AVAILABLE  0x08
#define RIOTS_CLOUD_FAIL          0x09
#define RIOTS_RING_FAIL           0x0A
#define RIOTS_INCORRECT_IO        0x0B
#define RIOTS_FACTOR_NOT_ALLOWED  0x0C
#define RIOTS_RESET               0x0D
#define RIOTS_SLEEP               0x0E

#define RIOTS_UBER_FAIL           0xFF
#define RIOTS_EMPTY               0xFF

#define RIOTS_SENSOR_FAIL         -9999

#define RIOTS_DATA_AVAILABLE      0x10

#define RIOTS_INDICATE_LEDS             0x01
#define RIOTS_FLASH_COLOR               0xFF9933
#define RIOTS_CONNECTION_FAIL_COLOR     0x870019
#define RIOTS_CONNECTION_OK_COLOR       0x008000
#define RIOTS_ORANGE_COLOR              0xFF8000
#define RIOTS_BLUE_COLOR                0x1E90FF
#define RIOTS_CLOUD_FAIL_COLOR          0x33FFFF
#define RIOTS_MAMA_SAVE_DATA_COLOR      0x0000FF
#define RIOTS_MAMA_NO_IP_COLOR          0xFF0000
#define RIOTS_CLOUD_INITALIZING_COLOR   0xFFE600
#define RIOTS_MAGENTA_COLOR             0x330033
#define RIOTS_CYAN_COLOR                0x003333

#define RIOTS_HOUR                     3600 // 1 hour
#define RIOTS_SECOND                   1    // 1 second

 // Base specific defines
#define RIOTS_HOURLY_REPORTING_INTERVAL         3600 // 1 hour
#define RIOTS_LOW_RESOLUTION                    1

#define RIOTS_AIR_MEASUREMENT_INTERVAL          5    // seconds
#define RIOTS_AIR_BATTERY_MEASUREMENT_INTERVAL  900  // 15 min
#define RIOTS_AIR_TEMP_RESOLUTION               3
#define RIOTS_AIR_HUMI_RESOLUTION               3
#define RIOTS_AIR_PRES_RESOLUTION               30

 // Riots I/Os
#define RIOTS_IO_0                            0
#define RIOTS_IO_1                            1
#define RIOTS_IO_2                            2
#define RIOTS_IO_3                            3
#define RIOTS_IO_4                            4
#define RIOTS_IO_5                            5
#define RIOTS_IO_6                            6
#define RIOTS_IO_7                            7

 // SIM800
#define RIOTS_AT_COMMAND_TIMEOUT              7000
#define AT                                    0x01
#define AT_CPIN                               0x02
#define AT_CIPMODE                            0x03
#define AT_CSTT                               0x04
#define AT_CIICR                              0x05
#define AT_CIFSR                              0x06
#define AT_CIPSTART                           0x07
#define AT_CREG                               0x08
#define AT_PPP                                0x09
#define AT_ATO                                0x0A

#define CELLULAR_CONNECTION_RETRY_TIME        60000

/* Converts Arduino pin to right AVR PORT */
#define __pinToPort(P) \
(((P) <= 7) ? &PORTD : (((P) <= 13) ? &PORTB : &PORTC))

/* Converts Arduino pin to right AVR DDR */
#define __pinToDDR(P) \
(((P) <= 7) ? &DDRD : (((P) <= 13) ? &DDRB : &DDRC))

/* Converts Arduino pin to right AVR PIN */
#define __pinToPIN(P) \
(((P) <= 7) ? &PIND : (((P) <= 13) ? &PINB : &PINC))

/* Converts Arduino pin to right pin bit */
#define __pinToBit(P) \
(((P) <= 7) ? (P) : (((P) <= 13) ? (P) - 8 : (P) - 14))

#define digitalWriteFast(P, V) bitWrite(*__pinToPort(P), __pinToBit(P), (V));
#define pinModeFast(P, V) bitWrite(*__pinToDDR(P), __pinToBit(P), (V));


#ifndef RIOTS_RADIO_DEBUG
  // uncomment following to enable tracing
 // #define RIOTS_RADIO_DEBUG
#endif

#ifndef RIOTS_RADIO_EXT_DEBUG
  // uncomment following to enable extended tracing
  //#define RIOTS_RADIO_EXT_DEBUG
#endif

#ifndef RIOTS_FLASH_MODE
  // comment following to enable flash mode
  // #define RIOTS_FLASH_MODE
#endif

// RIOTS_DEBUG_TYPE_DEFINED flasg is for use using correct tracing for Serial Mama
#ifndef RIOTS_DEBUG_TYPE_DEFINED
#define RIOTS_DEBUG_TYPE_DEFINED
  #ifdef RIOTS_RADIO_DEBUG
    #define _DEBUG_BEGIN(args...) Serial.begin(38400)
    #define _DEBUG_PRINT(args...) Serial.print(args)
    #define _DEBUG_PRINTLN(args...) Serial.println(args)
  #else
    #define _DEBUG_BEGIN(args...)
    #define _DEBUG_PRINT(args...)
    #define _DEBUG_PRINTLN(args...)
  #endif
#endif

#if defined(RIOTS_RADIO_EXT_DEBUG) && defined(RIOTS_RADIO_DEBUG)
  #define _DEBUG_EXT_BEGIN(args...) Serial.begin(args)
  #define _DEBUG_EXT_PRINT(args...) Serial.print(args)
  #define _DEBUG_EXT_PRINTLN(args...) Serial.println(args)
#else
  #define _DEBUG_EXT_BEGIN(args...)
  #define _DEBUG_EXT_PRINT(args...)
  #define _DEBUG_EXT_PRINTLN(args...)
#endif

#endif
