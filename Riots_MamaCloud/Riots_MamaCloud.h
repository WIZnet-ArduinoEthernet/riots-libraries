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

#ifndef Riots_MamaCloud_h
#define Riots_MamaCloud_h

#include "Riots_Helper.h"
#include "Ethernet.h"
#include "Riots_Mamadef.h"
#include "Riots_Memory.h"
#include "Riots_RGBLed.h"

class Riots_MamaCloud {
  public:
    void setAddresses(byte* plain_data_adr, byte* tx_crypt_buff_adr, byte* rx_crypt_buff_adr, byte* uniq_key_addr );
    void setup(byte indicativeLedsOn=0);
    byte update(byte *action_needed);
    byte* getNextReceiverAddress();
    byte getNextDataBlob();
    byte forwardToCloud();
    void processCachedMessage();
    void connectionSettingsVerificated();
    void sendCoreNotReached();
    void startFilling();
    void stopFilling();
    void fillByte(byte ch);

  private:
    EthernetClient ethernet_client;/*!< Instance of the EthernetClient libary, used for connection network           */
    Riots_RGBLed riots_RGBLed;    /*!< Instance of the Riots RGB library, used for indicating status to the user     */
    Riots_Memory riots_memory;    /*!< Instance for reading and writing base's EEPROM                                */
    byte sess_key[16];            /*!< Received AES key for current TCP session                                      */
    byte challenge[4];            /*!< Challenge used for verifying the both direction connections                   */
    uint16_t current_msg_ind;     /*!< Next index where saved message should be saved to EEPROM                      */
    uint16_t last_saved_msg_ind;  /*!< Index of EEPROM where saved message should be read and forwarded              */
    byte* uni_aes;                /*!< ptr to Unique AES128 key for the mama, data allocated in Riots_MaraRadio side */
    byte* aes_key;                /*!< ptr to Shared AES128 key for the mama, data allocated in Riots_MaraRadio side */
    byte* tx_address;             /*!< ptr to radio receiver address data allocated in Riots_MaraRadio side          */
    byte* plain_data;             /*!< ptr to plain data which is used in both riot mamaradio and cloud              */
    byte* rx_crypt_buff;          /*!< buffer to store crypted rx data and header                                    */
    byte* tx_crypt_buff;          /*!< buffer to store crypted tx data and header                                    */
    byte data_blobs_available;    /*!< Count of the datablobs still available for reading                            */
    bool session_key_received;    /*!< Do we have connection available and valid session key.                        */
    uint32_t myNextConnection;    /*!< Time for the next keep alive request                                          */
    uint32_t myNextAttempt;       /*!< Time for the next connectiong attempt                                         */
    byte eeprom_status;           /*!< Status of base eeprom                                                         */
    bool eeprom_looped_once;      /*!< internal eeprom has been filled once with full of data                        */
    bool connection_verificated;  /*!< Have the connection verified with the cloud                                   */
    bool pending_message;         /*!< Do we have saved messages in EEPROM                                           */
    bool time_received;           /*!< Have we yet received a correct time from the cloud.                           */
    byte indicativeLeds;          /*!< Has led indication requested from the INO                                     */

    // private functions starts from here
    void saveMessage();
    byte dhcpValidated();
    byte validateConnection();
    byte validateSession();
    byte validateReceiver();
    bool isMamaConfigMessage();
    void sendRequestToCloud(Riots_Message cloud_message_type);
    void fillRandomPadding(byte* start_ptr, byte length);
    byte calcChecksum(byte* input, byte lenght);
    bool readLastCachedMessage(byte* read_buffer);
    void activateLeds(uint32_t rgb);
 };

#endif // Riots_MamaRadio_h
