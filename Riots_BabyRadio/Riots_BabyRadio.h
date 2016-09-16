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

#ifndef Riots_BabyRadio_h
#define Riots_BabyRadio_h

#include "Riots_Helper.h"
#include "Riots_Radio.h"
#include "Riots_Flash.h"
#include "Riots_RGBLed.h"


class Riots_BabyRadio : public Print {
  public:
    void setup(byte indicativeLedsOn=1, byte debug=0, int nrfce=0xFF, int nrfcsn=0xFF, int nrfirq=0xFF, int nrfrst=0xFF);
    byte send(uint8_t index, int32_t data, int8_t factor);
    byte update(byte sleep=0);
    byte getCloudStatus();
    int32_t getData();
    uint8_t getIndex();
    uint32_t getSeconds();
    virtual size_t write(uint8_t);
    virtual size_t write(const uint8_t *buffer, size_t size);
    using Print::write;

  private:
    Riots_Radio riots_radio;            /*!< Implementation of the Radio Hardware Library                                   */
    Riots_Flash riots_flash;            /*!< Riots Flash library, used for flashing the firmware                            */
    Riots_RGBLed riots_RGBLed;          /*!< Implementation of the RGB LED  Library                                         */
    uint32_t sleep_count;               /*!< Counter for sleep cycles                                                       */
    uint32_t last_alive_time;           /*!< Last I'm alive sent time                                                       */
    uint16_t dataCounter;               /*!< Counter for Cloud events                                                       */
    uint16_t current_ring_counter;      /*!< Counter of the ring event                                                      */
    byte* unique_aes;                   /*!< ptr to Unique AES128 key for the mama, data allocated in Riots_MaraRadio side  */
    byte* shared_aes;                   /*!< ptr to Shared AES128 key for the mama, data allocated in Riots_MaraRadio side  */
    byte* own_address;                  /*!< ptr to own core radio address, data allocated in Riots_Radio side              */
    byte* plain_data;                   /*!< ptr to plain data which is used in both riot mamaradio and cloud               */
    byte* tx_crypt_buff;                /*!< tx_buffer to store crypted data and header                                     */
    byte* rx_crypt_buff;                /*!< rx_buffer to store crypted data and header                                     */
    byte MA[RF_ADDRESS_SIZE];           /*!< Mama radio address                                                             */
    byte CA[RF_ADDRESS_SIZE];           /*!< Child radio address                                                            */
    byte DA[RF_ADDRESS_SIZE];           /*!< Debug radio address                                                            */
    byte RN[RF_ADDRESS_SIZE];           /*!< Ring next address                                                              */
    byte RP[RF_ADDRESS_SIZE];           /*!< Ring previous address                                                          */
    byte RPC[2];                        /*!< Ring previous child id                                                         */
    byte RNC[2];                        /*!< Ring next child id                                                             */
    byte challenge[2];                  /*!< Storage for challenges used in config init                                     */
    byte childId[2];                    /*!< Received Child id from the cloud                                               */
    byte resp[8];                       /*!< Cloud reponse data                                                             */
    byte counter[2];                    /*!< Message counter                                                                */
    bool debugger;                      /*!< Debugger enabling status                                                       */
    byte send_status;                   /*!< Have previous send failed and we need to do resending for it                   */
    byte core_status;                   /*!< Riots Core specific statuses                                                   */
    byte net_status;                    /*!< Riots Network specific statuses                                                */
    byte aes_update[AES_KEY_SIZE];      /*!< Temporary storage for shared AES during update                                 */
    byte io_table[2][8];                /*!< Table for I/O mapping                                                          */
    byte io_index;                      /*!< I/O table index                                                                */
    byte saved_event[6];                /*!< Used for saving received ring event                                            */
    bool ring_data_available;           /*!< Ring data for INO                                                              */
    byte base_attached;                 /*!< Tells if there is official Riots Base attached to this Core                    */
    byte sleep_enabled;                 /*!< Tells if sleep mode is enabled                                                 */
    byte indicativeLeds;                /*!< Library led status                                                             */
    bool own_ring_event_ongoing;        /*!< Status if baby is waiting own ring event back from the ring                    */
    int reset_pin;                      /*!< Used pin number for resetting the system                                       */
    byte im_alive_fail_count;           /*!< Count of alive messages, TODO: replace this with bits in net_status            */

#ifdef RIOTS_FLASH_MODE
    byte flash_mode;                    /*!< Set if Baby is in programming mode                                             */
    uint32_t flash_start;               /*!< Programming start time                                                         */
#endif

    /* Private functions start here */
    byte processMessage();
    void formMessage(byte type, byte length, byte addCounter=1, byte forward=0);
    void addChildId(byte start_pos);
    void addChecksum();
    byte validatePrivateMessage();
    byte handlePrivateMessage();
    byte handleSharedMessage();
    bool checkSharedMessageValidity();
    byte cloudForward();
    byte ringForward();
    byte ringBackward();
    byte routeForward();
    void sendMessage(byte message, byte status);
    byte checkCounter();
    byte getMyIO(byte ringIO);
    byte getRingIO(byte myIO);
    byte uartSend();
    void updateLedStatus(byte sleep, uint32_t color = 0);
    bool isRingCounterValid();
    bool isRingAddressValid();
};

#endif // Riots_BabyRadio_h
