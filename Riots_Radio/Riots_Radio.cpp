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
#include "nRF24L01.h"

#include "Riots_Radio.h"

/**
 * Setup function sets given ce, csn, irq and reset pins.
 * If arguments are not given function reads pin values from EEPROM
 * If pin values are not found from EEPROM, function sets hardcoded values (v05)
 *
 * @param debug       Enable debugging
 * @param nrfce       Chip Enable Activates RX or TX mode
 * @param nrfcsn      SPI Chip Select
 * @param nrfirq      Maskable interrupt pin. Active low
 * @param nrfrst      Reset pin
 * @return reset_pin  Returns selected reset pin number
 */
int Riots_Radio::setup(int nrfce, int nrfcsn, int nrfirq, int nrfrst) {

  wdt_reset();

  ce_pin = nrfce;
  if(ce_pin == 0xFF) ce_pin = EEPROM.read(EEPROM_RADIO_CE);
  if(ce_pin == 0xFF || ce_pin == 0x00) ce_pin = RIOTS_CE_PIN;

  csn_pin = nrfcsn;
  if(csn_pin == 0xFF) csn_pin = EEPROM.read(EEPROM_RADIO_CSN);
  if(csn_pin == 0xFF || csn_pin == 0x00) csn_pin = RIOTS_CSN_PIN;

  irq_pin = nrfirq;
  if(irq_pin == 0xFF) irq_pin = EEPROM.read(EEPROM_RADIO_IRQ);
  if(irq_pin == 0xFF || irq_pin == 0x00) irq_pin = RIOTS_IRQ_PIN;

  reset_pin = nrfrst;
  if(reset_pin == 0xFF) reset_pin = EEPROM.read(EEPROM_RESET);
  if(reset_pin == 0xFF || reset_pin == 0x00) reset_pin = RIOTS_RESET_PIN;

  // Initializes radio pins
  pinModeFast(ce_pin, OUTPUT);
  pinModeFast(csn_pin, OUTPUT);
  pinModeFast(irq_pin, INPUT);

  digitalWriteFast(reset_pin, HIGH);
  pinModeFast(reset_pin, OUTPUT);

  digitalWriteFast(csn_pin, HIGH);
  digitalWriteFast(ce_pin, LOW);

  SPI.setBitOrder(MSBFIRST);
  SPI.setDataMode(SPI_MODE0);
  SPI.setClockDivider(SPI_CLOCK_DIV2);
  SPI.begin();
  delay(100);

  // Read own RF address from EEPROM
  _DEBUG_EXT_PRINT("RiotsRadio::Setup Core Address:");
  for (int i=0; i < RF_ADDRESS_SIZE; i++) {
    CA[i] = EEPROM.read(EEPROM_RX_ADDR+i);
    _DEBUG_EXT_PRINT(CA[i],HEX);
    _DEBUG_EXT_PRINT(F(" "));
  }
  _DEBUG_EXT_PRINTLN();

  // Read AES keys from EEPROM
  for (int i=0; i < AES_KEY_SIZE; i++) {
    shared_aes[i] = EEPROM.read(EEPROM_AES_CHANGING+i);
    unique_aes[i] = EEPROM.read(EEPROM_AES_UNIQUE+i);
  }

  // Configure nrf24l01 radio
  regw(W_REGISTER | EN_AA,      0x01);            // Enable auto-ack for data pipe 0
  regw(W_REGISTER | EN_RXADDR,  0x01);            // Enable RX data pipe 0
  regw(W_REGISTER | SETUP_AW,   0x03);            // Sets 5 bytes address length
  regw(W_REGISTER | SETUP_RETR, 0x13);            // Retransmit delay ARD=1(500us). Retransmit count ARC=3(3 retransmits).
  regw(W_REGISTER | RF_CH,      0x42);            // Channel selection
  regw(W_REGISTER | RX_PW_P0,   RF_PAYLOAD_SIZE); // 16 bytes payload
  regw(W_REGISTER | RF_SETUP,   0x26);            // 250kbps transmission rate

  // Flush FIFOS
  digitalWriteFast(csn_pin, LOW);
  SPI.transfer(FLUSH_RX);
  digitalWriteFast(csn_pin, HIGH);

  // There is no data in RXbuffer yet
  rxbuffer = 0;
  txflush();

  // clear all interrupts
  regw(W_REGISTER | STATUS, 0x70);
  digitalWriteFast(ce_pin, HIGH);

  // Start as a receiver
  receiver();

  //_DEBUG_PRINTLN(F("Riots_Radio::setup done"));

  return reset_pin;
}

/**
 * Returns memory address of the receiver radio ID
 *
 * @return            Address of the radio ID array
 */
byte* Riots_Radio::getRadioReceiverAddress() {
  return SA;
}


/**
 * Returns memory address of the own radio ID
 *
 * @return            Address of the radio ID array
 */
byte* Riots_Radio::getOwnRadioAddress() {
  return CA;
}


/**
 * Returns memory address of the plain data buffer
 *
 * @return            Address of the plain data buffer array
 */
byte* Riots_Radio::getPlainDataAddress() {
  return plain_data;
}

/**
 * Returns memory address of the crypted data buffer
 *
 * @return            Address of the plain data buffer array
 */
byte* Riots_Radio::getTXCryptBuffAddress() {
  return tx_crypt_buff;
}


/**
 * Returns memory address of the crypted data buffer
 *
 * @return            Address of the plain data buffer array
 */
byte* Riots_Radio::getRXCryptBuffAddress() {
  return rx_crypt_buff;
}

/**
 * Returns memory address of the private key
 *
 * @return            Address of the private key array
 */
byte* Riots_Radio::getPrivateKeyAddress() {
  return unique_aes;
}


/**
 * Returns memory address of the shared key
 *
 * @return            Address of the shared key array
 */
byte* Riots_Radio::getSharedKeyAddress() {
  return shared_aes;
}


byte* Riots_Radio::getDebugBufferAddress() {
  return debug_buffer;
}

/**
* Sends a message to previous configured recipient
*
* @return byte      False if message was not delivered successfully
*/
byte Riots_Radio::send() {

  byte retvalue;

  long send_start = millis();

  // Switch to transmitter mode
  transmitter();
  // Write radiosend buffer to SPI
  digitalWriteFast(csn_pin, LOW);
  SPI.transfer(W_TX_PAYLOAD);

  _DEBUG_EXT_PRINT(F("Riots_Radio::send tx_crypt_buff: "));
  for (int i=0; i<RF_PAYLOAD_SIZE; i++) {
    _DEBUG_EXT_PRINT(tx_crypt_buff[i],HEX);
    _DEBUG_EXT_PRINT(F(" "));
    SPI.transfer(tx_crypt_buff[i]);
  }
  _DEBUG_EXT_PRINTLN(F(""));
  digitalWriteFast(csn_pin, HIGH);
  digitalWriteFast(ce_pin, HIGH);

  while(digitalRead(irq_pin)) {
    if((millis() - send_start) > MAX_RADIO_AIRTIME) {
      // fix problem where IRQ pin occassionally doesn't go down
      break;
    }
  }
  digitalWriteFast(ce_pin, LOW);

  retvalue = writeInterrupt();
  receiver();

  return retvalue;
}

/**
* Watchdog interrupt
*/
ISR (WDT_vect) {

  // Reset watchdog
  wdt_reset();
  // Enable watchdog again
  wdt_enable(WDTO_8S);
}

/**
* Watchdog sleep function. Sleep 8 seconds by default.
*/
void Riots_Radio::wdtSleep() {

  _DEBUG_EXT_PRINTLN(F("SLEEP"));

  set_sleep_mode(SLEEP_MODE_PWR_DOWN);
  // Turn off ad converter
  ADCSRA = 0;
  // Power down radio
  regw(W_REGISTER | CONFIG,      0x0D);
  // Enable watchdog interrupt
  WDTCSR |= (1 << WDIE);
  // Reset watchdog
  wdt_reset();
  // Enter to sleep mode
  sleep_mode();
  _DEBUG_EXT_PRINTLN(F("WAKE UP"));
  // Power up radio
  regw(W_REGISTER | CONFIG,      0x0F);
}

/**
* Check updates from the Riots network.
*
* With parameter it is possible to activate sleeping mode if there is no data
* in the radio.
*
* @param sleep      Status of wait.
* @return           True if we received data.
*/
byte Riots_Radio::update(byte sleep) {
  // Reset watchdog
  wdt_reset();

  if (digitalRead(irq_pin) > 0 && rxbuffer == 0) {
    // No interrupts and the buffer is empty
    if (sleep == 1) {
      // Device wants to activate a sleep routine
      wdtSleep();
    }
    return RIOTS_NO_DATA_AVAILABLE;
  }
  else {
    // Interrupt has fired, check the data
    readData();
    return RIOTS_OK;
  }
}

/**
* Decrypts the data. Use given AES128 keys for decrypting.
*
* @return      RIOTS_OK, if message was decrypted successfully and message was valid.
*/
byte Riots_Radio::decrypt(byte *key) {
  byte checksum = 0;

  // Uses a given key for decrypting
  AES128_ECB_decrypt(rx_crypt_buff, key, plain_data);

  // check the validity of the message
  for (int i=0; i < RF_PAYLOAD_SIZE; i++) {
    checksum = checksum^plain_data[i];
  }

  if (checksum == 0) {
    // CRC was valid, check the length of the message
    if (plain_data[M_LENGTH] < 0x0E) {
      return RIOTS_OK;
    }
  }
  return RIOTS_FAIL;
}

/**
* Cleans the transmitter pipe.
*
*/
void Riots_Radio::txflush() {

  digitalWriteFast(csn_pin, LOW);
  SPI.transfer(FLUSH_TX);
  digitalWriteFast(csn_pin, HIGH);
  // Clear errors
  this->sendStatus = 0;
}

/**
* Switches radio to the trasmitting mode.
*
*/
void Riots_Radio::transmitter() {

  byte fifostat = 0;
  _DEBUG_EXT_PRINTLN(F("Riots_Radio::transmitter"));

  // Set all interrupts, 2 bit CRC, Power up and PTX
  regw(W_REGISTER  | CONFIG,     0x0E);
  regw4(W_REGISTER | RX_ADDR_P0, SA);
  digitalWrite(ce_pin, LOW);
  // Clear RX interrupt
  digitalWrite(csn_pin, LOW);
  SPI.transfer(W_REGISTER | STATUS);
  SPI.transfer(0x40);
  digitalWrite(csn_pin, HIGH);
  // Read FIFO status
  digitalWrite(csn_pin, LOW);
  SPI.transfer(R_REGISTER | FIFO_STATUS);
  fifostat = SPI.transfer(0x00);
  digitalWrite(csn_pin, HIGH);
  // Data in RX FIFO
  if((fifostat & (1 << RX_EMPTY)) == 0) {
    rxbuffer = 1;
  }
  else {
    rxbuffer = 0;
  }
}

/**
* Updates the trasmitter address to the radio.
*
* @value address    Address of the transmitter in byte array.
*/
void Riots_Radio::setTXAddress(byte* address) {

  _DEBUG_EXT_PRINT(F("Riots_Radio::setTXAddress"));
  _DEBUG_EXT_PRINT(address[0], HEX);
  _DEBUG_EXT_PRINT(address[1], HEX);
  _DEBUG_EXT_PRINT(address[2], HEX);
  _DEBUG_EXT_PRINTLN(address[3], HEX);

  memcpy(SA, address, RF_ADDRESS_SIZE);
  regw4(W_REGISTER | TX_ADDR, SA);
}

/**
* Switches radio to the receiving mode.
*
*/
void Riots_Radio::receiver() {

  _DEBUG_EXT_PRINTLN(F("Riots_Radio::receiver"));

  this->transmitter_mode = 0;
  // Set all interrupts, 2 bit CRC, Power up and PRX
  regw(W_REGISTER | CONFIG,      0x0F);
  regw4(W_REGISTER | RX_ADDR_P0, CA);
  digitalWrite(csn_pin, LOW);
  SPI.transfer(FLUSH_TX);

  digitalWrite(csn_pin, HIGH);
  digitalWrite(ce_pin, HIGH);
}

/**
* Check and clears a interrupt register for the radio.
*
*/
byte Riots_Radio::writeInterrupt() {

  byte interrupt = 0;
  digitalWrite(csn_pin, LOW);
  // Get interrupt from STATUS register
  interrupt = SPI.transfer(W_REGISTER | STATUS);
  // Clear TX interrupts
  SPI.transfer(0x30);
  digitalWrite(csn_pin, HIGH);

  if(interrupt & (1 << TX_DS)) {
    // Write interrupt ok
    return RIOTS_OK;
  }
  return RIOTS_FAIL;
}

/**
* Reads data from the radio.
*
* 1) read payload through SPI
* 2) clear RX_DR IRQ
* 3) read FIFO_STATUS to check if there are more payloads available in RX FIFO
* 4) if there are more data in RX FIFO, repeat from step 1).
*
*/
void Riots_Radio::readData() {
  byte fifostat = 0;

  // Disable receiver
  digitalWrite(ce_pin, LOW);

  // Read payload
  _DEBUG_EXT_PRINT(F("Riots_Radio::readData rx_crypt_buff: "));
  digitalWrite(csn_pin, LOW);
  SPI.transfer(R_RX_PAYLOAD);
  for (int i=0; i < RF_PAYLOAD_SIZE; i++) {
    rx_crypt_buff[i] = SPI.transfer(0);
    _DEBUG_EXT_PRINT(rx_crypt_buff[i],HEX);
    _DEBUG_EXT_PRINT(F(" "));
  }
  _DEBUG_EXT_PRINTLN(F(""));
  digitalWrite(csn_pin, HIGH);

  // Clear interrupt
  digitalWrite(csn_pin, LOW);
  SPI.transfer(W_REGISTER | STATUS);
  SPI.transfer(0x40);
  digitalWrite(csn_pin, HIGH);

  // Read FIFO status
  digitalWrite(csn_pin, LOW);
  SPI.transfer(R_REGISTER | FIFO_STATUS);
  fifostat = SPI.transfer(0x00);
  digitalWrite(csn_pin, HIGH);

  // Data in RX FIFO
  if((fifostat & (1 << RX_EMPTY)) == 0) {
    rxbuffer = 1;
  }
  else {
    rxbuffer = 0;
  }

  // Enable receiver
  digitalWrite(ce_pin, HIGH);
}

/**
* Writes given value to register.
*
* @param reg      Register to be written.
* @param val      value to be written.
*/
void Riots_Radio::regw(byte reg, byte val) {

  digitalWrite(csn_pin, LOW);
  SPI.transfer(reg);
  SPI.transfer(val);
  digitalWrite(csn_pin, HIGH);
}

/**
* Writes given values to register.
*
* @param reg      Register to be written.
* @param val      Array of values to be written.
*/
void Riots_Radio::regw4(byte reg, byte val[]) {

  digitalWrite(csn_pin, LOW);
  SPI.transfer(reg);

  SPI.transfer(MAGIC_ADDRESS_BYTE);
  // Write in reverse order
  for (int i=3; i>=0; i--) {
    SPI.transfer(val[i]);
  }
  digitalWrite(csn_pin, HIGH);
}

/**
* Saves a new issued aes key from the cloud to memory
*
* @param key      Pointer to array which holds the key value.
*/
void Riots_Radio::saveNewAesKey(byte part_number, byte *new_key) {
  // TODO refactor this to another library
  if ( part_number == 0 || part_number == 1) {
    // allow only correct numbers, to prevent memory writing problems
    memcpy(aes_update+(part_number*AES_MSG_DELIVERY_SIZE), new_key, AES_MSG_DELIVERY_SIZE);
  }
}

/**
* Saves received keys to the EEPROM
*
* @param second_part            Is this part 1 or 2 of the aes keys?
* @param aes_keys               Address of the part of the aes_key
*/
void Riots_Radio::activateNewAesKey() {
  // TODO refactor this to another library

  memcpy(shared_aes, aes_update, AES_KEY_SIZE);

  for (int i = 0; i < AES_KEY_SIZE; i++) {
    EEPROM.write(EEPROM_AES_CHANGING+i, shared_aes[i]);
  }
}
