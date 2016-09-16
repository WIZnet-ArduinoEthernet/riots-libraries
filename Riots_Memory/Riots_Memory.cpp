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

#include "Riots_Memory.h"
#include <util/twi.h>

// Start I2c
uint8_t Riots_Memory::setup(uint8_t eeprom_addr) {
  uint8_t ret = 1;
  TWSR = 0; // set prescalar to zero
  TWBR = ((F_CPU/F_SCL)-16)/2; // set SCL frequency in TWI bit register
  ret = I2C_Start();
  if(ret) {
    ret = I2C_SendAddr(eeprom_addr); // send bus address
  }
  I2C_Stop(); // stop
  return ret;
}

uint8_t Riots_Memory::I2C_Start() {
  // reset TWI control register
  TWCR = 0;
  // transmit START condition
  TWCR = (1<<TWINT) | (1<<TWSTA) | (1<<TWEN);
  // wait for end of transmission
  while( !(TWCR & (1<<TWINT)) );

  // check if the start condition was successfully transmitted
	if((TWSR & 0xF8) != TW_START){ return 0; }
  return 1;
}

uint8_t Riots_Memory::I2C_SendAddr(uint8_t addr) {
  TWDR = addr; // load device's bus address
  TWCR = TW_SEND; // and send it
  while (!TW_READY); // wait
  return (TW_STATUS==0x18); // return 1 if found; 0 otherwise
}

uint8_t Riots_Memory::I2C_Write(uint8_t data) {
  TWDR = data; // load data to be sent

  TWCR = (1<<TWINT) | (1<<TWEN);
  while ((TWCR & (1<<TWINT)) == 0);

  if( (TWSR & 0xF8) != TW_MT_DATA_ACK ){ return 1; }
  return 0;
}

uint8_t Riots_Memory::I2C_ReadNACK() {
  TWCR = (1<<TWINT)|(1<<TWEN);
  while ((TWCR & (1<<TWINT)) == 0);
  return TWDR;
}

uint8_t Riots_Memory::I2C_ReadACK() {
  TWCR = (1<<TWINT)|(1<<TWEN)|(1<<TWEA);
  while ((TWCR & (1<<TWINT)) == 0);
  return TWDR;
}

/* Writes a single byte to I2C eeprom */
void Riots_Memory::write(uint16_t page_addr, uint8_t data, uint8_t eeprom_addr) {
  I2C_Start();
  I2C_SendAddr(eeprom_addr); // send bus address
  I2C_Write((page_addr>>8)&0xFF); // first uint8_t = device register address
  I2C_Write(page_addr&0xFF); // first uint8_t = device register address
  I2C_Write(data);
  I2C_Stop();
  //Todo: Check from data sheet is shorter delay enough for single byte write
  delay(5);
}

void Riots_Memory::startPageWrite(uint16_t page_addr, uint8_t eeprom_addr) {
  I2C_Start();
  I2C_SendAddr(eeprom_addr); // send bus address
  I2C_Write((page_addr>>8)&0xFF); // first uint8_t = device register address
  I2C_Write(page_addr&0xFF); // first uint8_t = device register address
}

void Riots_Memory::pageFill(uint8_t data) {
  I2C_Write(data);
  //I2C_ReadNACK();
}

void Riots_Memory::stopPageWrite() {
  I2C_Stop();
  delay(5);
}


void Riots_Memory::startRead(uint16_t page_addr, uint8_t eeprom_addr) {
  I2C_Start();
  I2C_SendAddr(eeprom_addr); // send bus address
  I2C_Write((page_addr>>8) & 0xFF);
  I2C_Write(page_addr & 0xFF);

  I2C_Start();
  I2C_SendAddr((eeprom_addr+1)); // send bus read address
}

uint8_t Riots_Memory::sequentialRead() {
  uint8_t data = I2C_ReadACK();
  return data;
}

uint8_t Riots_Memory::readLast() {
  uint8_t data = I2C_ReadNACK();
  I2C_Stop(); // stop
  return data;
}


/* Reads a single byte from I2C eeprom */
uint8_t Riots_Memory::read(uint16_t page_addr, uint8_t eeprom_addr) {
  uint8_t data = 0;

  I2C_Start();
  I2C_SendAddr(eeprom_addr); // send bus address
  I2C_Write((page_addr>>8) & 0xFF);
  I2C_Write(page_addr & 0xFF);

  I2C_Start();
  I2C_SendAddr((eeprom_addr+1)); // send bus READ address
  data = I2C_ReadNACK();
  I2C_Stop(); // stop

  return data;
}