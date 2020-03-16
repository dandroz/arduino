/*
Copyright (C) AC SOFTWARE SP. Z O.O.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.
This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.
You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
*/

#include <SPI.h>
#include <SuplaDevice.h>

/*
 * This example requires Dallas Temperature Control library installed. 
 * https://github.com/milesburton/Arduino-Temperature-Control-Library
 */
// Add include to DS sensor
#include <supla/sensor/DS18B20.h>

#include <supla/network/esp_wifi.h>
Supla::ESPWifi wifi("your_wifi_ssid", "your_wifi_password");

void setup() {

  Serial.begin(9600);

  // Replace the falowing GUID with value that you can retrieve from https://www.supla.org/arduino/get-guid
  char GUID[SUPLA_GUID_SIZE] = {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00};

  // Replace the following AUTHKEY with value that you can retrieve from: https://www.supla.org/arduino/get-authkey
  char AUTHKEY[SUPLA_AUTHKEY_SIZE] = {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00};

  /*
   * Having your device already registered at cloud.supla.org,
   * you want to change CHANNEL sequence or remove any of them,
   * then you must also remove the device itself from cloud.supla.org.
   * Otherwise you will get "Channel conflict!" error.
   */

  // 4 DS18B20 thermometers at pin 23. DS address can be omitted when there is only one device at a pin
  DeviceAddress ds1addr = {0x28, 0xFF, 0xC8, 0xAB, 0x6E, 0x18, 0x01, 0xFC};
  DeviceAddress ds2addr = {0x28, 0xFF, 0x54, 0x73, 0x6E, 0x18, 0x01, 0x77};
  DeviceAddress ds3addr = {0x28, 0xFF, 0x55, 0xCA, 0x6B, 0x18, 0x01, 0x8D};
  DeviceAddress ds4addr = {0x28, 0xFF, 0x4F, 0xAB, 0x6E, 0x18, 0x01, 0x66};

  new Supla::Sensor::DS18B20(23, ds1addr);
  new Supla::Sensor::DS18B20(23, ds2addr);
  new Supla::Sensor::DS18B20(23, ds3addr);
  new Supla::Sensor::DS18B20(23, ds4addr);

  /* By default this library uses secured connection with SUPLA server. You can switch it of by writing below line 
   * Works only with ESP8266 boards!
   */

  wifi.enableSSL(false);

  /*
   * SuplaDevice Initialization.
   * If you do not have an account, you can create it at https://cloud.supla.org/account/create
   * SUPLA and SUPLA CLOUD are free of charge
   */

  SuplaDevice.begin(GUID,              // Global Unique Identifier 
                    "svr1.supla.org",  // SUPLA server address
                    "email@address",   // Email address used to login to Supla Cloud
                    AUTHKEY);          // Authorization key
    
}

void loop() {
  SuplaDevice.iterate();
}
