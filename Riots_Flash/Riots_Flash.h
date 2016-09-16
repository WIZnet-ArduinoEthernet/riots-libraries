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
#ifndef Riots_Flash_H
#define Riots_Flash_H

#include "Arduino.h"
#include "Riots_Memory.h"

class Riots_Flash {
  public:
    uint8_t handleFlashMessage(uint8_t type, uint8_t *plain, bool *response_needed);
    uint8_t setup();
  private:

    void writeEepromPage ();
    uint16_t page_address;              /*!< Address of page to write next                                                        */
    uint16_t page_address_previous;     /*!< Address of previously written page                                                   */
    uint16_t i2c_offset;                /*!< Address offset (depending on do we write official or unofficial image                */
    uint8_t flash_buffer[128];          /*!< Buffer to store data to be written to EEPROM                                         */
    uint16_t firmware_size;             /*!< Size of the firmware image to be written to EEPROM                                   */
    uint16_t firmware_written;          /*!< Amount of uint8_ts to written to EEPROM                                              */
    uint8_t packet_counter;             /*!< Index of next packet to be received with STK_PROG_PAGE                               */
    uint8_t flashbuffer_index;          /*!< Index of uint8_t to be copied to flash_buffer                                        */
    uint8_t next_boot_status;           /*!< Tells what will be the next boot mode (BOOT_LOAD_OFFICIAL, _UNOFFICIAL OR _FAILSAFE) */
    uint8_t eeprom_status;              /*!< Status of external EEPROM (connected or not)                                         */
    bool reported;                      /*!< Do we have already reported status of the package to the cloud                       */
    bool new_page;                      /*!< Do we have started a new page writing                                                */
    Riots_Memory riots_memory;          /*!< Instance of the RIOTS memory class                                                   */
};

#endif //Riots_Flash_H
