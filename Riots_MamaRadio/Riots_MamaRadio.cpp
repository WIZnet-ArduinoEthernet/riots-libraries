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
#include "Arduino.h"
#include <EEPROM.h>

#include "aes.h"
#include "nRF24L01.h"
#include <avr/wdt.h>
#include <avr/sleep.h>

#include <SPI.h>

#include "Riots_MamaRadio.h"
#include "Riots_Mamadef.h"
#include "Riots_Helper.h"

/**
 * Setup function sets given ce, csn, irq and reset pins.
 * If arguments are not given function reads pin values from EEPROM
 * If pin values are not found from EEPROM, function sets hardcoded values (v05)
 *
 * @param debug               Enable debugging
 * @param nrfce               Chip Enable Activates RX or TX mode
 * @param nrfcsn              SPI Chip Select
 * @param nrfirq              Maskable interrupt pin. Active low
 * @param nrfrst              Reset pin
 */
void Riots_MamaRadio::setup(byte indicativeLedsOn, byte debug, int nrfce, int nrfcsn, int nrfirq, int nrfrst) {

  resetPin = riots_radio.setup(nrfce, nrfcsn, nrfirq, nrfrst);

  if ( indicativeLedsOn == RIOTS_INDICATE_LEDS ) {
    riots_RGBLed.setup();
    indicativeLeds = 1;
  }

  // Read ptrs from the hardware library
  plain_data      = riots_radio.getPlainDataAddress();
  tx_crypt_buff   = riots_radio.getTXCryptBuffAddress();
  rx_crypt_buff   = riots_radio.getRXCryptBuffAddress();
  unique_aes      = riots_radio.getPrivateKeyAddress();
  shared_aes      = riots_radio.getSharedKeyAddress();
  own_address     = riots_radio.getOwnRadioAddress();

  // If this is First Boot Firmware ID has been changed on EEPROM
  if (EEPROM.read(EEPROM_FIRST_BOOT) == 1) {

    // Copy current Firmware ID from one EEPROM location to another
    for (uint8_t i=0; i<4; i++) {
      EEPROM.write((EEPROM_MAMA_FIRMWARE_ID+i), (EEPROM.read(EEPROM_FIRMWARE_ID+i)));
    }
    EEPROM.write(EEPROM_FIRMWARE_ID, 0xFF);
    EEPROM.write(EEPROM_FIRST_BOOT, 0);
  }

  for (int i=0; i<2; i++) {
    // Read Child id from EEPROM
    childId[i] = EEPROM.read(EEPROM_CHILD_ID+i);

    // Read counter from EEPROM
    configCounter[i] = EEPROM.read(EEPROM_COUNTER+i);
  }

  first_aes_part_received = false;
  alive_msg_sent = false;
  mama_reset_acknowledged = false;

  riots_flash.setup();
}

/**
 * Set receiver address of the radio.
 *
 * @return                    aaaaaa
 */
void Riots_MamaRadio::setRadioReceiverAddress(byte *new_address) {

  if (memcmp(own_address, new_address, RF_ADDRESS_SIZE) == 0) {
    // next message is for me as my own address was received.
    own_config_message = true;
  }
  else {
    // store tx address if core not reached message is needed
    memcpy(childTxAddr, new_address, RF_ADDRESS_SIZE);

    // Update new address to the radio
    riots_radio.setTXAddress(new_address);
    own_config_message = false;
  }
}

/**
 * Returns memory address of the plain data buffer
 *
 * @return                    Address of the plain data buffer array
 */
byte* Riots_MamaRadio::getPlainDataAddress() {
  return plain_data;
}

/**
 * Returns memory address of the crypted data buffer
 *
 * @return                    Address of the plain data buffer array
 */
byte* Riots_MamaRadio::getTXCryptBuffAddress() {
  return tx_crypt_buff;
}

/**
 * Returns memory address of the crypted data buffer
 *
 * @return                    Address of the plain data buffer array
 */
byte* Riots_MamaRadio::getRXCryptBuffAddress() {
  return rx_crypt_buff;
}

/**
 * Returns memory address of the private key
 *
 * @return                    Address of the private key array
 */
byte* Riots_MamaRadio::getPrivateKeyAddress() {
  return unique_aes;
}

/**
* Check possible updates from the Riots network.
*
* With parameter it is possible to activate sleeping mode if there is no data
* in the radio.
*
* @param sleep                Status of wait.
* @return byte                true, if data is read successfully
*/
byte Riots_MamaRadio::update(byte sleep) {

  // Have we received data from radio hardware?
  return riots_radio.update(sleep);
}

/**
* Handles the received message. The message should be either forwarded to RIOTS network
* or it will hold the configuration information for mama.
*
* @return bool                Information if the MAMA needs to send answer to the CLOUD
*/
byte Riots_MamaRadio::processMsg(bool *reply_needed) {

  if (own_config_message) {
    return handleOwnConfigMessage(reply_needed);
  }
  else {
  // Copy to TX buffer as we are going to forward this
    memcpy(tx_crypt_buff, rx_crypt_buff, RF_PAYLOAD_SIZE);
    return riots_radio.send();
  }
}

/**
* Decrytps and checks the message validity with CRC and lenght status.
*
* @return bool                RIOTS_OK if received message is valid, otherwise error code
*/
byte Riots_MamaRadio::checkRiotsMsgValidity() {
  // use a shared key for decrypting the crypted message
  // this function will make the check for the msg as well
  return riots_radio.decrypt(shared_aes);
}

/**
* Fills plain_data buffer with the core not reached message information.
*
*/
void Riots_MamaRadio::createCoreNotReachedMsg() {
  createResponse(TYPE_CORE_NOT_REACHED);
}

/**
* Handler for the MAMA's configuration message.
*
* @return bool                Information if the MAMA needs to send answer to the CLOUD
*/
byte Riots_MamaRadio::handleOwnConfigMessage(bool *reply_needed) {
  byte status = RIOTS_FAIL;

  // Plain data holds private crypted data
  if ( RIOTS_OK == riots_radio.decrypt(unique_aes) ) {
    last_message_type = plain_data[0];
    status = RIOTS_OK;
    switch ( last_message_type ) {
      case TYPE_INIT_AES_PART1:
      case TYPE_AES_PART1:
        status = checkCounter();
        if ( status == RIOTS_OK ) {
          riots_radio.saveNewAesKey(0, plain_data+2);
          first_aes_part_received = true;
        }
      break;

      case TYPE_INIT_AES_PART2:
      case TYPE_AES_PART2:
        status = checkCounter();
        if ( status == RIOTS_OK && first_aes_part_received ) {
          riots_radio.saveNewAesKey(1, plain_data+2);
          riots_radio.activateNewAesKey();
          first_aes_part_received = false;
        }
        *reply_needed = true;
        // Confirm part2 as we have now received the whole aes key
        createResponse(TYPE_CONFIRM_CONFIG, TYPE_AES_PART2, status);
      break;

      case TYPE_INIT_MAMA:
        // Agree the child id and counter value and use them
        childId[0]        = plain_data[M_CHILD_ID];
        childId[1]        = plain_data[M_CHILD_ID+1];
        configCounter[0]  = plain_data[M_COUNTER];
        configCounter[1]  = plain_data[M_COUNTER+1];

        // Save values to eeprom
        EEPROM.write(EEPROM_CHILD_ID,   plain_data[M_CHILD_ID]);
        EEPROM.write(EEPROM_CHILD_ID+1, plain_data[M_CHILD_ID+1]);
        EEPROM.write(EEPROM_COUNTER,    plain_data[M_COUNTER]);
        EEPROM.write(EEPROM_COUNTER+1,  plain_data[M_COUNTER+1]);
        *reply_needed = true;

        createResponse(TYPE_IM_ALIVE);
      break;

      case TYPE_ENTER_PROGMODE:  /* Fall through, flash related messages are handled in flash library */
      case TYPE_LEAVE_PROGMODE:
      case TYPE_LOAD_ADDRESS:
      case TYPE_PROG_FLASH:
        status = checkCounter();
      case TYPE_PROG_PAGE:
        status = riots_flash.handleFlashMessage(last_message_type, plain_data, reply_needed );
        if ( *reply_needed ) {
          *reply_needed = CONFIRM_CONFIG;
          if ( last_message_type == TYPE_PROG_PAGE ) {
            // Confirm LOAD_ADDRESS configuration after all packages has been received successfully
            // or when first failure happens
            if (indicativeLeds == 1) {
              // indicate with the led
              riots_RGBLed.setColor(RIOTS_FLASH_COLOR);
            }
            last_message_type = TYPE_LOAD_ADDRESS;
          }

          if ( status == RIOTS_RESET && last_message_type == TYPE_LEAVE_PROGMODE ) {
            // Reboot to bootloader needs to happen after this message is delivered
            mama_reset_acknowledged = true;
          }
          createResponse(TYPE_CONFIRM_CONFIG, last_message_type, status);
        }
      break;

      default:
      /*_DEBUG_PRINTLN(F("NOT SUPPORTED"));
      // Read Child id from EEPROM
      _DEBUG_PRINT(F(":"));
      for (int i=0; i<RF_PAYLOAD_SIZE; i++) {
        _DEBUG_PRINT(plain_data[i], HEX);
        _DEBUG_PRINT(F(" "));
      }*/
      break;
    }
  }
  return status;
}

/**
* Informs that RIOTS message has been sent to cloud.
*
* @param sucess               RIOTS_OK if message was delivered succesfully.
* @return boolean             TRUE, if message is was alive message.
*/
bool Riots_MamaRadio::messageDelivered(byte success) {

  if (indicativeLeds == 1) {
    riots_RGBLed.setColor(RIOTS_BLUE_COLOR);
  }

  if ( success == RIOTS_OK && last_message_type == TYPE_LEAVE_PROGMODE && mama_reset_acknowledged ) {
    // reboot
    digitalWriteFast(resetPin, LOW);
  }
  if ( alive_msg_sent )
  {
    alive_msg_sent = false;
    if ( success == RIOTS_OK ) {
    // inform that this was a I'm alive message
      return true;
    }
  }
  return false;
}

/**
* Creates a response to the cloud. Packs the message and decrypts it.
*
* @param answer_type          Type of the cloud message.
* @param message_type         Type of the message.
* @param status               Status of the message.
*/
void Riots_MamaRadio::createResponse(byte answer_type, byte message_type, byte status) {

  for (int i=M_VALUE+2; i < RF_PAYLOAD_SIZE-5; i++) {
    plain_data[i] = random(255);
  }

  plain_data[M_TYPE]        = answer_type;

  // add counter
  plain_data[M_COUNTER]     = configCounter[0];
  plain_data[M_COUNTER+1]   = configCounter[1];

  // add child id
  plain_data[M_CHILD_ID]    = childId[0];
  plain_data[M_CHILD_ID+1]  = childId[1];

  if (answer_type == TYPE_CONFIRM_CONFIG) {
    plain_data[M_LENGTH]    = 0x2;
    plain_data[M_VALUE]     = message_type; // Received message type
    plain_data[M_STATUS]    = status;
  }

  else if (answer_type == TYPE_CORE_NOT_REACHED) {
    plain_data[M_LENGTH]    = CORE_NOT_REACHED_LEN;
    memcpy(&plain_data[M_VALUE], &childTxAddr[0], RF_ADDRESS_SIZE);
  }

  else if (answer_type == TYPE_IM_ALIVE) {
    alive_msg_sent = true;
    plain_data[M_LENGTH]    = IM_ALIVE_LEN;

    // Read Firmware ID from EEPROM
    for(int i=0; i<4; i++) {
      plain_data[M_VALUE+i] = EEPROM.read(EEPROM_MAMA_FIRMWARE_ID+i);
    }

    // Read Base ID from EEPROM
    for(int i=0; i<4; i++) {
      plain_data[M_VALUE2+i] = EEPROM.read(EEPROM_BASE_ID+i);
    }
  }
  else {
    return;
  }

  // Add checksum
  byte checksum = 0;
  for (int i=0; i<M_LAST_DIGIT; i++) {
    checksum = checksum^plain_data[i];
  }
  plain_data[M_LAST_DIGIT]  = checksum;
}

/**
* Checks received counter validity and save it to memory
*
*/
byte Riots_MamaRadio::checkCounter() {
  // Read counter from EEPROM
  configCounter[0] = EEPROM.read(EEPROM_COUNTER);
  configCounter[1] = EEPROM.read(EEPROM_COUNTER+1);

  // Check the status (allow all values after 0xFFF0)
  if (memcmp(plain_data + 11, configCounter, 2) >= 0) {
    //Write received counter to EEPROM
    EEPROM.write(EEPROM_COUNTER, plain_data[M_COUNTER]);
    EEPROM.write(EEPROM_COUNTER+1, plain_data[M_COUNTER+1]);

    // Save current counter to class member variable
    configCounter[0] = plain_data[M_COUNTER];
    configCounter[1] = plain_data[M_COUNTER+1];
    return RIOTS_OK;
  }
  return RIOTS_FAIL;
}
