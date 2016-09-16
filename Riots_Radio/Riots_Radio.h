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

#ifndef Riots_Radio_h
#define Riots_Radio_h

#include "Riots_Helper.h"

class Riots_Radio {
  public:
    int setup(int nrfce, int nrfcsn, int nrfirq, int nrfrst);
    byte* getRadioReceiverAddress();
    byte* getPlainDataAddress();
    byte* getDebugBufferAddress();
    byte* getSharedKeyAddress();
    byte* getRXCryptBuffAddress();
    byte* getTXCryptBuffAddress();
    byte* getPrivateKeyAddress();
    byte* getOwnRadioAddress();
    void activateNewAesKey();
    void saveNewAesKey(byte part_number, byte *aes_key_part);
    byte decrypt(byte* aes_key);
    byte send();
    byte update(byte sleep);
    byte validityCheck();
    void setTXAddress(byte *address);

  private:
    byte unique_aes[AES_KEY_SIZE];      /*!< Unique AES128 key for the child                */
    byte shared_aes[AES_KEY_SIZE];      /*!< Public AES128 key for the RIOTS network        */
    byte aes_update[AES_KEY_SIZE];      /*!< Public AES128 key for the RIOTS network        */
    byte CA[RF_ADDRESS_SIZE];           /*!< Own core radio address                         */
    byte SA[RF_ADDRESS_SIZE];           /*!< Transmitter address of the radio               */
    byte plain_data[RF_PAYLOAD_SIZE+2]; /*!< Shared data buffer, used for plain data        */
    byte tx_crypt_buff[RF_PAYLOAD_SIZE+2]; /*!< Shared tx data buffer, used for crypted data      */
    byte rx_crypt_buff[RF_PAYLOAD_SIZE+2]; /*!< Shared rx data buffer, used for crypted data*/
    byte debug_buffer[RF_PAYLOAD_SIZE];
    byte sendStatus;                    /*!< Status of sending                              */
    byte sendCount;                     /*!< Count message resended attempts                */
    byte debugger;                      /*!< Is debugger enabled                            */
    byte transmitter_mode;              /*!< Is the radio in the trasmitting mode           */
    int ce_pin;                         /*!< Chip Enable pin number                         */
    int csn_pin;                        /*!< SPI Chip celect pin number                     */
    int irq_pin;                        /*!< Maskable inttupt pin number                    */
    int reset_pin;                      /*!< Reset pin number                               */
    unsigned long sendTime;             /*!< Time to try sending a message                  */
    byte rxbuffer;                      /*!< Do we have some data left in rx buffer         */

    /* Private functions start here */
    void wdtSleep();
    byte mamaSend();
    byte radioSend();
    byte resend();
    void txflush();
    void transmitter();
    void receiver();
    byte writeInterrupt();
    void readData();
    void regw(byte reg, byte val);
    void regw4(byte reg, byte val[]);
    void aeskey(byte key[]);
    bool checkCRC();
    byte handleMessage();
};

#endif
