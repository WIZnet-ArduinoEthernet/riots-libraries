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

#ifndef Riots_MamaRadio_h
#define Riots_MamaRadio_h

#include "Riots_Helper.h"
#include "Riots_Radio.h"
#include "Riots_Flash.h"
#include "Riots_RGBLed.h"

class Riots_MamaRadio{
  public:
    void setup(byte indicativeLedsOn = 0, byte debug=0, int nrfce=0xFF, int nrfcsn=0xFF, int nrfirq=0xFF, int nrfrst=0xFF);
    void setRadioReceiverAddress(byte* new_address);
    byte* getPlainDataAddress();
    byte* getTXCryptBuffAddress();
    byte* getRXCryptBuffAddress();
    byte* getPrivateKeyAddress();
    byte update(byte sleep=0);
    byte checkRiotsMsgValidity();
    bool messageDelivered(byte status);
    byte processMsg(bool *reply_needed);
    void createCoreNotReachedMsg();

   private:
    Riots_Radio riots_radio;            /*!< Implementation of the Radio Hardware Library                                   */
    Riots_RGBLed riots_RGBLed;          /*!< Implementation of the RIOTS led libary, used for led indications               */
    byte configCounter[2];              /*!< Config counter, received during initialization from the cloud                  */
    byte childId[2];                    /*!< Child id received from cloud                                                   */
    byte childTxAddr[4];                /*!< TX address used for sending                                                    */
    Riots_Flash riots_flash;            /*!< Implementation of the flash library                                            */
    byte* unique_aes;                   /*!< ptr to Unique AES128 key for the mama, data allocated in Riots_Radio side      */
    byte* shared_aes;                   /*!< ptr to Shared AES128 key for the mama, data allocated in Riots_Radio side      */
    byte* own_address;                  /*!< ptr to wwn core radio address, data allocated in Riots_Radio side              */
    byte* tx_address;                   /*!< ptr to transmitter address                                                     */
    byte* plain_data;                   /*!< ptr to plain data which is shared between the libraries                        */
    byte* tx_crypt_buff;                /*!< buffer to store crypted tx data and header                                     */
    byte* rx_crypt_buff;                /*!< buffer to store crypted rx data and header                                     */
    bool own_config_message;            /*!< Is next message meant for me?                                                  */
    bool first_aes_part_received;       /*!< Have we received first part of new AES key                                     */
    byte last_message_type;             /*!< Last delivered message type                                                    */
    bool alive_msg_sent;                /*!< Have we sent alive message to the cloud                                        */
    int resetPin;                       /*!< reset pin number                                                               */
    bool mama_reset_acknowledged;       /*!< Is it ok to reset mama after message is delivered to the cloud                 */
    byte indicativeLeds;                /*!< Has the led indications enabled from the INO                                   */

    /* Private functions start here */
    void createResponse(byte answer_type, byte message_type=0, byte status=0);
    byte checkCounter();
    byte getSavedMessageCount();
    void readNextSavedMessage();
    void wdtSleep();
    byte handleOwnConfigMessage(bool *reply_needed);
};

#endif // Riots_MamaRadio_h
