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
#include <avr/wdt.h>
#include <avr/sleep.h>

#include "aes.h"
#include <SPI.h>
#include <Time.h>

#include "Riots_MamaCloud.h"
#include "Riots_Mamadef.h"

#ifdef RIOTS_RADIO_DEBUG
/* flag for printing IP address in the first time*/
bool first_time = true;
#endif
uint8_t count;

/**
 * Saves memory addresses for the shared memory. These same memory chunks
 * are used in both Radio and Cloud libraries.
 *
 * @param plain_data_adr          Memory ptr to shared buffer used for storing plain data.
 * @param crypt_buff_adr          Memory ptr to shared buffer used for storing crypt data.
 * @param uniq_key_addr           Memory ptr to shared buffer used for storing unique AES key.
 */
void Riots_MamaCloud::setAddresses(byte* plain_data_adr,
                                   byte* tx_crypt_buff_adr,
                                   byte* rx_crypt_buff_adr,
                                   byte* uniq_key_addr ) {
  tx_crypt_buff  = tx_crypt_buff_adr;
  rx_crypt_buff  = rx_crypt_buff_adr;
  plain_data  = plain_data_adr;
  uni_aes     = uniq_key_addr;
}

/**
 * Setups the ethetner shield and establish the connection to the DHCP
 *
 */
void Riots_MamaCloud::setup(byte indicativeLedsOn) {
  _DEBUG_PRINTLN(F("Cloud"));
  if ( indicativeLedsOn == 1) {
    indicativeLeds = 1;
    riots_RGBLed.setup();
  }

  byte mac[6];

  /* initialize the pseudo-random number generator with unconnected pin */
  randomSeed(analogRead(2));

  /* Reset the W5500 ethernet shield */
  pinMode(RIOTS_ETHERNET_RESET, OUTPUT);

  digitalWrite(RIOTS_ETHERNET_RESET, LOW);
  delay(100); // keep down for a moment
  digitalWrite(RIOTS_ETHERNET_RESET, HIGH);

  // Enable with chipset selection pin
  pinMode(RIOTS_ETHERNET_CS, OUTPUT);
  digitalWrite(RIOTS_ETHERNET_CS, LOW);

  activateLeds(RIOTS_CLOUD_INITALIZING_COLOR);

  if( riots_memory.setup(RIOTS_PRIMARY_EEPROM) ) {
    /* Using 00 FE for the beginin of the MAC which should be inline with
     * IETF: http://tools.ietf.org/html/rfc7042
    */
    mac[0] = 0x00;
    mac[1] = 0xFE;

    // fill rest of the mac with MAC address of the Base which is read from I2C EEPROM
    for (int i=0; i < MAC_ADDRESS_SIZE; i++) {
      mac[i+2] = riots_memory.read((I2C_EEPROM_MAC_ADDRESS+i));
    }
  }
  else {
    riots_RGBLed.setColor(RIOTS_CLOUD_FAIL_COLOR);
    // No reason to continue, User shall reboot Mama in this state
    for(;;) ;
  }

  // Establish the ethernet connection
  while(Ethernet.begin(mac) == 0) {
    riots_RGBLed.setColor(RIOTS_MAMA_NO_IP_COLOR);
  }
  // release the line
  digitalWrite(RIOTS_ETHERNET_CS, HIGH);

  // Connection to server is not yet valid
  session_key_received = false;
  if ( riots_memory.setup(RIOTS_SECONDARY_EEPROM) ) {
    eeprom_status = RIOTS_OK;
    // TODO: Read from EEPROM
    eeprom_looped_once  = false;
  }
  else {
    activateLeds(RIOTS_CLOUD_FAIL_COLOR);
    // No reason to continue, User shall reboot Mama in this state
    for(;;) ;
  }
  // begining of the eeprom is reserved for the further use
  current_msg_ind     = I2C_EEPROM_MIN;
  last_saved_msg_ind  = I2C_EEPROM_MIN;

  connection_verificated = false;
  pending_message = false;
  time_received = false;
  myNextAttempt = 0;
}

/**
 * Checks if there is available data from the cloud.
 *
 * If the data is related to the ethernet connection calls the funtion handlers for it
 * otherwise saves the action code, which can be used in the INO-side for determing
 * what to do with the data.
 *
 * @param action_needed           Byte representing the required action from the INO side
 * @return byte                   RIOTS_OK if successfully, otherwise error code
 */
byte Riots_MamaCloud::update(byte *action_needed) {

  *action_needed = NO_ACTION_REQUIRED;

  if ( RIOTS_OK == validateConnection() ) {
    if ( ethernet_client.available() > 0 ) {

      byte length    = ethernet_client.read();
      byte operation = ethernet_client.read();

      switch ( operation ) {
        case SERVER_VERIFICATION:
          return validateSession();
        break;

        case SERVER_REQUESTS_INTRODUCTION:
          session_key_received = false;
          // Start negotiation from the begining
          sendRequestToCloud(CLIENT_INTRODUCTION);
        break;

        case SERVER_DATA_RECEIVER:
          validateReceiver();
          *action_needed = SET_RADIO_RECEIVER;
        break;

        case SERVER_DATA_POST:
          // substract length of the operation from the total amount of data
          // and calculate total data blob count and save to member variable
          data_blobs_available = (length - 1) / DATA_BLOCK_SIZE;
          // increase the count by one
          data_blobs_available++;
          *action_needed = FORWARD_DATA;
        break;

        default:
          _DEBUG_PRINTLN(operation, HEX);
          uint16_t count = ethernet_client.available();

          // clean the whole data buffer and do nothing
          for (uint16_t i = 0; i < count; i++) {
            ethernet_client.read();
          }
          return RIOTS_NOT_FOUND;
        break;

      }
    }
    else {
      if ( ethernet_client.connected() ) {
        // we are connected to network sending a keep alive request
        if ( session_key_received &&
          ( ( millis() - myNextConnection ) > PINGING_INTERVAL ) ) {
          // update time
          myNextConnection = millis();

          // keep Ethernet/DHCP connection alive
          Ethernet.maintain();

          // send alive request to keep connection alive
          sendRequestToCloud(KEEP_ALIVE);
          // indicate with the led
          activateLeds(RIOTS_CONNECTION_OK_COLOR);
        }
      }
    }
  }
  return RIOTS_OK;
}

/**
 * Reads the next data blob from the etherner shield buffer.
 *
 * @return byte                   Number of datablobs available after this
 */
byte Riots_MamaCloud::getNextDataBlob() {

  // read next data blob if there was originally more than 1 available
  if ( data_blobs_available > 1 && ethernet_client.available() >= DATA_BLOCK_SIZE ) {
    // There is atleast one more data blob left
    for (int i = 0; i < DATA_BLOCK_SIZE; i++) {
      rx_crypt_buff[i] = ethernet_client.read();
    }
    // decrypt the data message with session_key
    AES128_ECB_decrypt(rx_crypt_buff, sess_key, rx_crypt_buff);
  }
  if ( data_blobs_available > 0 ) {
    data_blobs_available--;
  }
  return data_blobs_available;
}

/**
 * Forwards the message received from the radio side to the cloud.
 *
 * TODO: If we are not able to send this message we need to save the message to the
 * EEPROM and save it later when the connection is again available.
 *
 * @return byte                   RIOTS_OK if successfully, otherwise error code
 */
byte Riots_MamaCloud::forwardToCloud() {

  if ( time_received ) {
    if ( this->session_key_received ) {
      // indicate with the led
      activateLeds(RIOTS_ORANGE_COLOR );
      sendRequestToCloud(CLIENT_DATA_POST);
      return RIOTS_OK;
    }
    saveMessage();
  }

  // There is no session available, message should be saved
  return RIOTS_NOT_CONNECTED;
}

byte* Riots_MamaCloud::getNextReceiverAddress() {
  // Return begining of the plain_data, as the address must be there when
  // this function is called
  return plain_data;
}

/**
 * Validates the connection to the DHCP, If the connection is valid and there is session
 * available is valid issues the first message in the handshaking protocol.
 *
 * @return byte                   RIOTS_OK if successfully, otherwise error code
 */
byte Riots_MamaCloud::validateConnection() {

  if( RIOTS_OK == dhcpValidated() ) {
    if ( millis() - myNextAttempt > CONNECTION_RETRY_TIME ) {
      // time to re-check connection
      if ( !ethernet_client.connected() ) {
        myNextAttempt = millis();
        // enable with CS
        digitalWrite(RIOTS_ETHERNET_CS, LOW);
        if (ethernet_client.connect(RIOTS_CLOUD_ADDRESS, RIOTS_CLOUD_PORT)) {
          activateLeds(RIOTS_CLOUD_FAIL_COLOR);

          // Issue introduction to server
          sendRequestToCloud(CLIENT_INTRODUCTION);
        }
        else {
          connection_verificated = false;
          session_key_received = false;
          ethernet_client.stop();
          activateLeds(RIOTS_CONNECTION_FAIL_COLOR);

          return RIOTS_NOT_CONNECTED;
        }
      }
      else {
      }
    }
  }
  return RIOTS_OK;
}

/**
 * Encrypts the data, checks the CRC matches and in the if ecerything is successfully
 * saves the new radio address for the previous received pointer.
 *
 * @return byte                   RIOTS_OK if successfully, otherwise error code
 */
byte Riots_MamaCloud::validateReceiver() {

  // Read entire data
  for (uint8_t i = 0; i < DATA_BLOCK_SIZE; i++) {
    rx_crypt_buff[i] = ethernet_client.read();
  }

  // decrypt the data with session key
  AES128_ECB_decrypt(rx_crypt_buff, sess_key, plain_data);

  if ( calcChecksum(plain_data, DATA_BLOCK_SIZE) == 0 ) {
    // reply ok, as the checksum matches
    return RIOTS_OK;
  }
  // Cheksum did not match
  return RIOTS_FAIL;
}

/**
 * Process possible saved message.
 *
 * Sends possible saved message if the connection is valid.
 */
void Riots_MamaCloud::processCachedMessage() {
  if ( connection_verificated && session_key_received ) {
    // we can process with possible cached message
    if ( pending_message ) {
      sendRequestToCloud(CLIENT_SAVED_DATA_POST);
    }
  }
}

/**
 * Handler for the SERVER_INTRODUCTION mesage. This message is used for
 * validating the session with the TCP server.
 *
 * Encrypt the data with own unique aes key, makes the CRC checksum. Saves the
 * time and received challenge bytes for later use and in the end saves the
 * receved session key.
 *
 * If there is a new keys available sends a verification message to the server.
 *
 * @return byte                   RIOTS_OK if successfully, otherwise error code
 */
byte Riots_MamaCloud::validateSession(){

  uint8_t i;

  // Read first 16 bytes
  for (i = 0; i < DATA_BLOCK_SIZE; i++) {
    rx_crypt_buff[i] = ethernet_client.read();
  }

  // decrypt the message with unique key
  AES128_ECB_decrypt(rx_crypt_buff, uni_aes, plain_data);

  if ( calcChecksum(plain_data, DATA_BLOCK_SIZE) == 0 ) {
    // Checksum matches
    time_t current_time = (uint32_t) plain_data[0] << 24;
    current_time |= (uint32_t) plain_data[1] << 16;
    current_time |= (uint32_t) plain_data[2] << 8;
    current_time |= (uint32_t) plain_data[3];

    // Set current time for messages
    setTime(current_time);

    if (memcmp(challenge, &plain_data[4], 4) != 0) {
      return RIOTS_FAIL;
    }
    // Read next 16 bytes
    for (i = 0; i < DATA_BLOCK_SIZE; i++) {
      rx_crypt_buff[i] = ethernet_client.read();
    }

    // decrypt rest of message with unique key
    AES128_ECB_decrypt(rx_crypt_buff, uni_aes, plain_data);

    if (memcmp(sess_key, plain_data, AES_KEY_SIZE) != 0) {
      memcpy(sess_key, plain_data, AES_KEY_SIZE);

      // inform server that wer have received these keys successfully
      sendRequestToCloud(CLIENT_VERIFICATION);
      // The connection should be now valid
      session_key_received = true;
      time_received = true;
      return RIOTS_OK;
    }
  }
  return RIOTS_FAIL;
}

/**
 * Used for checking connection validity to the DHCP server.
 * TODO: Not implemented currently.
 *
 * @return byte                   RIOTS_OK if successfully, otherwise error code
 */
byte Riots_MamaCloud::dhcpValidated() {
#ifdef RIOTS_RADIO_DEBUG
  if ( first_time ) {
    _DEBUG_PRINT(F("my IP: "));
    _DEBUG_PRINTLN(Ethernet.localIP());
     first_time = false;
  }
#endif
  return RIOTS_OK;
}

/**
 * Sends a request to cloud.
 *
 * Packs the requested information for the message and sends the message to the
 * TCP server.
 */
void Riots_MamaCloud::sendRequestToCloud(Riots_Message cloud_message_type) {

  switch ( cloud_message_type ) {
    case CLIENT_INTRODUCTION:

      // Save whole data as a plain data
      plain_data[0] = 0x0D; // lenght - this value
      plain_data[1] = CLIENT_INTRODUCTION;

      if( riots_memory.setup(RIOTS_PRIMARY_EEPROM) ) {
        // Read mama address from i2c eeprom
        plain_data[2] = riots_memory.read(I2C_EEPROM_MAC_ADDRESS);
        plain_data[3] = riots_memory.read(I2C_EEPROM_MAC_ADDRESS+1);
        plain_data[4] = riots_memory.read(I2C_EEPROM_MAC_ADDRESS+2);
        plain_data[5] = riots_memory.read(I2C_EEPROM_MAC_ADDRESS+3);
        // Initialize back to secondary EEPROM
        riots_memory.setup(RIOTS_SECONDARY_EEPROM);
      }

      // Read MAC address from EEPROM (Same as mama address currently)
      plain_data[6] = EEPROM.read(EEPROM_RX_ADDR);
      plain_data[7] = EEPROM.read(EEPROM_RX_ADDR+1);
      plain_data[8] = EEPROM.read(EEPROM_RX_ADDR+2);
      plain_data[9] = EEPROM.read(EEPROM_RX_ADDR+3);

      // fillRandomPadding as a challenge
      fillRandomPadding(challenge, 4);

      memcpy(plain_data+10, challenge, 4);

      // Send message
      ethernet_client.write(plain_data, 0x0E);
      ethernet_client.flush();
    break;

    case CLIENT_VERIFICATION:
      tx_crypt_buff[0]  = 0x11;                 // lenght of message
      tx_crypt_buff[1]  = CLIENT_VERIFICATION;  // type as a plain data

      // Save current time.
      plain_data[0]     = (byte)((now() >> 24) & 0xFF);
      plain_data[1]     = (byte)((now() >> 16) & 0xFF);
      plain_data[2]     = (byte)((now() >> 8) & 0xFF);
      plain_data[3]     = (byte)(now() & 0xFF);

      fillRandomPadding(plain_data+4, 11);
      plain_data[15] = calcChecksum(plain_data, DATA_BLOCK_SIZE-1);

      // crypt the data and keep the header as a plain data
      AES128_ECB_encrypt(plain_data, sess_key, tx_crypt_buff+2 );

      // Send message
      ethernet_client.write(tx_crypt_buff, 0x12);
      ethernet_client.flush();
    break;

    case KEEP_ALIVE:
      activateLeds(RIOTS_BLUE_COLOR);

      plain_data[0] = 0x01;               // length
      plain_data[1] = KEEP_ALIVE;         // operation

      ethernet_client.write(plain_data, 0x02);
      ethernet_client.flush();
    break;

    case CLIENT_DATA_POST:
      activateLeds(RIOTS_BLUE_COLOR);

      //Using rx buff as we dont want to mess possible sending buffer.
      rx_crypt_buff[0]  = 0x11;  // length
      rx_crypt_buff[1]  = CLIENT_DATA_POST;  // type
      // rest of the buffer is filled in RIOTS_RADIO side

      // crypt the data and keep the header as a plain data
      AES128_ECB_encrypt(plain_data, sess_key, rx_crypt_buff+2 );
      ethernet_client.write(rx_crypt_buff, 0x12);
      ethernet_client.flush();
    break;

    case CLIENT_SAVED_DATA_POST:
      activateLeds(RIOTS_BLUE_COLOR);

      byte send_buff[34];
      send_buff[0] = 0x21;                  // Length
      send_buff[1] = CLIENT_SAVED_DATA_POST;// operation
      // read rest of the message
      if ( readLastCachedMessage(send_buff+2) ) {
        // succesfully read, send the message
        ethernet_client.write(send_buff, 0x22);
        ethernet_client.flush();
    }
    break;
  }
  if ( connection_verificated) {
    activateLeds(RIOTS_CONNECTION_OK_COLOR);
  }
}

/**
 * Helper function for calculating the CRC cheksum.
 *
 * Starts from the giver pointer and cacculates the checksum for the count
 * of bytes informed by the user.
 *
 * @param input                   Start point of the message.
 * @param lenght                  Lenght of the message.
 * @return byte                   Checksum for the characters.
 */
byte Riots_MamaCloud::calcChecksum(byte* input, byte length) {
  byte i, chksum=0;

  // Limit to the maximun datablock size
  if ( length <= DATA_BLOCK_SIZE ) {
    for (i = 0; i < length; i++) {
      chksum ^= input[i];
    }
  }
  return chksum;
}

/**
 * Helper function for filling random padding.
 *
 * Starts from the giver pointer and fills as many random characters as
 * required.
 *
 * @param input                   Start point of the filling area.
 * @param lenght                  Lenght of the filling area.
 */
void Riots_MamaCloud::fillRandomPadding(byte* start_ptr, byte length) {

  // Limit to the maximun datablock size
  if ( length <= DATA_BLOCK_SIZE ) {
    // Gererate random padding for message
    for (uint8_t i = 0; i < length; i++) {
      start_ptr[i] = random(255);
    }
  }
}

/**
 * Function to inform class that connection has been verified in radio side.
 *
 * This function needs to be called before we can send possible cached messages to cloud.
 */
void Riots_MamaCloud::connectionSettingsVerificated() {
  activateLeds(RIOTS_CONNECTION_OK_COLOR);

  // enable watchdog reset here
  wdt_enable(WDTO_8S);

  connection_verificated = true;
}

/**
 * Sends core not reached message to the cloud and flushes current datablobs for this core.
 *
 */
void Riots_MamaCloud::sendCoreNotReached() {

  if (data_blobs_available > 1) {
  // flush the datablobs by reading those out from ethernet buffer.
    for (int i = 0; i < DATA_BLOCK_SIZE*(data_blobs_available-1); i++) {
      ethernet_client.read();
    }
  // plain data should be filled with the appropriate core not reached at this point
  // issue the message to the cloud.
  sendRequestToCloud(CLIENT_DATA_POST);
  data_blobs_available = 0;
  }
}

/**
 * Saves a valid data message to the eeprom.
 *
 * Increases the eeprom message counters and makes a smooth looping in ring buffer
 * if EEPROM memory size is full.
 */
void Riots_MamaCloud::saveMessage() {

  if ( eeprom_status == RIOTS_OK ) {
    // check the current address
    activateLeds(RIOTS_MAMA_SAVE_DATA_COLOR);

    startFilling();

    // save first current time:
    fillByte((byte)((now() >> 24) & 0xFF));
    fillByte((byte)((now() >> 16) & 0xFF));
    fillByte((byte)((now() >> 8) & 0xFF));
    fillByte((byte)(now() & 0xFF));

    // save current datablob
    for(uint8_t i=0; i < DATA_BLOCK_SIZE; i++) {
      fillByte(plain_data[i]);
    }
    stopFilling();
    pending_message = true;

    if ( current_msg_ind > I2C_EEPROM_MAX ) {
      current_msg_ind = I2C_EEPROM_MIN;
      if ( !eeprom_looped_once ) {
        // Set last message address to point current message address as this
        // is first time when the index has reached the end
        last_saved_msg_ind = current_msg_ind;
      }
      eeprom_looped_once = true;
    }
    else if ( eeprom_looped_once ) {
      // as the head has looped once, we need to keep last at head + 1 position
      last_saved_msg_ind += I2C_EEPROM_MSG_SIZE;
      if ( last_saved_msg_ind > I2C_EEPROM_MAX ) {
        last_saved_msg_ind = I2C_EEPROM_MIN;
      }

    }
  }
}

/**
 * Start filling to secondary EEPROM.
 *
 */
void Riots_MamaCloud::startFilling() {
    // start eeprom writing
  riots_memory.startPageWrite(current_msg_ind, RIOTS_SECONDARY_EEPROM);
}

/**
 * Stop EEPROM filling.
 *
 */
void Riots_MamaCloud::stopFilling() {
    // stop eeprom writing
    riots_memory.stopPageWrite();
}

/**
 * Writes a single byte to the SECONDARY eeprom.
 *
 * @param ch                      Byte to be written in EEPROM
 */
void Riots_MamaCloud::fillByte(byte ch) {

  current_msg_ind++;
  if ( current_msg_ind % 128 == 0 ) {
    // eeprom is 128 aligned, we need to jump to the next page
    stopFilling();
    startFilling();
  }
  riots_memory.pageFill(ch);
}

/**
 * Reads last cached message from the eeprom
 *
 * Start reading either from the begining of the memory area or from head+1 position
 * if the memory has looped.
 *
 * @return bool                   True if the EEPROM is available.
 */
bool Riots_MamaCloud::readLastCachedMessage(byte* read_buffer) {
  if ( eeprom_status == RIOTS_OK ) {
    _DEBUG_PRINT(last_saved_msg_ind);
//    _DEBUG_PRINTLN(F("-->SEND"));

    // give a starting point to eeprom reader
    riots_memory.startRead(last_saved_msg_ind, RIOTS_SECONDARY_EEPROM);

    read_buffer[0] = riots_memory.sequentialRead();
    read_buffer[1] = riots_memory.sequentialRead();
    read_buffer[2] = riots_memory.sequentialRead();
    read_buffer[3] = riots_memory.sequentialRead();

    // add random filling
    fillRandomPadding(read_buffer+4, 11);

    // add checksum
    read_buffer[15] = calcChecksum(read_buffer, DATA_BLOCK_SIZE-1);

    // read datablob - 1 to the end of the buffer
    for(uint8_t i=0; i < DATA_BLOCK_SIZE-1; i++) {
      read_buffer[DATA_BLOCK_SIZE+i] = riots_memory.sequentialRead();
    }
    //read last byte to last index (datablock size x 2 - 1)
    read_buffer[(DATA_BLOCK_SIZE*2)-1] = riots_memory.readLast();

    // encrypt the first part of the data
    AES128_ECB_encrypt(read_buffer, sess_key, read_buffer);

    // encrypt the second part of data with the session key
    AES128_ECB_encrypt(read_buffer+DATA_BLOCK_SIZE, sess_key, read_buffer+DATA_BLOCK_SIZE);

    // increase the counter
    last_saved_msg_ind += I2C_EEPROM_MSG_SIZE;

    if ( last_saved_msg_ind > I2C_EEPROM_MAX ) {
      last_saved_msg_ind = I2C_EEPROM_MIN;
      // we are in the end of memory area
      if ( eeprom_looped_once ) {
      // as we have looped, continue reading from the begining of memory area
        eeprom_looped_once = false;
      }
    }

    if ( !eeprom_looped_once &&
          last_saved_msg_ind == current_msg_ind ) {
      // we are in the same round and the tail and head is the same, all messages has been written
   //  _DEBUG_PRINTLN(F("LAST"));
      pending_message = false;
      last_saved_msg_ind = I2C_EEPROM_MIN;
      current_msg_ind    = I2C_EEPROM_MIN;
    }
    return true;
  }
  return false;
}

/**
 * Set led to given value if indicative leds has been activated.
 * This led color indicates status of the device to the end user.
 *
 */
void Riots_MamaCloud::activateLeds(uint32_t rgb) {
  if (indicativeLeds == 1) {
    // indicate with the led
    riots_RGBLed.setColor(rgb);
  }
}
