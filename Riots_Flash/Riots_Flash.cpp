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
#include "Riots_Flash.h"
#include "Riots_Helper.h"
#include <EEPROM.h>

/**
 * Setup flash library.
 *
 * @return reset_pin  RIOTS_OK, if the flash library is initialized correctly.
 */
uint8_t Riots_Flash::setup() {

  page_address = 0;
  packet_counter = 0;
  flashbuffer_index = 0;
  reported  = false;
  eeprom_status = RIOTS_FAIL;
  next_boot_status=RIOTS_EMPTY;
  firmware_written=0;

  if(riots_memory.setup(RIOTS_PRIMARY_EEPROM)) {
    eeprom_status = RIOTS_OK;
    return RIOTS_OK; // Memory setup ok
  }

  return RIOTS_FAIL; // Memory setup failed
}

/**
 * Setup function sets given ce, csn, irq and reset pins.
 * If arguments are not given function reads pin values from EEPROM
 * If pin values are not found from EEPROM, function sets hardcoded values (v05)
 *
 * @param type        Type of the message
 * @param plain       Message in the plain text
 * @param response_needed Will be set to true, if RIOTS product needs to reply to this message
 * @return reset_pin  RIOTS_OK, if the message was correct and handled
 */
uint8_t Riots_Flash::handleFlashMessage(uint8_t type, uint8_t *plain, bool *response_needed) {

  switch (type) {
    case TYPE_ENTER_PROGMODE:
      *response_needed = true;

      _DEBUG_PRINTLN(F(" TYPE_ENTER_PROGMODE"));
      if (plain[M_LENGTH] != 0x1) {
        _DEBUG_PRINTLN(F(" incorrect data length"));
        return RIOTS_FAIL;
      }

      // Save next boot status
      next_boot_status = plain[M_VALUE];
      // Clear firmware written
      firmware_written = 0;
      // Set previous page address to 0xFFFF
      page_address_previous = 65535;

      if(eeprom_status == RIOTS_FAIL || next_boot_status==BOOT_LOAD_FAILSAFE) {
        /* Base not connected or failsafe boot requested => reset to bootloader */
        _DEBUG_PRINTLN(F(" BOOT_LOAD_FAILSAFE"));
        EEPROM.write(EEPROM_BOOT_STATUS, BOOT_LOAD_FAILSAFE);

        return RIOTS_RESET;
      }

      if(next_boot_status==BOOT_LOAD_OFFICIAL) {
        /* store official image offset in I2C EEPROM */
        i2c_offset=I2C_EEPROM_O_FW;
      }

      else if(next_boot_status==BOOT_LOAD_UNOFFICIAL) {
        /* store unofficial image offset in I2C EEPROM */
        i2c_offset=I2C_EEPROM_UO_FW;
      }

      else  {
        next_boot_status=RIOTS_EMPTY;
        return RIOTS_FAIL;
      }
      break;

    case TYPE_PROG_FLASH:
      *response_needed = true;

      _DEBUG_PRINTLN(F(" TYPE_PROG_FLASH"));
      if (plain[M_LENGTH] != 0x2) {
        _DEBUG_PRINTLN(F(" incorrect data length"));
        return RIOTS_FAIL;
      }
      if(next_boot_status==RIOTS_EMPTY) {
        _DEBUG_PRINTLN(F(" incorrect boot status"));
        return RIOTS_FAIL;
      }

      /* store firmware size to be flashed */
      firmware_size = (plain[M_VALUE]<<8) | plain[M_VALUE+1];


      /* Corrupt (fill with 0x00) image in I2C eeprom to be flashed
         If wdt or button reset occurs during flash process, device will stay in booloader */
      if(next_boot_status==BOOT_LOAD_OFFICIAL) {
        riots_memory.startPageWrite(I2C_EEPROM_O_FW_ID, RIOTS_PRIMARY_EEPROM);
      }
      else if(next_boot_status==BOOT_LOAD_UNOFFICIAL) {
        riots_memory.startPageWrite(I2C_EEPROM_UO_FW_ID, RIOTS_PRIMARY_EEPROM);
      }
      for(uint8_t i=0; i<6; i++) {
        riots_memory.pageFill(0x00);
      }
      riots_memory.stopPageWrite();

      break;


    case TYPE_LOAD_ADDRESS:
      _DEBUG_PRINTLN(F(" TYPE_LOAD_ADDRESS"));

      // Check message length
      if (plain[M_LENGTH] != 0x2) {
        _DEBUG_PRINTLN(F(" incorrect data length"));
        return RIOTS_FAIL;
      }

      if(next_boot_status==RIOTS_EMPTY) {
        _DEBUG_PRINTLN(F(" incorrect boot status"));
        return RIOTS_FAIL;
      }

      /* Store page address to be written next in I2C EEPROM */
      page_address = (plain[M_VALUE]<<8) | plain[M_VALUE+1];

      /* Check if this is a new load address */
      if (page_address_previous != page_address) {
        _DEBUG_PRINTLN(F(" new page"));
        new_page = true;
      }
      else {
        _DEBUG_PRINTLN(F(" old page"));
        new_page = false;
      }

      page_address += i2c_offset;

      flashbuffer_index = 0;
      packet_counter = 0;
      reported = false;
      break;

    case TYPE_PROG_PAGE:
      _DEBUG_PRINTLN(F(" TYPE_PROG_PAGE"));

      if(next_boot_status==RIOTS_EMPTY) {
        _DEBUG_PRINTLN(F(" incorrect boot status"));
        return RIOTS_FAIL;
      }

      uint8_t length;
      length = ((plain[M_LENGTH])-1);
      if(plain[M_VALUE] == packet_counter && (flashbuffer_index+length) <= I2C_EEPROM_PAGE_SIZE) {
        memcpy(&flash_buffer[flashbuffer_index], &plain[M_VALUE+1], length);
        flashbuffer_index+=length;
        packet_counter++;
        if(flashbuffer_index == I2C_EEPROM_PAGE_SIZE) {
          if (new_page) {
            writeEepromPage();
            firmware_written += I2C_EEPROM_PAGE_SIZE;
            page_address_previous = page_address - i2c_offset;
          }
          *response_needed = true;
        }
      }

      else if ( plain[M_VALUE] < packet_counter ) {
        // Previous flash package received second time - do nothing
        _DEBUG_PRINTLN(F(" ERROR: Flash package already received, skipping"));
      }

      else {
        // Flash package missed or buffer overflow
        _DEBUG_PRINTLN(F(" ERROR: Flash package missed, reporting to cloud"));
        if (!reported) {
          *response_needed = true;
          reported = true;
        }
        return RIOTS_FAIL;
      }

      break;

    case TYPE_LEAVE_PROGMODE:
      *response_needed = true;
      _DEBUG_PRINTLN(F(" TYPE_LEAVE_PROGMODE"));
      if (plain[M_LENGTH] != 0x4) {
        _DEBUG_PRINTLN(F(" incorrect data length"));
        return RIOTS_FAIL;
      }

      if(next_boot_status==RIOTS_EMPTY) {
        _DEBUG_PRINTLN(F(" incorrect boot status"));
        return RIOTS_FAIL;
      }

      // Check that correct amount of data is written
      // Check is skipped in case of official image as no flashing is done in this case
      if(firmware_written!=firmware_size && firmware_written>0) {
        _DEBUG_PRINTLN(F(" all data not written"));
        return RIOTS_FAIL;
      }

      // Write boot status to EEPROM
      EEPROM.write(EEPROM_BOOT_STATUS, next_boot_status);

      // Write firmware ID and length to I2C EEPROM and perform reset
      if(next_boot_status==BOOT_LOAD_OFFICIAL) {
        _DEBUG_PRINTLN(F(" BOOT_LOAD_OFFICIAL"));
        riots_memory.startPageWrite(I2C_EEPROM_O_FW_ID, RIOTS_PRIMARY_EEPROM);
      }
      else if(next_boot_status==BOOT_LOAD_UNOFFICIAL) {
        _DEBUG_PRINTLN(F(" BOOT_LOAD_UNOFFICIAL"));
        riots_memory.startPageWrite(I2C_EEPROM_UO_FW_ID, RIOTS_PRIMARY_EEPROM);
      }

      for(uint8_t i=0; i<4; i++) {
        riots_memory.pageFill(plain[M_VALUE+i]);
      }
      riots_memory.pageFill((firmware_size>>8) & 0xFF);
      riots_memory.pageFill(firmware_size & 0xFF);
      riots_memory.stopPageWrite();

      return RIOTS_RESET;

      break;

    default:
      _DEBUG_PRINTLN(F(" Unknown message type"));
      return RIOTS_FAIL;
  }
  return RIOTS_OK;
}

/**
 * Writes page to the EEPROM.
 *
 */
void Riots_Flash::writeEepromPage() {
  _DEBUG_PRINTLN(F("Write page to EEPROM"));

  /* sanity check */
  if(page_address >= I2C_EEPROM_O_FW && page_address < I2C_EEPROM_FREE_SPACE) {
    riots_memory.startPageWrite(page_address, RIOTS_PRIMARY_EEPROM);
    for(uint8_t i=0; i<I2C_EEPROM_PAGE_SIZE; i++) {
      riots_memory.pageFill(flash_buffer[i]);
    }
    riots_memory.stopPageWrite();
  }
}
