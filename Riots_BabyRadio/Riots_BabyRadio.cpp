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

/* Riots Radio library */
#include "Arduino.h"
#include <EEPROM.h>
#include <SPI.h>
#include <avr/wdt.h>
#include <avr/sleep.h>
#include "aes.h"

#include "Riots_Radio.h"
#include "Riots_BabyRadio.h"

/**
 *
 * @param debug     Enable debugging
 * @param nrfce     Chip Enable Activates RX or TX mode
 * @param nrfcsn    SPI Chip Select
 * @param nrfirq    Maskable interrupt pin. Active low
 * @param nrfrst    Reset pin
 */
void Riots_BabyRadio::setup(byte indicativeLedsOn, byte debug, int nrfce, int nrfcsn, int nrfirq, int nrfrst) {

  // Setting up the possible enabled debugger interface
  if (debug == 1) {
    Serial.begin(38400);
    Serial.println(F("Starting Riots Wireless debugger"));
    debugger = 1;
  }
  else {
    debugger = 0;
  }

  // Setup radio hardware library and get reset pin
  reset_pin = riots_radio.setup(nrfce, nrfcsn, nrfirq, nrfrst);

  if ( indicativeLedsOn == RIOTS_INDICATE_LEDS ) {
    riots_RGBLed.setup();
    indicativeLeds = 1;
  }

  // Read ptrs from the hardware library
  plain_data = riots_radio.getPlainDataAddress();
  tx_crypt_buff = riots_radio.getTXCryptBuffAddress();
  rx_crypt_buff = riots_radio.getRXCryptBuffAddress();
  unique_aes = riots_radio.getPrivateKeyAddress();
  shared_aes = riots_radio.getSharedKeyAddress();
  own_address = riots_radio.getOwnRadioAddress();


  if (EEPROM.read(EEPROM_FIRST_BOOT) == 1) {

    // Check if Base has been changed

    // Read Base ID from EEPROM
    for(int i=0; i<4; i++) {
      plain_data[i] = EEPROM.read(EEPROM_BASE_ID+i);
    }
    // Read Previous Base ID from EEPROM
    for(int i=0; i<4; i++) {
      plain_data[i+4] = EEPROM.read(PREV_BASE_ID+i);
    }

    if((memcmp(&plain_data[0], &plain_data[4], 4) != 0)) {

      // Clear sleep and ring configures from EEPROM
      EEPROM.write(EEPROM_SLEEP_ENABLED, 0);
      EEPROM.write(EEPROM_IO_INDEX, 0);
      EEPROM.write(EEPROM_FIRST_BOOT, 0);

      // Write Previous Base ID to EEPROM
      for(int i=0; i<4; i++) {
        EEPROM.write((PREV_BASE_ID+i), plain_data[i]);
      }
    }
  }

  // Read Mama, Child, Debug, Next and Prev addresses from EEPROM
  _DEBUG_PRINTLN(F("RX addresses:"));
  _DEBUG_PRINTLN(F("MA CA DA RP RN"));
  for (int i=0; i<4; i++) {
    MA[i] = EEPROM.read(EEPROM_MAMA_ADDR+i);
    CA[i] = EEPROM.read(EEPROM_CHILD_ADDR+i);
    DA[i] = EEPROM.read(EEPROM_DEBUG_ADDR+i);
    RP[i] = EEPROM.read(EEPROM_RING_PREV+i);
    RN[i] = EEPROM.read(EEPROM_RING_NEXT+i);
    _DEBUG_PRINT(MA[i], HEX);
    _DEBUG_PRINT(F(" "));
    _DEBUG_PRINT(CA[i], HEX);
    _DEBUG_PRINT(F(" "));
    _DEBUG_PRINT(DA[i], HEX);
    _DEBUG_PRINT(F(" "));
    _DEBUG_PRINT(RP[i], HEX);
    _DEBUG_PRINT(F(" "));
    _DEBUG_PRINT(RN[i], HEX);
    _DEBUG_PRINTLN(F(" "));
  }

  for (int i=0; i<2; i++) {
    // Read Child id from EEPROM
    childId[i] = EEPROM.read(EEPROM_CHILD_ID+i);
    // Read counter from EEPROM
    counter[i] = EEPROM.read(EEPROM_COUNTER+i);
    RPC[i] = EEPROM.read(EEPROM_RING_PREV_CHILD+i);
    RNC[i] = EEPROM.read(EEPROM_RING_NEXT_CHILD+i);
  }
  dataCounter = 0;

  // initialize first acceptable number
  current_ring_counter = 1;

  // Read Riots Core status from EEPROM
  core_status = EEPROM.read(EEPROM_CORE_STATUS);

  // Read Riots Network status from EEPROM
  net_status = EEPROM.read(EEPROM_NET_STATUS);

  // Read sleep enable status bit from EEPROM
  sleep_enabled = EEPROM.read(EEPROM_SLEEP_ENABLED);
  if(sleep_enabled != 1) { // must be set explicitly to 1 to enable sleep
    sleep_enabled = 0;
  }

  // Not initialized, set to zero
  // TODO: Check if these can be moved to production
  if ( core_status == 0xFF ) {
    core_status = 0x00;
    EEPROM.write(EEPROM_CORE_STATUS, core_status);
  }
  if ( net_status == 0xFF ) {
    net_status = 0x00;
    EEPROM.write(EEPROM_NET_STATUS, net_status);
  }

  // Set first three bits to zero just in case
  // These should not be ever wrtiten to EEPROM
  bitClear(core_status, CORE_INIT_AES1);
  bitClear(core_status, CORE_INIT_AES2);
  bitClear(core_status, CORE_INIT_PARENT);

  _DEBUG_PRINT(F("Riots_BabyRadio::setup core status: "));
  _DEBUG_PRINTLN(core_status, HEX);
  _DEBUG_PRINT(F("Riots_BabyRadio::setup net status: "));
  _DEBUG_PRINTLN(net_status, HEX);

  // Read I/O mapping from EEPROM
  io_index = EEPROM.read(EEPROM_IO_INDEX);
  if (io_index > 8) {
    EEPROM.write(EEPROM_IO_INDEX, 0);
    io_index = 0;
  }
  _DEBUG_PRINT(F("IO index:"));
  _DEBUG_PRINTLN(io_index, HEX);

  _DEBUG_PRINTLN(F("IO table:"));
  for (int i = 0; i<io_index; i++) {
    io_table[0][i] = EEPROM.read(EEPROM_CORE_INDEX+i);
    io_table[1][i] = EEPROM.read(EEPROM_RING_INDEX+i);
    _DEBUG_PRINT(io_table[0][i], HEX);
    _DEBUG_PRINT(F(" "));
    _DEBUG_PRINTLN(io_table[1][i], HEX);
  }

  // Initialize send status
  send_status = RIOTS_OK;

  // Initialize ring data status
  ring_data_available = false;

  // There is no ongoing ring messages yet
  own_ring_event_ongoing = false;


#ifdef RIOTS_FLASH_MODE
  flash_mode = 0;
#endif

  // Set last alive time and sleep count to zero
  last_alive_time = 0;
  sleep_count = 0;

  // Initialize Riots Flash
  base_attached = riots_flash.setup();

  // Send I'm alive if cloud is available
  sendMessage( TYPE_IM_ALIVE, RIOTS_OK );

  _DEBUG_PRINTLN(F("Riots_BabyRadio::setup done"));
}

/**
* Sends a given message to Ring and Cloud if available
*
* @param index      Data index (I/O)
* @param data       Data to Cloud
* @param factor     Multiplier for data value
* @return byte      Returns status of sent message

*/
 byte Riots_BabyRadio::send( uint8_t index, int32_t data, int8_t factor ) {

  updateLedStatus(0, RIOTS_ORANGE_COLOR);

  _DEBUG_PRINTLN(F("Riots_BabyRadio::send"));
  byte result;

#ifdef RIOTS_FLASH_MODE
  if ( flash_mode != 0 ) {
    _DEBUG_PRINTLN(F("Riots_BabyRadio::send ERROR: Baby in flash mode"));
    return RIOTS_FAIL;
  }
#endif

  // Change endianess and fill plain_data
  plain_data[M_VALUE+1] = data >> 24;
  plain_data[M_VALUE+2] = data >> 16;
  plain_data[M_VALUE+3] = data >> 8;
  plain_data[M_VALUE+4] = data;

  // Add common data for ring and cloud
  memcpy(plain_data + M_VALUE+5, &factor, 1);

  // Send to ring using Ring IO mapping
  plain_data[M_VALUE] = getRingIO(index);
  if (plain_data[M_VALUE] != RIOTS_UBER_FAIL) {
    // increase the ring counter
    formMessage(TYPE_RING_EVENT, 0x8, 0);
    // TODO get result also from ring forward
    own_ring_event_ongoing = true;
    if ( RIOTS_OK != ringForward() ) {
      own_ring_event_ongoing = false;
    }
  }

  // Send to cloud using plain index
  plain_data[M_VALUE] = index;
  memcpy(plain_data + M_COUNTER, &dataCounter, 2);
  dataCounter += 1;
  formMessage(TYPE_CLOUD_EVENT, 0x6, 0);
  return cloudForward();

}

/**
* Send pending messages from radio and check updates from the Riots network.
*
* With parameter it is possible to activate sleeping mode if there is no data
* in the radio.
*
* @param sleep     Status of wait.
*/
byte Riots_BabyRadio::update(byte sleep) {

  byte retvalue = RIOTS_OK;

  // Check if there is need to send I'm alive message
  if ( (getSeconds() - last_alive_time) > 600 ) {
    // Try to send I'm alive
    sendMessage( TYPE_IM_ALIVE, RIOTS_OK );
    // Update timer
    last_alive_time = getSeconds();
  }

#ifdef RIOTS_FLASH_MODE
  if ( flash_mode == 1 && millis() - flash_start > 150000 ){
    _DEBUG_PRINTLN(F("Riots_BabyRadio::update Leaving flash mode "));
    flash_mode = 0;
  }
#endif

  // Force sleep if enabled from Cloud
  if(sleep_enabled) {
    sleep = 1;
  }

  // Update leds
  updateLedStatus(sleep);

  // Increase sleep count
  if ( sleep == 1 ) {
    sleep_count++;
    riots_radio.update(1);
    retvalue = RIOTS_SLEEP;
  }

  // Check data from Radio
  if ( riots_radio.update(0) == RIOTS_OK ) {
    if ( processMessage() == RIOTS_DATA_AVAILABLE ) {
      _DEBUG_PRINTLN(F("Riots_BabyRadio::update available for INO"));
      retvalue = RIOTS_DATA_AVAILABLE;
    }
  }
  return retvalue;
}

/**
* Get cloud connection status.
*
* @param return     Status of cloud connection.
*/
byte Riots_BabyRadio::getCloudStatus() {
  // If mama address configured and cloud status ok
  if (bitRead(core_status, CORE_MAMA_ADDRESS_SET) && bitRead(net_status, NET_CLOUD_CONNECTION)) {
    return RIOTS_OK;
  }
  return RIOTS_FAIL;
}

/**
* Returns received data to INO
*
* @return int32_t    Data received from Riots network
*/
int32_t Riots_BabyRadio::getData() {

  // index 0 holds ring id and index 5 coefficient factor
  return ((saved_event[1] << 24) + (saved_event[2] << 16) + (saved_event[3] << 8) + saved_event[4]);
}

/**
* Returns received data index to INO
*
* @return int32_t    Data index received from Riots network
*/
uint8_t Riots_BabyRadio::getIndex() {

  return saved_event[0];
}

/**
* How many seconds we have been running since reboot.
*
* @return uint32_t   Current time in seconds.
*/
uint32_t Riots_BabyRadio::getSeconds() {

  return ((millis()/1000) + (8 * sleep_count));

}

/**
* Send pending messages from radio and check updates from the Riots network.
*
* With parameter it is possible to activate sleeping mode if there is no data
* in the radio.
*
* @param type       Type of the message.
* @param length     Length of the data.
* @param value      Array of the data which should be sent
*/
void Riots_BabyRadio::formMessage(byte type, byte length, byte addCounter, byte forward) {
  plain_data[M_TYPE] = type;
  plain_data[M_LENGTH] = length;

  // Add padding
  for (int i=M_VALUE+length; i<RF_PAYLOAD_SIZE-5; i++) {
    plain_data[i] = random(255);
  }

  if (type == TYPE_INIT_STATUS) {
    // Add more padding
    for (int i=M_COUNTER; i<M_LAST_DIGIT; i++) {
      plain_data[i] = random(255);
    }
  }
  else {
    if (addCounter) {
      plain_data[M_COUNTER]   = counter[0];
      plain_data[M_COUNTER+1] = counter[1];
    }
    if ( !forward ) {
      // add child id as this message is originally sent by me
      addChildId(M_CHILD_ID);
    }
  }

  if ( type == TYPE_RING_EVENT || type == TYPE_RING_EVENT_BACK ) {
    // Add own child id to "message body" before sending
    addChildId(M_VALUE+6);
    // Add ring counter as well
    memcpy(plain_data + M_COUNTER, &current_ring_counter, 2);
    // increase the next accetable value
    current_ring_counter++;
  }

  addChecksum();

  _DEBUG_PRINT(F(" Riots_BabyRadio::formMessage Sending Message:"));
  for(int i=0; i< RF_PAYLOAD_SIZE; i++){
    _DEBUG_PRINT(F(" "));
    _DEBUG_PRINT(plain_data[i],HEX);
  }
  _DEBUG_PRINTLN();

  // Encrypt message
  AES128_ECB_encrypt(plain_data, shared_aes, tx_crypt_buff);
}

void Riots_BabyRadio::addChildId(byte start_pos) {
  plain_data[start_pos]   = childId[0];
  plain_data[start_pos+1] = childId[1];
}

void Riots_BabyRadio::addChecksum() {
  byte checksum = 0;
  for (int i=0; i<M_LAST_DIGIT; i++) {
    checksum = checksum^plain_data[i];
  }
  plain_data[M_LAST_DIGIT] = checksum;
}

/**
* Sends written data to serial.
*
* @return byte      Returns status of sent message
*/
byte Riots_BabyRadio::uartSend() {
  //_DEBUG_PRINTLN(F("Riots_BabyRadio::uartSend"));
  // Set uart child address as destination
  riots_radio.setTXAddress(DA);
  // Send data over the radio
  return riots_radio.send();
}

/**
* Forwards the cloud message from the child to next radio address. Either this
* address is a next Child or Mama which will in the end transfer the message
* to the cloud.
*
* @return byte      Returns status of sent message
*/
byte Riots_BabyRadio::cloudForward() {

  byte retries = 0;
  byte delaytime = 0;

  if ( bitRead(core_status, CORE_MAMA_ADDRESS_SET) == 0 ) {
     _DEBUG_PRINTLN(F("Riots_BabyRadio::cloudForward: Error: Mama address not configured"));
     return RIOTS_NO_CLOUD_CONNECTION;
  }

  // Set parent address
  riots_radio.setTXAddress(MA);

  if (riots_radio.send() == RIOTS_OK) {
    _DEBUG_PRINTLN(F("Riots_BabyRadio::cloudForward send status: RIOTS_OK"));
    if ( !bitRead(net_status, NET_CLOUD_CONNECTION) ) {
      // Cloud forward was ok but cloud is disabled, swith internal status
      _DEBUG_PRINTLN(F(" Enabling Cloud"));
      im_alive_fail_count = 0;
      bitSet(net_status, NET_CLOUD_CONNECTION);
      EEPROM.write(EEPROM_NET_STATUS, net_status);
    }
    return RIOTS_OK;
  }
  _DEBUG_PRINT(F(" Error: NO ACK"));

  return RIOTS_FAIL;
}

/**
* Sends a message to next baby in the RIOTS ring.
*
* @return byte      Returns status of sent message
*/
byte Riots_BabyRadio::ringForward() {
  // Set TX to ring next
  riots_radio.setTXAddress(RN);

  // Message send failed
  if (riots_radio.send() != RIOTS_OK) {
    _DEBUG_PRINT(F("Riots_BabyRadio::ringForward sent fail!"));

    // Form ring backward message
    formMessage(TYPE_RING_EVENT_BACK, 0x8);

    ringBackward();

    // Report failure to cloud
    memcpy(plain_data + M_VALUE, RN, 4);
    sendMessage(TYPE_CORE_NOT_REACHED, RIOTS_OK);
    return RIOTS_FAIL;
  }
  _DEBUG_PRINT(F("Riots_BabyRadio::ringForward send ok!"));
  return RIOTS_OK;
}

/**
* Sends a message to prev baby in the RIOTS ring.
*
* @return byte      Returns status of sent message
*/
byte Riots_BabyRadio::ringBackward() {
  // Set TX to ring prev
  riots_radio.setTXAddress(RP);

  if (riots_radio.send() != RIOTS_OK) {
    // Report failure to cloud
    memcpy(plain_data + M_VALUE, RP, 4);
    sendMessage(TYPE_CORE_NOT_REACHED, RIOTS_OK);
    return RIOTS_FAIL;
  }
  _DEBUG_PRINT(F("Riots_BabyRadio::ringBackward send ok!"));
  return RIOTS_OK;
}

/**
* Sends a message to next baby in the route.
*
* @return byte      Returns status of sent message
*/

byte Riots_BabyRadio::routeForward() {

  _DEBUG_PRINT(F("Riots_BabyRadio::routeForward "));

  // Set Child address
  riots_radio.setTXAddress(CA);

  if (riots_radio.send() != RIOTS_OK) {
    _DEBUG_PRINT(F("Riots_BabyRadio::routeForward sent fail!"));
    // Report failure to cloud
    memcpy(plain_data + M_VALUE, CA, 4);
    sendMessage(TYPE_CORE_NOT_REACHED, RIOTS_OK);
    return RIOTS_FAIL;
  }

  return RIOTS_OK;
}


/**
* Process with the message.
*
* Decrypts the data. Try both public and private AES128 keys. Private key
* is used for the config messages, public for the other communication.
*
* @return      true, if message was decrypted successfully and message was valid.
*/
byte Riots_BabyRadio::processMessage() {
  if (riots_radio.decrypt(shared_aes) == RIOTS_OK) {
    // CRC matches with shared key, run deeper check
    if (checkSharedMessageValidity()) {
      return handleSharedMessage();
    }
  }
  if ( riots_radio.decrypt(unique_aes) == RIOTS_OK ) {
    // We were able to open with the private key
    // execute possible command and return type of the message afterwards

    if (validatePrivateMessage() == RIOTS_OK) {
      return handlePrivateMessage();
    }
  }

  _DEBUG_PRINTLN(F("Riots_BabyRadio::processMessage DECRYPT FAILED -> routing message"));

  // Check route status
  if ( bitRead(core_status, CORE_CHILD_ADDRESS_SET) == 0 ) {
    _DEBUG_PRINTLN(F(" Error: Child address not set"));
    return RIOTS_FAIL;
  }

  // copy rx crypt to tx crypt and send
  memcpy(tx_crypt_buff, rx_crypt_buff, RF_PAYLOAD_SIZE);

  // Send message
  return routeForward();
}

/**
* Runs an operation and lenght check for the decrypted message.
*
* @return      true, if decrypted message is valid.
*/
bool Riots_BabyRadio::checkSharedMessageValidity() {
  _DEBUG_PRINTLN(F("checkSharedMessageValidity"));
  // run checks with the type and exact length of the message
  switch (plain_data[M_TYPE]) {
     case TYPE_CONFIRM_CONFIG:
      if ( plain_data[1] == 0x02 ) {
        return true;
      }
    break;

    case TYPE_CORE_NOT_REACHED:
      if ( plain_data[1] == 0x04 ) {
        return true;
      }
    break;

    case TYPE_CLOUD_EVENT:
    case TYPE_CLOUD_EVENT_DOWN:
      if ( plain_data[1] == 0x05 || plain_data[1] == 0x06 ) {
        return true;
      }
    break;

    case TYPE_IM_ALIVE:
      if ( plain_data[1] == 0x04 || plain_data[1] == 0x08 ) {
        return true;
      }
    break;

    case TYPE_RING_EVENT:
    case TYPE_RING_EVENT_BACK:
    case TYPE_INIT_STATUS:
      if ( plain_data[1] == 0x08 ) {
        return true;
      }
    break;

    case TYPE_DEBUG_START:
    case TYPE_DEBUG_DATA:
      if ( plain_data[1] > 0x01 && plain_data[1] < 0x0C ) {
        return true;
      }
    break;

    default:
      return false;
  }
}

/**
* Handles a message opened with the shared AES key.
*
* @return      true, if message was processed succesfully.
*/
byte Riots_BabyRadio::handleSharedMessage() {
  switch (plain_data[M_TYPE]) {
    case TYPE_RING_EVENT:
    case TYPE_RING_EVENT_BACK:
      // Save received event for the further use
      memcpy(saved_event, plain_data+M_VALUE, 6);

      _DEBUG_PRINT(F("Riots_BabyRadio::handleSharedMessage TYPE_RING_EVENT"));
      if (bitRead(net_status, NET_RING_CONNECTION) == 0) {
         _DEBUG_PRINTLN(F("Riots_BabyRadio::handleSharedMessage: Ring not available, or not configured"));
         return RIOTS_RING_NOT_AVAILABLE;
      }
      if (!isRingAddressValid()) {
        // Did not receive it from the correct sender, ignore the message here
        return RIOTS_FAIL;
      }

      // Check if I have sent this message earlier
      if (plain_data[M_CHILD_ID] == childId[0] && plain_data[M_CHILD_ID+1] == childId[1]) {
        _DEBUG_PRINTLN(F(" Ring message sent by me -> ring event STOPPED"));
        own_ring_event_ongoing = false;
        memcpy(&current_ring_counter, plain_data+M_COUNTER, 2);

        // increase next accetable value
        if (plain_data[M_TYPE] == TYPE_RING_EVENT) {
          current_ring_counter++;
        }
        return RIOTS_OK;
      }
      else {
        if (plain_data[M_TYPE] == TYPE_RING_EVENT && !isRingCounterValid()) {
        // Did not receive correct counter from the correct sender, ignore the message here
        return RIOTS_FAIL;
        }
      }

      // Forward message to the next core in the ring
      if (plain_data[M_TYPE] == TYPE_RING_EVENT) {
        formMessage(TYPE_RING_EVENT, 0x8, 0, 1);
        if ( ringForward() != RIOTS_OK && own_ring_event_ongoing ) {
          // Decrease the counter as we were not able to send the ring message
          current_ring_counter--;
        }
      }
      else {
        formMessage(TYPE_RING_EVENT_BACK, 0x8, 0, 1);
        ringBackward();
      }

      // Check IO -> Handle if for me
      if (getMyIO(saved_event[0]) != RIOTS_UBER_FAIL) {
        _DEBUG_PRINTLN(F(" Got data from RING to my IO"));
        return RIOTS_DATA_AVAILABLE;
      }
      return RIOTS_OK;
      break;

    case TYPE_IM_ALIVE: /* FALLTHROUGH */
    case TYPE_INIT_STATUS:
    case TYPE_CORE_NOT_REACHED:
    case TYPE_CONFIRM_CONFIG:
    case TYPE_CLOUD_EVENT:
      _DEBUG_PRINTLN(F("Riots_BabyRadio::handleSharedMessage Routing to Cloud"));
      for (int i=0; i<16; i++){
      _DEBUG_PRINT(plain_data[i],HEX);
      _DEBUG_PRINT(F(" "));
      }
      _DEBUG_PRINTLN();
      // Encrypt message
      AES128_ECB_encrypt(plain_data, shared_aes, tx_crypt_buff);
      // Send message
      return cloudForward();

    break;

    case TYPE_DEBUG_DATA: /* FALLTHROUGH */
    case TYPE_DEBUG_START:
      if ( debugger == 1 ) {

        updateLedStatus(0, RIOTS_BLUE_COLOR);

        if ( plain_data[M_TYPE] == TYPE_DEBUG_START &&
             ( plain_data[M_LENGTH] > 0x02 && plain_data[M_LENGTH] < 0x0C ) ) {
            /* Print sender information */
          Serial.print(plain_data[M_CHILD_ID]*256 + plain_data[M_CHILD_ID+1]);
          Serial.print(F(","));
          Serial.print(plain_data[M_VALUE]);
          Serial.print(F(": "));
        }
        // Start printing data from the buffer, data is stored M_VALUE + 1 position
        for (int i=1; i < plain_data[M_LENGTH]; i++) {
          Serial.write(plain_data[M_VALUE + i]);
        }
      }
    break;

    default:
      _DEBUG_PRINTLN(F(" Unknown message"));
      return RIOTS_FAIL;

  }
  return RIOTS_OK;
}

/**
* Runs an operation and lenght check for the decrypted private message.
*
* @return      true, if private message is valid.
*/
byte Riots_BabyRadio::validatePrivateMessage() {
  _DEBUG_PRINT(F("Riots_BabyRadio::validatePrivateMessage: "));

  // Check counter
  switch (plain_data[M_TYPE]) {
    case TYPE_CLOUD_EVENT_DOWN:
    case TYPE_ACTIVATE_RING:
    case TYPE_CHILD_ID:
    case TYPE_AES_PART2:
    case TYPE_AES_PART1:
    case TYPE_DEBUG_ADDRESS:
    case TYPE_CHILD_ADDRESS:
    case TYPE_MAMA_ADDRESS:
    case TYPE_SET_ADDRESS_NEXT:
    case TYPE_SET_ADDRESS_PREV:
    case TYPE_SET_BATTERY_OP:
    case TYPE_DEACTIVATE_RING:
    case TYPE_ENTER_PROGMODE:
    case TYPE_LEAVE_PROGMODE:
    case TYPE_LOAD_ADDRESS:
    case TYPE_PROG_FLASH:
      if (checkCounter() != RIOTS_OK){
        _DEBUG_PRINTLN(F(" Counter check failed"));
        return RIOTS_FAIL;
      }
      break;
  }

  // Check message length
  byte length_fail = 0;
  switch (plain_data[M_TYPE]) {
    case TYPE_DEACTIVATE_RING:
      if (plain_data[M_LENGTH] != 0) {
        length_fail = 1;
      }
      break;

    case TYPE_ENTER_PROGMODE:
    case TYPE_SET_BATTERY_OP:
      if (plain_data[M_LENGTH] != 0x01) {
        length_fail = 1;
      }
      break;

    // TODO this is not in specification
    case TYPE_CHILD_ID:
    case TYPE_LOAD_ADDRESS:
    case TYPE_PROG_FLASH:
    case TYPE_INIT_CONFIRM:
      if(plain_data[M_LENGTH] != 0x2) {
        length_fail = 1;
      }
      break;

    case TYPE_MAMA_ADDRESS:
    case TYPE_DEBUG_ADDRESS:
    case TYPE_CHILD_ADDRESS:
    case TYPE_LEAVE_PROGMODE:
      if (plain_data[M_LENGTH] != 0x04) {
        length_fail = 1;
      }
      break;

    case TYPE_INIT_PARENT:
    case TYPE_SET_ADDRESS_PREV:
    case TYPE_SET_ADDRESS_NEXT:
      if (plain_data[M_LENGTH] != 0x06) {
        length_fail = 1;
      }
      break;

    case TYPE_CLOUD_EVENT_DOWN:
      if (plain_data[M_LENGTH] > 0x06 || plain_data[M_LENGTH] < 0x05) {
        length_fail = 1;
      }
      break;

    case TYPE_AES_PART1:
    case TYPE_AES_PART2:
      if (plain_data[M_LENGTH] != 0x08) {
        length_fail = 1;
      }
      break;

    case TYPE_INIT_AES_PART1:
    case TYPE_INIT_AES_PART2:
      if (plain_data[M_LENGTH] != 0x0A) {
        length_fail = 1;
      }
      break;

    case TYPE_ACTIVATE_RING:
      // Check length
      if (plain_data[M_LENGTH] > 0x08) {
        length_fail = 1;
      }
      if (plain_data[M_LENGTH] % 2 != 0) {
        length_fail = 1;
      }
      break;

    case TYPE_PROG_PAGE:
      if (plain_data[M_LENGTH] < 0x09 || plain_data[M_LENGTH] > 0x0D) {
        length_fail = 1;
      }
    break;

    default:
      length_fail = 1;
    break;
  }
  if (length_fail) {
    _DEBUG_PRINTLN(F(" Length check failed"));
    return RIOTS_FAIL;
  }

  return RIOTS_OK;
}

/**
* Checks the type of the message and execute requested command if needed
*
* @return byte      Returns the type of the message if the message structure was
*                   correct.
*/
byte Riots_BabyRadio::handlePrivateMessage() {

  bool response = false;
  byte flash_reply;

  _DEBUG_PRINT(F("Riots_BabyRadio::handlePrivateMessage: "));
  for (int i=0; i<RF_PAYLOAD_SIZE; i++) {
    _DEBUG_PRINT(plain_data[i], HEX);
    _DEBUG_PRINT(F(" "));
  }
  _DEBUG_PRINTLN();

  switch (plain_data[M_TYPE]) {
    case TYPE_SET_BATTERY_OP:
      _DEBUG_PRINTLN(F(" TYPE_SET_BATTERY_OPERATION"));

      sleep_enabled = plain_data[M_VALUE];

      // reply confirm config and start sleeping on next update
      sendMessage(TYPE_CONFIRM_CONFIG, RIOTS_OK);
      break;

    case TYPE_SET_ADDRESS_PREV:
      _DEBUG_PRINTLN(F(" TYPE_SET_ADDRESS_PREV"));

      // Store ring previous address
      for (uint8_t i=0; i<4; i++) {
        if (plain_data[M_VALUE+i] != RP[i]) {
          RP[i] = plain_data[M_VALUE+i];
          EEPROM.write(EEPROM_RING_PREV+i, RP[i]);
        }
      }
      // Store prev child id
      for (uint8_t i=0; i < 2; i++) {
        if (plain_data[M_VALUE+4+i] != RPC[i]) {
          RPC[i] = plain_data[M_VALUE+4+i];
          EEPROM.write(EEPROM_RING_PREV_CHILD+i, RPC[i]);
        }
      }

      // Reply
      sendMessage(TYPE_CONFIRM_CONFIG, RIOTS_OK);
      break;

    case TYPE_SET_ADDRESS_NEXT:
      _DEBUG_PRINTLN(F(" TYPE_SET_ADDRESS_NEXT"));

      // Store ring next address
      for (uint8_t i=0; i<4; i++) {
        if (plain_data[M_VALUE+i] != RN[i]) {
          RN[i] = plain_data[M_VALUE+i];
          EEPROM.write(EEPROM_RING_NEXT+i, RN[i]);
        }
      }
      // Store next child id
      for (uint8_t i=0; i < 2; i++) {
        if (plain_data[M_VALUE+4+i] != RNC[i]) {
          RNC[i] = plain_data[M_VALUE+4+i];
          EEPROM.write(EEPROM_RING_NEXT_CHILD+i, RNC[i]);
        }
      }

      // Reply
      sendMessage(TYPE_CONFIRM_CONFIG, RIOTS_OK);
      break;

    case TYPE_INIT_AES_PART1:
      _DEBUG_PRINTLN(F(" TYPE_INIT_AES_PART1"));

      // Store first part of new AES
      for (int i=0; i<8; i++) {
        aes_update[i] = plain_data[M_VALUE+i];
      }

      // Store new challenge for init sequence
      for (int i=0; i<2; i++) {
        challenge[i] = plain_data[M_VALUE+8+i];
      }

      // Set TYPE_INIT_AES_PART1 received status
      bitSet(core_status, CORE_INIT_AES1);
      break;

    case TYPE_INIT_AES_PART2:
      _DEBUG_PRINTLN(F(" TYPE_INIT_AES_PART2"));

      // Already got challenge
      if (bitRead(core_status, CORE_INIT_AES1)) {
        // Check challenge
        if (memcmp(plain_data+M_VALUE+8, challenge, 2) != 0) {
          _DEBUG_PRINTLN(F(" Challenge doesn't match!"));
          bitClear(core_status, CORE_INIT_AES1);
          return RIOTS_FAIL;
        }
        // Store second part of new AES
        for (int i=0; i<8; i++) {
          aes_update[i+8] = plain_data[M_VALUE+i];
        }
        // Set TYPE_INIT_AES_PART2 received status
        bitSet(core_status, CORE_INIT_AES2);
      }
      break;

    case TYPE_INIT_PARENT:
      _DEBUG_PRINTLN(F(" TYPE_INIT_PARENT"));

      if (bitRead(core_status, CORE_INIT_AES1) && bitRead(core_status, CORE_INIT_AES2)) {
        // Check Challenge
        if (memcmp(plain_data + M_VALUE+4, challenge, 2) != 0) {
          _DEBUG_PRINTLN(F(" Challenge doesn't match!"));
          bitClear(core_status, CORE_INIT_AES1);
          bitClear(core_status, CORE_INIT_AES2);
          return RIOTS_FAIL;
        }
        bitSet(core_status, CORE_INIT_PARENT);
        // TODO move storage to INIT_CONFIRM
        // Store new Mama RX-address
        for (int i=0; i<4; i++) {
          if (plain_data[M_VALUE+i] != MA[i]) {
            MA[i] = plain_data[M_VALUE+i];
            EEPROM.write(EEPROM_MAMA_ADDR+i, MA[i]);
          }
        }
        _DEBUG_PRINT(F(" Using Mama address: "));
        for (int i=0; i<4; i++) {
          _DEBUG_PRINT(MA[i],HEX);
          _DEBUG_PRINT(F(" "));
        }
        _DEBUG_PRINTLN();
        _DEBUG_PRINTLN(F(" Saving old AES key to EEPROM"));
        _DEBUG_PRINT(F(" Using shared AES key: "));
        for (int i=0; i<16; i++) {
          // Store old shared AES key
          EEPROM.write(EEPROM_AES_OLD+i, shared_aes[i]);
          if (shared_aes[i] != aes_update[i]) {
            shared_aes[i] = aes_update[i];
            EEPROM.write(EEPROM_AES_CHANGING+i, aes_update[i]);
          }
          _DEBUG_PRINT(shared_aes[i],HEX);
          _DEBUG_PRINT(F(" "));
        }
        _DEBUG_PRINTLN();

        // Setting cloud status
        _DEBUG_PRINTLN(F(" Cloud status: OK"));
        net_status = 1; // Only first bit (cloud status ok) is set

        core_status = 0x40; // This was new init, only Mama address (bit index 6) is set

        // Write core status to EEPROM
        EEPROM.write(EEPROM_CORE_STATUS, core_status);
        EEPROM.write(EEPROM_NET_STATUS, net_status);
        _DEBUG_PRINT(F(" writing to EEPROM core status: "));
        _DEBUG_PRINTLN(core_status,HEX);
        _DEBUG_PRINT(F(" writing to EEPROM net status: "));
        _DEBUG_PRINTLN(net_status,HEX);

        // Send init status
        memcpy(plain_data + M_VALUE, own_address, 4);
        // Challenge 2 bytes
        memcpy(plain_data + M_VALUE+4, challenge, 2);
        // Generate new 2 byte challenge
        for (int i=0; i<2; i++) {
          challenge[i] = random(255);
        }
        memcpy(plain_data + M_VALUE+6, challenge, 2);
        // Form message
        formMessage(TYPE_INIT_STATUS, INIT_STATUS_LEN);
        // Send message
        cloudForward();
      }
      break;

    case TYPE_INIT_CONFIRM:
      _DEBUG_PRINTLN(F(" TYPE_INIT_CONFIRM"));

      // Check challenge
      if (memcmp(plain_data + M_VALUE, challenge, 2) != 0) {
        _DEBUG_PRINTLN(F(" Challenge NOK"));
        return RIOTS_FAIL;
      }

      // TODO move Mama address and AES key updates here

      // Store counter and Child ID to EEPROM
      for (int i=0; i<2; i++) {
        counter[i] = plain_data[M_COUNTER+i];
        childId[i] = plain_data[M_CHILD_ID+i];
        EEPROM.write(EEPROM_COUNTER+i, counter[i]);
        EEPROM.write(EEPROM_CHILD_ID+i, childId[i]);
      }

      // Send init IDs
      sendMessage(TYPE_IM_ALIVE, RIOTS_OK);
      break;

    case TYPE_MAMA_ADDRESS:
        _DEBUG_PRINTLN(F(" TYPE_MAMA_ADDRESS "));

      // Check changed bytes
      for (int i=0; i<4; i++) {
        _DEBUG_PRINT(plain_data[M_VALUE+i],HEX);
        if (plain_data[M_VALUE+i] != MA[i]) {
          MA[i] = plain_data[M_VALUE+i];
          EEPROM.write(EEPROM_MAMA_ADDR+i, MA[i]);
        }
      }
      // Mama address is now set
      _DEBUG_PRINTLN(F(" New mama address received"));
      bitSet(core_status, CORE_MAMA_ADDRESS_SET);
      EEPROM.write(EEPROM_CORE_STATUS, core_status);
      sendMessage(TYPE_CONFIRM_CONFIG, RIOTS_OK);
      break;

    case TYPE_CHILD_ADDRESS:
      _DEBUG_PRINTLN(F(" TYPE_CHILD_ADDRESS"));

      // Check changed bytes
      for (int i=0; i<4; i++) {
        _DEBUG_PRINT(plain_data[M_VALUE+i],HEX);
        if (plain_data[M_VALUE+i] != CA[i]) {
          CA[i] = plain_data[M_VALUE+i];
          EEPROM.write(EEPROM_CHILD_ADDR+i, CA[i]);
        }
      }
      // Child address is now set
      _DEBUG_PRINT(F(" New child address is set"));
      bitSet(core_status, CORE_CHILD_ADDRESS_SET);
      EEPROM.write(EEPROM_CORE_STATUS, core_status);
      sendMessage(TYPE_CONFIRM_CONFIG, RIOTS_OK);
      break;

    case TYPE_DEBUG_ADDRESS:
      _DEBUG_PRINT(F(" TYPE_DEBUG_ADDRESS "));

      // Check changed bytes
      for (int i=0; i<4; i++) {
        _DEBUG_PRINT(plain_data[M_VALUE+i],HEX);
        if (plain_data[M_VALUE+i] != DA[i]) {
          DA[i] = plain_data[M_VALUE+i];
          EEPROM.write(EEPROM_DEBUG_ADDR+i, DA[i]);
        }
      }
      // Set debug status
      bitSet(net_status, NET_DEBUG_CONNECTION);
      EEPROM.write(EEPROM_NET_STATUS, net_status);

      _DEBUG_PRINTLN(F(" received"));
      sendMessage(TYPE_CONFIRM_CONFIG, RIOTS_OK);
      break;

    case TYPE_AES_PART1: // Set AES key part 1
      _DEBUG_PRINTLN(F(" TYPE_AES_PART1"));

      // Store counter to challenge, this will be used when second AES part is received
      memcpy(challenge, plain_data + M_COUNTER, 2);

      // Store first part of new AES
      for (int i=0; i<8; i++) {
        aes_update[i] = plain_data[M_VALUE+i];
      }
      break;

    case TYPE_AES_PART2:
      _DEBUG_PRINTLN(F(" TYPE_AES_PART2"));

      // Increase challenge by one
      // TODO: implement following in one line
      if (challenge[0] != 0xFF) {
        challenge[0]++;
      } else if (challenge[1] < 0xFF) {
        challenge[1]++;
        challenge[0] = 0x00;
      } else {
        challenge[0] = 0x00;
        challenge[1] = 0x00;
      }

      // Check that counter was jumping only by one
      if (memcmp(challenge, plain_data + M_COUNTER, 2) != 0) {
        _DEBUG_PRINTLN(F(" 2nd AES part challenge check FAILed"));
        return RIOTS_FAIL;
      }

      // Store second part of new AES
      for (int i=0; i<8; i++) {
        aes_update[i+8] = plain_data[M_VALUE+i];
      }
      // Store old shared AES key
      _DEBUG_PRINTLN(F(" Saving old AES key to EEPROM"));
      for (int i=0; i<16; i++) {
          EEPROM.write(EEPROM_AES_OLD+i, shared_aes[i]);
      }
      // Take new shared AES into use
      _DEBUG_PRINTLN(F(" Using new shared AES key"));
      for (int i=0; i<16; i++) {
        if (shared_aes[i] != aes_update[i]) {
          shared_aes[i] = aes_update[i];
          EEPROM.write(EEPROM_AES_CHANGING+i, aes_update[i]);
        }
      }
      sendMessage(TYPE_CONFIRM_CONFIG, RIOTS_OK);
      break;

    case TYPE_CHILD_ID:
      _DEBUG_PRINTLN(F(" TYPE_CHILD_ID"));

      // Check changed bytes
      for (int i=0; i<2; i++) {
        _DEBUG_PRINT(plain_data[M_VALUE+i],HEX);

        if (plain_data[M_VALUE+i] != childId[i]) {
          childId[i] = plain_data[M_VALUE+i];
          EEPROM.write(EEPROM_CHILD_ID+i, childId[i]);
        }
      }
      _DEBUG_PRINTLN(F(" received"));
      sendMessage(TYPE_CONFIRM_CONFIG, RIOTS_OK);
      break;

    case TYPE_ACTIVATE_RING:
      _DEBUG_PRINTLN(F(" TYPE_ACTIVATE_RING"));

      // Take new I/Os into use and store them
      for (int i=0; i<plain_data[M_LENGTH]; i+=2) {
        // Check that there is room for new IO
        if (io_index > 7) {
          _DEBUG_PRINTLN(F(" No room for new IOs"));
          sendMessage(TYPE_CONFIRM_CONFIG, RIOTS_FAIL);
          io_index = EEPROM.read(EEPROM_IO_INDEX);
          return RIOTS_FAIL;
        }

        io_table[0][io_index] = plain_data[M_VALUE+i];
        io_table[1][io_index] = plain_data[M_VALUE+i+1];
        EEPROM.write(EEPROM_CORE_INDEX+io_index, io_table[0][io_index]);
        EEPROM.write(EEPROM_RING_INDEX+io_index, io_table[1][io_index]);
        io_index++;
      }

      EEPROM.write(EEPROM_IO_INDEX, io_index);
      // Activate ring
      bitSet(net_status, NET_RING_CONNECTION);
      EEPROM.write(EEPROM_NET_STATUS, net_status);

      // Clear the current ring counter back to first
      current_ring_counter = 1;

      sendMessage(TYPE_CONFIRM_CONFIG, RIOTS_OK);
      break;

    case TYPE_DEACTIVATE_RING:
      _DEBUG_PRINTLN(F(" TYPE_DEACTIVATE_RING"));

      // Clear ring index
      io_index = 0;
      EEPROM.write(EEPROM_IO_INDEX, io_index);

      // Clear ring status
      bitClear(net_status, NET_RING_CONNECTION);
      EEPROM.write(EEPROM_NET_STATUS, net_status);

      sendMessage(TYPE_CONFIRM_CONFIG, RIOTS_OK);
      break;

    case TYPE_CLOUD_EVENT_DOWN:
      // Save received event for the further use
      memcpy(saved_event, plain_data+M_VALUE, 6);

      _DEBUG_PRINTLN(F(" Got data from RING to my IO"));
      sendMessage(TYPE_CONFIRM_CONFIG, RIOTS_OK);
      return RIOTS_DATA_AVAILABLE;
      break;


    case TYPE_ENTER_PROGMODE: /* FALLTHROUGH */
    case TYPE_LEAVE_PROGMODE:
    case TYPE_LOAD_ADDRESS:
    case TYPE_PROG_FLASH:
    case TYPE_PROG_PAGE:

#ifdef RIOTS_FLASH_MODE
      flash_mode = 1;
      flash_start = millis();
#endif

      updateLedStatus(0, RIOTS_FLASH_COLOR);

      flash_reply = riots_flash.handleFlashMessage(plain_data[M_TYPE], plain_data, &response);

      if ( response == true ) {
        if ( plain_data[M_TYPE] == TYPE_PROG_PAGE ) {
          // Confirm LOAD_ADDRESS configuration after all packages has been received successfully
          // or when first failure happens
          plain_data[M_TYPE] = TYPE_LOAD_ADDRESS;
        }

        _DEBUG_PRINTLN(F(" Replying to flash message"));
        sendMessage(TYPE_CONFIRM_CONFIG, flash_reply);
      }

      if ( flash_reply == RIOTS_RESET ) {
        // Reboot to bootloader here, as we have replied to cloud
        // Reset PIN currently hard coded
        digitalWriteFast(reset_pin, LOW);
      }

      break;

    default:
      _DEBUG_PRINTLN(F(" UNKNOWN MESSAGE"));
      _DEBUG_PRINTLN(plain_data[M_TYPE]);
      return RIOTS_FAIL;

    }
  return RIOTS_OK;

}

/**
* Sends message to the cloud.
*
*/
void Riots_BabyRadio::sendMessage(byte message, byte status) {
  // Override previously failed messages (priority package)
  send_status = RIOTS_OK;
  uint8_t i;

  switch (message) {

    case TYPE_IM_ALIVE:
      _DEBUG_PRINTLN(F("Riots_BabyRadio::sendMessage: TYPE_IM_ALIVE"));
      // Read firmware ID from EEPROM
      for(i=0; i<4; i++) {
        plain_data[M_VALUE+i] = EEPROM.read(EEPROM_FIRMWARE_ID+i);
      }

      if(base_attached == RIOTS_OK) {
        // Read Base ID from EEPROM
        for(i=0; i<4; i++) {
          plain_data[M_VALUE2+i] = EEPROM.read(EEPROM_BASE_ID+i);
        }
        formMessage(TYPE_IM_ALIVE, IM_ALIVE_LEN);
      }
      else {
        formMessage(TYPE_IM_ALIVE, IM_ALIVE_NO_BASE_LEN);
      }

      // Send message and check I'm alive fail count
      if ( cloudForward() == RIOTS_OK) {
        // If there was no connection earlier enable it
        if ( !bitRead(net_status, NET_CLOUD_CONNECTION) ) {
          _DEBUG_PRINTLN(F(" Enabling Cloud"));
          bitSet(net_status, NET_CLOUD_CONNECTION);
          EEPROM.write(EEPROM_NET_STATUS, net_status);
        }
        // Clear fail count
        _DEBUG_PRINTLN(F(" Clearing fail count"));
        im_alive_fail_count = 0;

      } else if ( im_alive_fail_count < 3) {
        im_alive_fail_count++;
        _DEBUG_PRINT(F(" Fail count: "));
        _DEBUG_PRINTLN(im_alive_fail_count);
      }

      if ( im_alive_fail_count > 2 && bitRead(net_status, NET_CLOUD_CONNECTION) ) {
        // Disable Cloud
        _DEBUG_PRINTLN(F(" Disabling Cloud"));
        bitClear(net_status, NET_CLOUD_CONNECTION);
        EEPROM.write(EEPROM_NET_STATUS, net_status);
      }

      break;

    case TYPE_CONFIRM_CONFIG:
      byte sendStatus;
      uint8_t retries;
      retries = 0;
      _DEBUG_PRINTLN(F("Riots_BabyRadio::sendMessage: TYPE_CONFIRM_CONFIG"));
      plain_data[M_VALUE] = plain_data[M_TYPE]; // Received message type
      plain_data[M_STATUS] = status;

      // Form message
      formMessage(TYPE_CONFIRM_CONFIG, CONFIRM_CONFIG_LEN);
      // Send message
      sendStatus = RIOTS_FAIL;
      while (sendStatus != RIOTS_OK && retries < 5) {
        sendStatus = cloudForward();
        retries++;
        if ( sendStatus != RIOTS_OK) {
          _DEBUG_PRINT(F("FIX FIX FIX"));
          delay(100);
        }
      }
      break;

    case TYPE_CORE_NOT_REACHED:
      _DEBUG_PRINTLN(F("Riots_BabyRadio::sendMessage: TYPE_CORE_NOT_REACHED"));
      formMessage(TYPE_CORE_NOT_REACHED, CORE_NOT_REACHED_LEN);
      cloudForward();
      break;

    default:
      _DEBUG_PRINTLN(F("Riots_BabyRadio::sendMessage: Unknown message"));
  }
}

/**
* Check that received counter is valid.
*
* Accept only counter values which are equal or greater than previous values.
*
*/
byte Riots_BabyRadio::checkCounter() {
  // Check that received counter is bigger than stored
  if (memcmp(plain_data + M_COUNTER, counter, 2) >= 0) {
    // Write received counter to EEPROM
    for (int i=0; i<2; i++) {
      counter[i] = plain_data[M_COUNTER+i];
      EEPROM.write(EEPROM_COUNTER+i, counter[i]);
    }
    return RIOTS_OK;
  }
  _DEBUG_PRINTLN(F("Riots_Radio::checkCounter: FAILED"));
  return RIOTS_FAIL;
}


/**
* Checks that given IO number is valid and saves received data.
*
* @param ringIO     IO number in thering
* @return byte      Returns RIOTS_OK if giving IO number is found.
*/
byte Riots_BabyRadio::getMyIO(byte ringIO) {
  for (int i=0; i<io_index; i++) {
    if (io_table[1][i] == ringIO) {
      _DEBUG_PRINT(F("Riots_Radio::getMyIO: Got ID: "));
      _DEBUG_PRINTLN(io_table[0][i]);
      saved_event[0] = io_table[0][i];
      return RIOTS_OK;
    }
  }
  return RIOTS_UBER_FAIL;
}

/**
* Checks that given ring IO number is valid.
*
* @param ringIO     IO number
* @return byte      Returns ring IO number is given IO number is valid.
*/
byte Riots_BabyRadio::getRingIO( byte myIO ) {
  for (int i = 0; i<io_index; i++) {
    if ( myIO == io_table[0][i] ) {
      _DEBUG_PRINT(F("Riots_Radio::getRingIO: Got ID: "));
      _DEBUG_PRINTLN(io_table[1][i]);
      return io_table[1][i];
    }
  }
  return RIOTS_UBER_FAIL;
}

/**
* Writes and sends the given character to Radio
*
* @param character  Character to be written.
* @return size_t    Status of the writing.
*/
size_t Riots_BabyRadio::write(uint8_t character) {
  write(&character, 1);
  return false;
}

/**
* Sends given message to riots radio. This is handler for the print command.
*
* @param buffer     Buffer of the characets to be written.
* @param size       Size of the writing buffer.
* @return size_t    Status of the writing. Currently allways zero
*/
/* Handler for the print-command */
size_t Riots_BabyRadio::write(const uint8_t *buffer, size_t size) {

  updateLedStatus(0, RIOTS_BLUE_COLOR);

  byte retries;
  byte length;
  byte delaytime = 0;

  // Calculate count of 10 bytes packets
  byte packet_count = (size-1)/10+1;

  for (int packet=0; packet<packet_count; packet++) {
    retries = 0;
    length = 10;

    // Last packet
    if ((packet+1) == packet_count) {
      length = size-packet*10;
    }

    memcpy(plain_data + M_VALUE+1, buffer + (10*packet), length);

    length += 1;
    // First packet
    if ( packet == 0 ) {
      plain_data[M_VALUE] = packet_count;
      formMessage(TYPE_DEBUG_START, length, 0);
    }
    else {
      plain_data[M_VALUE] = packet;
      formMessage(TYPE_DEBUG_DATA, length, 0);
    }

    // Send to Debug address
    while(uartSend() != RIOTS_OK && retries < BABY_RADIO_RETRY_COUNT) {
      retries += 1;
    }
    if (retries == BABY_RADIO_RETRY_COUNT) {
      break;
    }
  }
  return 1;
}

/**
* Indicates baby status with the leds.
*
*/
void Riots_BabyRadio::updateLedStatus(byte sleep, uint32_t color) {

  if (indicativeLeds == 1 && sleep != 1) {
    if ( color != 0 ) {
      riots_RGBLed.setColor(color);
    } else if ( bitRead(net_status, NET_CLOUD_CONNECTION) ) {
      riots_RGBLed.setColor(RIOTS_CONNECTION_OK_COLOR);
    } else if ( bitRead(net_status, NET_RING_CONNECTION) ) {
      riots_RGBLed.setColor(RIOTS_MAGENTA_COLOR);
    } else if ( bitRead(net_status, NET_DEBUG_CONNECTION) ) {
      riots_RGBLed.setColor(RIOTS_CYAN_COLOR);
    } else {
      riots_RGBLed.setColor(RIOTS_CONNECTION_FAIL_COLOR);
    }
  }
  else if (sleep ==1){
    riots_RGBLed.setColor(0);
  }
}

/**
* Checks that received ring address is valid.
*
* @return boolean   true, if received package is valid.
*/
bool Riots_BabyRadio::isRingAddressValid() {
  byte message_type = plain_data[M_TYPE];

  if ( message_type == TYPE_RING_EVENT ) {
    // check if the received address is same as received from the cloud previously
    if ( RPC[0] == plain_data[M_VALUE+6] &&
         RPC[1] == plain_data[M_VALUE+7]) {
      return true;
    }
  }
  else if ( message_type == TYPE_RING_EVENT_BACK ) {
    // check if the received address is same as received from the cloud previously
    if ( RNC[0] == plain_data[M_VALUE+6] &&
         RNC[1] == plain_data[M_VALUE+7]) {
      return true;
    }
  }
  _DEBUG_PRINT(F("wrong child id "));

  return false;
}

/**
* Checks that received ring counter is valid.
*
* @return boolean   true, if received ring counter is valid.
*/
bool Riots_BabyRadio::isRingCounterValid() {
  uint16_t received_ring_counter;
  memcpy(&received_ring_counter, plain_data + M_COUNTER, 2);


  if ( ( received_ring_counter < (current_ring_counter + MAX_SKIPPED_RING_EVENTS ) ) ||
        ( current_ring_counter == 1 && received_ring_counter > 0 ) ) {
    // increase the counter
    current_ring_counter = received_ring_counter;
    memcpy(plain_data + M_COUNTER, &current_ring_counter, 2);

    if ( own_ring_event_ongoing ) {
      // increase the couter to msg as well as my own message is on the way
      _DEBUG_PRINTLN(F("Increase counter as own message is ongoing"));
      current_ring_counter++;
    }
    return true;
  }
  _DEBUG_PRINTLN(F("wrong counter"));

  return false;
}
